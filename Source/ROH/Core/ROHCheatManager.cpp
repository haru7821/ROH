#include "Core/ROHCheatManager.h"
#include "Items/ROHItemDatabase.h"
#include "Items/ROHInventoryComponent.h"
#include "Items/ROHItemTypes.h"
#include "Character/ROHPlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "ROH.h"

namespace
{
	UAbilitySystemComponent* GetPlayerASC(const UCheatManager* Cheat)
	{
		const APlayerController* PC = Cheat->GetOuterAPlayerController();
		if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PC ? PC->GetPawn() : nullptr))
		{
			return ASI->GetAbilitySystemComponent();
		}
		return nullptr;
	}

	UROHInventoryComponent* GetPlayerInventory(const UCheatManager* Cheat)
	{
		const APlayerController* PC = Cheat->GetOuterAPlayerController();
		if (const AROHPlayerCharacter* Player = Cast<AROHPlayerCharacter>(PC ? PC->GetPawn() : nullptr))
		{
			return Player->GetInventory();
		}
		return nullptr;
	}

	UROHItemDatabase* GetDatabase(const UCheatManager* Cheat)
	{
		const UWorld* World = Cheat->GetWorld();
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UROHItemDatabase>() : nullptr;
	}

	void CheatPrint(const FString& Line)
	{
		UE_LOG(LogROH, Log, TEXT("%s"), *Line);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan, Line);
		}
	}
}

void UROHCheatManager::ROHSetAttr(FName AttributeName, float Value)
{
	UAbilitySystemComponent* ASC = GetPlayerASC(this);
	if (!ASC)
	{
		UE_LOG(LogROH, Warning, TEXT("ROHSetAttr: 플레이어 ASC를 찾을 수 없습니다."));
		return;
	}

	TArray<FGameplayAttribute> Attributes;
	ASC->GetAllAttributes(Attributes);

	for (const FGameplayAttribute& Attribute : Attributes)
	{
		if (Attribute.GetName() == AttributeName.ToString())
		{
			ASC->SetNumericAttributeBase(Attribute, Value);
			UE_LOG(LogROH, Log, TEXT("ROHSetAttr: %s = %.1f"), *Attribute.GetName(), Value);
			return;
		}
	}

	UE_LOG(LogROH, Warning, TEXT("ROHSetAttr: '%s' 어트리뷰트가 없습니다. ROHDumpAttrs로 이름을 확인하세요."), *AttributeName.ToString());
}

void UROHCheatManager::ROHDumpAttrs()
{
	UAbilitySystemComponent* ASC = GetPlayerASC(this);
	if (!ASC)
	{
		return;
	}

	TArray<FGameplayAttribute> Attributes;
	ASC->GetAllAttributes(Attributes);

	for (const FGameplayAttribute& Attribute : Attributes)
	{
		const float Base = ASC->GetNumericAttributeBase(Attribute);
		const float Current = ASC->GetNumericAttribute(Attribute);
		const FString Line = FString::Printf(TEXT("%s: base=%.1f current=%.1f"), *Attribute.GetName(), Base, Current);
		UE_LOG(LogROH, Log, TEXT("%s"), *Line);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Cyan, Line);
		}
	}
}

void UROHCheatManager::ROHGiveItem(FName BaseId, int32 ItemLevel, FString Quality)
{
	UROHItemDatabase* Database = GetDatabase(this);
	UROHInventoryComponent* Inventory = GetPlayerInventory(this);
	if (!Database || !Inventory)
	{
		return;
	}

	EROHItemQuality QualityEnum = EROHItemQuality::Magic;
	if (Quality.Equals(TEXT("normal"), ESearchCase::IgnoreCase))
	{
		QualityEnum = EROHItemQuality::Normal;
	}
	else if (Quality.Equals(TEXT("rare"), ESearchCase::IgnoreCase))
	{
		QualityEnum = EROHItemQuality::Rare;
	}

	const FROHItemInstance Item = Database->GenerateItem(BaseId, ItemLevel, QualityEnum);
	if (!Item.IsValid())
	{
		CheatPrint(FString::Printf(TEXT("ROHGiveItem: 알 수 없는 베이스 '%s'"), *BaseId.ToString()));
		return;
	}
	if (Inventory->AddItem(Item))
	{
		CheatPrint(FString::Printf(TEXT("획득: %s (접사 %d개)"),
			*Database->GetItemDisplayName(Item).ToString(), Item.Affixes.Num()));
	}
	else
	{
		CheatPrint(TEXT("인벤토리가 가득 찼습니다"));
	}
}

