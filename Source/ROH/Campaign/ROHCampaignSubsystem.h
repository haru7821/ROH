#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ROHCampaignSubsystem.generated.h"

class AROHMonsterCharacter;

/** 난이도 3단계 (docs/04 M4). 디아블로2 방식: 클리어 후 상위 난이도 재도전 */
UENUM(BlueprintType)
enum class EROHDifficulty : uint8
{
	Normal,
	Nightmare,
	Hell
};

/** 난이도별 배율 (몬스터 스폰 시 적용 + 플레이어 저항 페널티) */
struct FROHDifficultyParams
{
	float HealthMult = 1.f;
	float DamageMult = 1.f;
	float XPMult = 1.f;
	float AttackRatingMult = 1.f;
	float DefenseMult = 1.f;
	float MonsterLevelBonus = 0.f;   // 명중 공식 + 드랍 ilvl에 반영
	float PlayerResistPenalty = 0.f; // 음수로 적용 (악몽 -40 / 지옥 -100)
};

/**
 * 캠페인 상태: 난이도 + 액트1 메인 퀘스트 진행 (docs/04 M4, docs/06).
 * 퀘스트: 0 잿빛 들판 정리(처치 10) → 1 발타르 처치 → 2 모르가스 처치 → 3 클리어
 * 게임 인스턴스 수명 — 세이브/로드로 영속화.
 */
UCLASS()
class ROH_API UROHCampaignSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static constexpr int32 FieldKillTarget = 10;

	// --- 난이도 ---
	EROHDifficulty GetDifficulty() const { return Difficulty; }
	void SetDifficulty(EROHDifficulty InDifficulty) { Difficulty = InDifficulty; }
	static FROHDifficultyParams GetDifficultyParams(EROHDifficulty InDifficulty);
	static FString GetDifficultyDisplayName(EROHDifficulty InDifficulty);

	// --- 퀘스트 ---
	int32 GetQuestStage() const { return QuestStage; }
	int32 GetKillCount() const { return KillCount; }

	/** 몬스터 사망 훅 (AROHMonsterCharacter::HandleDeath에서 호출). 플레이어 처치만 집계 */
	void NotifyMonsterKilled(AROHMonsterCharacter* DeadMonster, AActor* Killer);

	/** HUD 표시용 현재 목표 (난이도 접두 포함) */
	FString GetObjectiveText() const;

	// --- 세이브/로드 ---
	void RestoreState(EROHDifficulty InDifficulty, int32 InQuestStage, int32 InKillCount);

private:
	void GrantStageReward(AActor* Killer, int32 SkillPoints, int32 Gold, const FString& Message);

	EROHDifficulty Difficulty = EROHDifficulty::Normal;
	int32 QuestStage = 0;
	int32 KillCount = 0;
};
