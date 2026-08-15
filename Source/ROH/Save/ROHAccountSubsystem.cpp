#include "Save/ROHAccountSubsystem.h"
#include "Save/ROHAccountSaveGame.h"
#include "Items/ROHItemDatabase.h"
#include "Character/ROHAttributeSet.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h" // GetSubsystem<T>() 템플릿 인스턴스화에 완전한 타입 필요
#include "ROH.h"

const TCHAR* UROHAccountSubsystem::AccountSlotName = TEXT("ROH_Account");

void UROHAccountSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 스태시 접사 재해석에 아이템 DB가 필요 — 초기화 순서 보장
	Collection.InitializeDependency(UROHItemDatabase::StaticClass());

	if (UGameplayStatics::DoesSaveGameExist(AccountSlotName, 0))
	{
		if (const UROHAccountSaveGame* Save = Cast<UROHAccountSaveGame>(UGameplayStatics::LoadGameFromSlot(AccountSlotName, 0)))
		{
			if (Save->SaveVersion == UROHAccountSaveGame::CurrentVersion)
			{
				ParagonLevel = FMath::Max(0, Save->ParagonLevel);
				ParagonXP = FMath::Max(0, Save->ParagonXP);
				ParagonPoints = FMath::Max(0, Save->ParagonPoints);
				ParagonAllocations = Save->ParagonAllocations;
				StashItems = Save->StashItems;

				// 접사 Attribute 재해석 (캐릭터 세이브 로드와 동일 규칙 — DB의 공용 헬퍼 사용)
				if (const UGameInstance* GameInstance = GetGameInstance())
				{
					if (const UROHItemDatabase* Database = GameInstance->GetSubsystem<UROHItemDatabase>())
					{
						for (FROHItemInstance& Item : StashItems)
						{
							Database->RefreshItemAffixes(Item);
						}
					}
				}
				UE_LOG(LogROH, Log, TEXT("계정 로드: 정복자 Lv %d (포인트 %d), 스태시 %d개"),
					ParagonLevel, ParagonPoints, StashItems.Num());
			}
			else
			{
				UE_LOG(LogROH, Warning, TEXT("계정 세이브 버전 불일치 (%d != %d) — 기본값으로 시작"),
					Save->SaveVersion, UROHAccountSaveGame::CurrentVersion);
			}
		}
	}
}

bool UROHAccountSubsystem::SaveAccount()
{
	UROHAccountSaveGame* Save = Cast<UROHAccountSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UROHAccountSaveGame::StaticClass()));
	if (!Save)
	{
		return false;
	}
	Save->ParagonLevel = ParagonLevel;
	Save->ParagonXP = ParagonXP;
	Save->ParagonPoints = ParagonPoints;
	Save->ParagonAllocations = ParagonAllocations;
	Save->StashItems = StashItems;

	const bool bSaved = UGameplayStatics::SaveGameToSlot(Save, AccountSlotName, 0);
	if (!bSaved)
	{
		UE_LOG(LogROH, Warning, TEXT("계정 저장 실패"));
	}
	return bSaved;
}

void UROHAccountSubsystem::GrantParagonXP(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	ParagonXP += Amount;
	++ChangeSerial; // b31: 정복자 창 XP 진행 표시 갱신
	bool bLeveledUp = false;
	while (ParagonXP >= ParagonXPForNextLevel(ParagonLevel))
	{
		ParagonXP -= ParagonXPForNextLevel(ParagonLevel);
		++ParagonLevel;
		++ParagonPoints;
		bLeveledUp = true;
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor(255, 200, 60),
				FString::Printf(TEXT("정복자 레벨 %d! (정복자 포인트 +1 — ROHParagonUp)"), ParagonLevel));
		}
		UE_LOG(LogROH, Log, TEXT("정복자 레벨 업: %d"), ParagonLevel);
	}

	// 레벨업은 잃으면 아프다 — 레벨업 시점마다 즉시 저장 (XP 단위 저장은 과도)
	if (bLeveledUp)
	{
		SaveAccount();
	}
}

const TArray<FName>& UROHAccountSubsystem::GetParagonCategories()
{
	static const TArray<FName> Categories = {
		TEXT("Offense"), TEXT("Defense"), TEXT("Precision"), TEXT("RuneAttune") };
	return Categories;
}

bool UROHAccountSubsystem::GetParagonCategoryBonus(FName Category, FGameplayAttribute& OutAttribute, float& OutPerPoint)
{
	// 4분류 (docs/02 정복자): 공격/방어/정밀/룬 조율
	if (Category == TEXT("Offense"))
	{
		OutAttribute = UROHAttributeSet::GetAttackPowerAttribute();
		OutPerPoint = 1.f;
		return true;
	}
	if (Category == TEXT("Defense"))
	{
		OutAttribute = UROHAttributeSet::GetMaxHealthAttribute();
		OutPerPoint = 5.f;
		return true;
	}
	if (Category == TEXT("Precision"))
	{
		OutAttribute = UROHAttributeSet::GetCritChanceAttribute();
		OutPerPoint = 0.1f;
		return true;
	}
	if (Category == TEXT("RuneAttune"))
	{
		OutAttribute = UROHAttributeSet::GetRunePowerAttribute();
		OutPerPoint = 0.2f;
		return true;
	}
	return false;
}

bool UROHAccountSubsystem::AllocateParagonPoint(FName Category, FString& OutError)
{
	FGameplayAttribute Attribute;
	float PerPoint = 0.f;
	if (!GetParagonCategoryBonus(Category, Attribute, PerPoint))
	{
		OutError = TEXT("분류: Offense(공격) / Defense(방어) / Precision(정밀) / RuneAttune(룬 조율)");
		return false;
	}
	if (ParagonPoints <= 0)
	{
		OutError = TEXT("정복자 포인트가 없습니다 (만렙 후 경험치로 획득)");
		return false;
	}
	int32& Allocated = ParagonAllocations.FindOrAdd(Category);
	if (Allocated >= ParagonCategoryCap)
	{
		OutError = FString::Printf(TEXT("분류 상한 도달 (%d포인트)"), ParagonCategoryCap);
		return false;
	}

	--ParagonPoints;
	++Allocated;
	++ChangeSerial; // b31
	SaveAccount(); // 계정 데이터는 즉시 저장
	return true;
}

bool UROHAccountSubsystem::StashDeposit(const FROHItemInstance& Item)
{
	if (!Item.IsValid() || IsStashFull())
	{
		return false;
	}
	StashItems.Add(Item);
	++ChangeSerial; // b31
	SaveAccount(); // 계정 데이터는 즉시 저장
	return true;
}

bool UROHAccountSubsystem::StashWithdrawAt(int32 StashIndex, FROHItemInstance& OutItem)
{
	if (!StashItems.IsValidIndex(StashIndex))
	{
		return false;
	}
	OutItem = StashItems[StashIndex];
	StashItems.RemoveAt(StashIndex);
	++ChangeSerial; // b31
	SaveAccount(); // 계정 데이터는 즉시 저장
	return true;
}
