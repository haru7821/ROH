#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ROHProgressionComponent.generated.h"

/**
 * 경험치/레벨/스탯 포인트 (docs/02 §1.3, §2.1)
 * - 레벨업: 스탯 포인트 +5, 스킬 포인트 +1, 완전 회복
 * - 스탯 분배는 어트리뷰트 베이스에 직접 반영 + 파생 스탯 갱신
 * - 최대 레벨 50 — 이후 획득 XP는 계정 정복자(UROHAccountSubsystem)로 라우팅
 */
UCLASS(ClassGroup = (ROH), meta = (BlueprintSpawnableComponent))
class ROH_API UROHProgressionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	static constexpr int32 MaxLevel = 50;

	void GrantXP(int32 Amount);

	/** Strength/Dexterity/Vitality/Energy 에 1포인트 분배 */
	bool AllocateStat(FName StatName);

	int32 GetLevel() const { return Level; }
	int32 GetXP() const { return XP; }
	int32 GetStatPoints() const { return StatPoints; }
	int32 GetSkillPoints() const { return SkillPoints; }
	bool SpendSkillPoint();
	void RefundSkillPoint() { ++SkillPoints; }

	/** 레벨 L에서 다음 레벨까지 필요한 경험치 */
	static int32 XPForNextLevel(int32 InLevel);

	// --- 세이브/로드용 상태 접근 ---
	const TMap<FName, int32>& GetAllocatedStats() const { return AllocatedStats; }
	void RestoreState(int32 InLevel, int32 InXP, int32 InStatPoints, int32 InSkillPoints, const TMap<FName, int32>& InAllocated);

private:
	void LevelUp();
	void ApplyStatToAttributes(FName StatName, int32 Points) const;
	class UROHAttributeSet* GetAttributeSet() const;

	/** 만렙 잉여 경험치 → 계정 정복자 (M5 최종, docs/02) */
	void RouteToParagon(int32 Amount) const;

	int32 Level = 1;
	int32 XP = 0;
	int32 StatPoints = 0;
	int32 SkillPoints = 1; // 시작 시 1 — 첫 스킬을 바로 찍을 수 있게

	/** 분배 내역 (세이브/로드 시 재적용) */
	TMap<FName, int32> AllocatedStats;
};