void UROHCheatManager::ROHDumpInventory()
{
	UROHItemDatabase* Database = GetDatabase(this);
	UROHInventoryComponent* Inventory = GetPlayerInventory(this);
	if (!Database || !Inventory)
	{
		return;
	}

	CheatPrint(FString::Printf(TEXT("=== 골드: %d ==="), Inventory->GetGold()));

	CheatPrint(TEXT("--- 장비창 ---"));
	for (const auto& Pair : Inventory->GetEquipped())
	{
		FString AffixText;
		for (const FROHAffixRoll& Affix : Pair.Value.Affixes)
		{
			AffixText += FString::Printf(TEXT(" [%s +%.0f]"), *Affix.AffixId.ToString(), Affix.Value);
		}
		CheatPrint(FString::Printf(TEXT("슬롯 %d: %s%s"),
			static_cast<int32>(Pair.Key),
			*Database->GetItemDisplayName(Pair.Value).ToString(), *AffixText));
	}

	CheatPrint(TEXT("--- 인벤토리 ---"));
	const TArray<FROHItemInstance>& Items = Inventory->GetItems();
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		CheatPrint(FString::Printf(TEXT("%d: %s (ilvl %d, 접사 %d)"),
			i, *Database->GetItemDisplayName(Items[i]).ToString(),
			Items[i].ItemLevel, Items[i].Affixes.Num()));
	}
}

void UROHCheatManager::ROHEquip(int32 ItemIndex)
{
	if (UROHInventoryComponent* Inventory = GetPlayerInventory(this))
	{
		CheatPrint(Inventory->EquipItemByIndex(ItemIndex)
			? FString::Printf(TEXT("장착 완료 (인덱스 %d). ROHDumpAttrs로 스탯 확인"), ItemIndex)
			: TEXT("장착 실패 (인덱스/종류 확인)"));
	}
}

void UROHCheatManager::ROHUsePotion()
{
	if (UROHInventoryComponent* Inventory = GetPlayerInventory(this))
	{
		CheatPrint(Inventory->UseFirstPotion() ? TEXT("물약 사용") : TEXT("물약 없음"));
	}
}

void UROHCheatManager::ROHAddGold(int32 Amount)
{
	if (UROHInventoryComponent* Inventory = GetPlayerInventory(this))
	{
		Inventory->AddGold(Amount);
		CheatPrint(FString::Printf(TEXT("골드: %d"), Inventory->GetGold()));
	}
}

void UROHCheatManager::ROHBuyPotion()
{
	UROHItemDatabase* Database = GetDatabase(this);
	UROHInventoryComponent* Inventory = GetPlayerInventory(this);
	if (!Database || !Inventory)
	{
		return;
	}

	const FROHItemBaseDef* PotionBase = Database->FindBase(TEXT("HealthPotion"));
	if (!PotionBase)
	{
		return;
	}
	if (Inventory->IsFull())
	{
		CheatPrint(TEXT("인벤토리가 가득 찼습니다"));
		return;
	}
	if (!Inventory->SpendGold(PotionBase->GoldValue))
	{
		CheatPrint(FString::Printf(TEXT("골드 부족 (필요: %d)"), PotionBase->GoldValue));
		return;
	}
	if (!Inventory->AddItem(Database->GenerateItem(TEXT("HealthPotion"), 1, EROHItemQuality::Normal)))
	{
		Inventory->AddGold(PotionBase->GoldValue); // 실패 시 환불
		return;
	}
	CheatPrint(FString::Printf(TEXT("치유물약 구매 (-%d 골드, 잔액 %d)"), PotionBase->GoldValue, Inventory->GetGold()));
}

void UROHCheatManager::ROHSimulateDrops(FName TCId, int32 Count)
{
	UROHItemDatabase* Database = GetDatabase(this);
	if (!Database || Count <= 0)
	{
		return;
	}

	int32 TotalItems = 0;
	int32 TotalGold = 0;
	int32 GoldDrops = 0;
	int32 QualityCounts[3] = { 0, 0, 0 }; // Normal, Magic, Rare
	TMap<FName, int32> BaseCounts;

	for (int32 i = 0; i < Count; ++i)
	{
		const FROHDropResult Drop = Database->RollTreasureClass(TCId, 5, 0.f);
		TotalItems += Drop.Items.Num();
		TotalGold += Drop.Gold;
		if (Drop.Gold > 0)
		{
			++GoldDrops;
		}
		for (const FROHItemInstance& Item : Drop.Items)
		{
			const int32 QualityIndex = FMath::Clamp(static_cast<int32>(Item.Quality), 0, 2);
			++QualityCounts[QualityIndex];
			++BaseCounts.FindOrAdd(Item.BaseId);
		}
	}

	CheatPrint(FString::Printf(TEXT("=== 드랍 시뮬레이션: %s × %d회 ==="), *TCId.ToString(), Count));
	CheatPrint(FString::Printf(TEXT("아이템 %d개 (%.1f%%) | 골드 드랍 %d회, 평균 %.1f"),
		TotalItems, 100.f * TotalItems / Count, GoldDrops, GoldDrops > 0 ? static_cast<float>(TotalGold) / GoldDrops : 0.f));
	CheatPrint(FString::Printf(TEXT("등급: 일반 %d / 매직 %d / 레어 %d"),
		QualityCounts[0], QualityCounts[1], QualityCounts[2]));
	for (const auto& Pair : BaseCounts)
	{
		CheatPrint(FString::Printf(TEXT("  %s: %d"), *Pair.Key.ToString(), Pair.Value));
	}
}
