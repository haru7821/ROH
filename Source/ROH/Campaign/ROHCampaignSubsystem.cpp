#include "Campaign/ROHCampaignSubsystem.h"
#include "Character/ROHMonsterCharacter.h"
#include "Character/ROHBossCharacter.h"
#include "Character/ROHPlayerCharacter.h"
#include "Items/ROHInventoryComponent.h"
#include "Progression/ROHProgressionComponent.h"
#include "Engine/Engine.h"
#include "ROH.h"

FROHDifficultyParams UROHCampaignSubsystem::GetDifficultyParams(EROHDifficulty InDifficulty)
{
	// 수치 근거: docs/04 M4 (몬스터 스케일링/저항 페널티/ilvl 상승), 디아블로2 벤치마크
	FROHDifficultyParams Params;
	switch (InDifficulty)
	{
	case EROHDifficulty::Nightmare:
		Params.HealthMult = 2.5f;
		Params.DamageMult = 1.8f;
		Params.XPMult = 2.f;
		Params.AttackRatingMult = 2.f;
		Params.DefenseMult = 1.6f;
		Params.MonsterLevelBonus = 15.f;
		Params.PlayerResistPenalty = -25.f; // docs/10 §4.2
		break;
	case EROHDifficulty::Hell:
		Params.HealthMult = 6.f;
		Params.DamageMult = 3.2f;
		Params.XPMult = 4.f;
		Params.AttackRatingMult = 3.5f;
		Params.DefenseMult = 2.5f;
		Params.MonsterLevelBonus = 30.f;
		Params.PlayerResistPenalty = -50.f; // docs/10 §4.2
		break;
	default:
		break;
	}
	return Params;
}

FString UROHCampaignSubsystem::GetDifficultyDisplayName(EROHDifficulty InDifficulty)
{
	switch (InDifficulty)
	{
	case EROHDifficulty::Nightmare: return TEXT("악몽");
	case EROHDifficulty::Hell:      return TEXT("지옥");
	default:                        return TEXT("노말");
	}
}

void UROHCampaignSubsystem::NotifyMonsterKilled(AROHMonsterCharacter* DeadMonster, AActor* Killer)
{
	// 플레이어가 직접 처치한 것만 퀘스트 집계
	if (!DeadMonster || !Cast<AROHPlayerCharacter>(Killer))
	{
		return;
	}

	const bool bIsMorgath = DeadMonster->IsA<AROHBossMorgath>();
	const bool bIsBaltar = !bIsMorgath && DeadMonster->IsA<AROHBossCharacter>();

	// 보스를 퀘스트 단계보다 먼저 잡은 경우: 기록해 두고 단계 도달 시 자동 정산
	// (지역에 보스가 미리 배치되므로 순서를 건너뛰어도 소프트락이 없어야 한다)
	if (bIsBaltar && QuestStage < 1)
	{
		bBaltarKilledEarly = true;
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Green,
				TEXT("발타르 처치 확인 — 들판 정리를 마치면 퀘스트가 정산됩니다"));
		}
	}
	if (bIsMorgath && QuestStage < 2)
	{
		bMorgathKilledEarly = true;
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Green,
				TEXT("모르가스 처치 확인 — 이전 퀘스트를 마치면 정산됩니다"));
		}
	}

	switch (QuestStage)
	{
	case 0: // 잿빛 들판 정리: 일반 몬스터 10 처치
		if (!DeadMonster->IsA<AROHBossCharacter>())
		{
			++KillCount;
			if (KillCount >= FieldKillTarget)
			{
				QuestStage = 1;
				GrantStageReward(Killer, 1, 200,
					TEXT("퀘스트 완료: 잿빛 들판 정리! (+스킬P 1, +골드 200)  다음: 무너진 지하묘지의 발타르"));
			}
		}
		break;

	case 1: // 중간 보스 발타르
		if (bIsBaltar)
		{
			QuestStage = 2;
			GrantStageReward(Killer, 2, 500,
				TEXT("퀘스트 완료: 발타르 처치! (+스킬P 2, +골드 500)  다음: 재의 성소의 모르가스"));
		}
		break;

	case 2: // 액트 보스 모르가스
		if (bIsMorgath)
		{
			QuestStage = 3;
			GrantStageReward(Killer, 3, 1000,
				FString::Printf(TEXT("액트1 클리어! (+스킬P 3, +골드 1000)  %s 난이도 완료 — ROHSetDifficulty로 상위 난이도에 도전하세요"),
					*GetDifficultyDisplayName(Difficulty)));
		}
		break;

	default:
		break;
	}

	SettlePendingBossStages(Killer);
}

