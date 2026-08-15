#include "Save/ROHSaveSubsystem.h"
#include "Save/ROHSaveGame.h"
#include "Character/ROHPlayerCharacter.h"
#include "Character/ROHPlayerClasses.h"
#include "Items/ROHInventoryComponent.h"
#include "Items/ROHItemDatabase.h"
#include "Progression/ROHProgressionComponent.h"
#include "Progression/ROHSkillTreeComponent.h"
#include "Core/ROHGameMode.h"
#include "Campaign/ROHCampaignSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
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
	Save->BoundSkillSlots = Player->ExportBoundSkills();
	Player->GetInventory()->ExportState(Save->InventoryItems, Save->EquippedItems, Save->Gold);

	if (const UROHCampaignSubsystem* Campaign = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UROHCampaignSubsystem>() : nullptr)
	{
		Save->Difficulty = static_cast<uint8>(Campaign->GetDifficulty());
		Save->QuestStage = Campaign->GetQuestStage();
		Save->QuestKills = Campaign->GetKillCount();
		Save->ActivatedWaypoints = Campaign->GetActivatedWaypointsSorted();
		Save->bBaltarKilledEarly = Campaign->WasBaltarKilledEarly();
		Save->bMorgathKilledEarly = Campaign->WasMorgathKilledEarly();
	}

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
			// 공용 헬퍼 (계정 스태시 로드와 동일 규칙 — UROHAccountSubsystem 참조)
			for (FROHItemInstance& Item : Save->InventoryItems)
			{
				Database->RefreshItemAffixes(Item);
			}
			for (auto& Pair : Save->EquippedItems)
			{
				Database->RefreshItemAffixes(Pair.Value);
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

	// 캠페인 상태는 리스폰 직전에 복원 — 새 폰의 빙의 시점에 난이도 페널티가 반영되도록
	// (실패 조기 반환들 이후에 두어, 로드 실패 시 캠페인 상태만 바뀌는 것 방지)
	if (UROHCampaignSubsystem* Campaign = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UROHCampaignSubsystem>() : nullptr)
	{
		Campaign->RestoreState(
			static_cast<EROHDifficulty>(FMath::Clamp<int32>(Save->Difficulty, 0, 2)),
			Save->QuestStage, Save->QuestKills);
		Campaign->RestoreWaypoints(Save->ActivatedWaypoints);
		Campaign->RestoreEarlyBossKills(Save->bBaltarKilledEarly, Save->bMorgathKilledEarly);
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
	Player->RestoreBoundSkills(Save->BoundSkillSlots); // 스킬트리 복원 후 (랭크 검증 필요)

	UE_LOG(LogROH, Log, TEXT("로드 성공 (Lv %d, %s)"), Save->Level, *Save->PlayerClassName);
	return Player;
}
