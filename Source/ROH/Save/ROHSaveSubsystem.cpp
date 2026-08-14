#include "Save/ROHSaveSubsystem.h"
#include "Save/ROHSaveGame.h"
#include "Character/ROHPlayerCharacter.h"
#include "Character/ROHPlayerClasses.h"
#include "Items/ROHInventoryComponent.h"
#include "Items/ROHItemDatabase.h"
#include "Progression/ROHProgressionComponent.h"
#include "Progression/ROHSkillTreeComponent.h"
#include "Core/ROHGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "ROH.h"

const TCHAR* UROHSaveSubsystem::SlotName = TEXT("ROH_Default");

bool UROHSaveSubsystem::HasSave() const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}

bool UROHSaveSubsystem::SaveCharacter(AROHPlayerCharacter* Player)
{
	if (!Player || !Player->GetProgression() || !Player->GetSkillTree() || !Player->GetInventory())
	{
		return false;
	}

	UROHSaveGame* Save = Cast<UROHSaveGame>(UGameplayStatics::CreateSaveGameObject(UROHSaveGame::StaticClass()));
	if (!Save)
	{
		return false;
	}

	Save->PlayerClassName =
		Player->GetSkillTree()->GetPlayerClass() == EROHPlayerClass::Elementalist ? TEXT("Elementalist") : TEXT("Warrior");

	const UROHProgressionComponent* Progression = Player->GetProgression();
	Save->Level = Progression->GetLevel();
	Save->XP = Progression->GetXP();
	Save->StatPoints = Progression->GetStatPoints();
	Save->SkillPoints = Progression->GetSkillPoints();
	Save->AllocatedStats = Progression->GetAllocatedStats();
	Save->SkillHardPoints = Player->GetSkillTree()->GetHardPoints();
	Player->GetInventory()->ExportState(Save->InventoryItems, Save->EquippedItems, Save->Gold);

	const bool bSaved = UGameplayStatics::SaveGameToSlot(Save, SlotName, 0);
	UE_LOG(LogROH, Log, TEXT("세이브 %s (Lv %d, %s)"), bSaved ? TEXT("성공") : TEXT("실패"), Save->Level, *Save->PlayerClassName);
	return bSaved;
}

AROHPlayerCharacter* UROHSaveSubsystem::LoadCharacter(AROHPlayerCharacter* CurrentPlayer)
{
	if (!CurrentPlayer || !HasSave())
	{
		return nullptr;
	}

	UROHSaveGame* Save = Cast<UROHSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (!Save)
	{
		return nullptr;
	}
	if (Save->SaveVersion != UROHSaveGame::CurrentVersion)
	{
		UE_LOG(LogROH, Warning, TEXT("세이브 버전 불일치 (%d != %d) — 마이그레이션 미구현"), Save->SaveVersion, UROHSaveGame::CurrentVersion);
		return nullptr;
	}

	// 접사 어트리뷰트 참조를 데이터베이스 기준으로 재구성 (경로 직렬화 의존 제거)
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UROHItemDatabase* Database = GameInstance->GetSubsystem<UROHItemDatabase>())
		{
			auto RefreshAffixes = [Database](FROHItemInstance& Item)
			{
				for (FROHAffixRoll& Affix : Item.Affixes)
				{
					if (const FROHAffixDef* Def = Database->FindAffix(Affix.AffixId))
					{
						Affix.Attribute = Def->Attribute;
					}
				}
			};
			for (FROHItemInstance& Item : Save->InventoryItems)
			{
				RefreshAffixes(Item);
			}
			for (auto& Pair : Save->EquippedItems)
			{
				RefreshAffixes(Pair.Value);
			}
		}
	}

	// 항상 저장된 클래스로 새 폰을 스폰해 깨끗한 초기 상태에서 복원
	// (기존 폰에 덧적용하면 세션 중 분배분과 이중 적용될 수 있음)
	APlayerController* PC = Cast<APlayerController>(CurrentPlayer->GetController());
	AROHGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AROHGameMode>() : nullptr;
	if (!PC || !GameMode)
	{
		return nullptr;
	}
	const bool bWantElementalist = Save->PlayerClassName == TEXT("Elementalist");
	const TSubclassOf<AROHPlayerCharacter> NewClass = bWantElementalist
		? TSubclassOf<AROHPlayerCharacter>(AROHElementalistCharacter::StaticClass())
		: TSubclassOf<AROHPlayerCharacter>(AROHWarriorCharacter::StaticClass());
	AROHPlayerCharacter* Player = GameMode->RespawnPlayerAs(PC, NewClass);
	if (!Player)
	{
		return nullptr;
	}

	Player->GetProgression()->RestoreState(Save->Level, Save->XP, Save->StatPoints, Save->SkillPoints, Save->AllocatedStats);
	Player->GetSkillTree()->RestoreState(Save->SkillHardPoints);
	Player->GetInventory()->RestoreState(Save->InventoryItems, Save->EquippedItems, Save->Gold);

	UE_LOG(LogROH, Log, TEXT("로드 성공 (Lv %d, %s)"), Save->Level, *Save->PlayerClassName);
	return Player;
}