void UROHCampaignSubsystem::SettlePendingBossStages(AActor* Killer)
{
	if (QuestStage == 1 && bBaltarKilledEarly)
	{
		QuestStage = 2;
		GrantStageReward(Killer, 2, 500,
			TEXT("퀘스트 정산: 발타르 처치! (+스킬P 2, +골드 500)  다음: 재의 성소의 모르가스"));
	}
	if (QuestStage == 2 && bMorgathKilledEarly)
	{
		QuestStage = 3;
		GrantStageReward(Killer, 3, 1000,
			FString::Printf(TEXT("액트1 클리어! (+스킬P 3, +골드 1000)  %s 난이도 완료 — ROHSetDifficulty로 상위 난이도에 도전하세요"),
				*GetDifficultyDisplayName(Difficulty)));
	}
}

void UROHCampaignSubsystem::GrantStageReward(AActor* Killer, int32 SkillPoints, int32 Gold, const FString& Message)
{
	if (const AROHPlayerCharacter* Player = Cast<AROHPlayerCharacter>(Killer))
	{
		if (UROHProgressionComponent* Progression = Player->GetProgression())
		{
			for (int32 i = 0; i < SkillPoints; ++i)
			{
				Progression->RefundSkillPoint();
			}
		}
		if (UROHInventoryComponent* Inventory = Player->GetInventory())
		{
			Inventory->AddGold(Gold);
		}
	}

	UE_LOG(LogROH, Log, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, Message);
	}
}

FString UROHCampaignSubsystem::GetObjectiveText() const
{
	const FString Prefix = FString::Printf(TEXT("[%s] "), *GetDifficultyDisplayName(Difficulty));
	switch (QuestStage)
	{
	case 0:
		return Prefix + FString::Printf(TEXT("퀘스트: 잿빛 들판 정리 — 몬스터 처치 %d/%d"), KillCount, FieldKillTarget);
	case 1:
		return Prefix + TEXT("퀘스트: 무너진 지하묘지의 발타르 처치 (E: 웨이포인트 이동)");
	case 2:
		return Prefix + TEXT("퀘스트: 재의 성소의 모르가스 처치 (E: 웨이포인트 이동)");
	default:
		return Prefix + TEXT("액트1 클리어 — 상위 난이도 도전 가능 (ROHSetDifficulty)");
	}
}

void UROHCampaignSubsystem::RestoreState(EROHDifficulty InDifficulty, int32 InQuestStage, int32 InKillCount)
{
	Difficulty = InDifficulty;
	QuestStage = FMath::Clamp(InQuestStage, 0, 3);
	KillCount = FMath::Max(0, InKillCount);
}

void UROHCampaignSubsystem::RestoreEarlyBossKills(bool bInBaltar, bool bInMorgath)
{
	bBaltarKilledEarly = bInBaltar;
	bMorgathKilledEarly = bInMorgath;
}

bool UROHCampaignSubsystem::ActivateWaypoint(int32 ZoneIndex)
{
	bool bAlreadyActivated = false;
	ActivatedWaypoints.Add(ZoneIndex, &bAlreadyActivated);
	return !bAlreadyActivated;
}

TArray<int32> UROHCampaignSubsystem::GetActivatedWaypointsSorted() const
{
	TArray<int32> Result = ActivatedWaypoints.Array();
	Result.Sort();
	return Result;
}

void UROHCampaignSubsystem::RestoreWaypoints(const TArray<int32>& InActivated)
{
	ActivatedWaypoints.Reset();
	for (const int32 ZoneIndex : InActivated)
	{
		ActivatedWaypoints.Add(ZoneIndex);
	}
	// 마을은 무조건 활성 — 지역 매니저 BeginPlay는 로드 시점엔 이미 지나갔고, 구버전 세이브는 빈 배열
	ActivatedWaypoints.Add(0);
}
