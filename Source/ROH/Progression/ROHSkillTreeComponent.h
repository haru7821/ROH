#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ROHSkillTreeComponent.generated.h"

class UROHGameplayAbility;

UENUM(BlueprintType)
enum class EROHPlayerClass : uint8
{
	Warrior,
	Elementalist
};

/** 시너지: 다른 스킬의 하드 포인트가 이 스킬 피해를 강화 (docs/02 §2.3) */
USTRUCT()
struct FROHSkillSynergy
{
	GENERATED_BODY()

	UPROPERTY()
	FName SkillId;

	UPROPERTY()
	float PerPointPercent = 8.f;
};

/** 스킬 정의 (M3: C++ 레지스트리, DataTable 이관 가능 구조 — docs/02 §2.5) */
USTRUCT()
struct FROHSkillDef
{
	GENERATED_BODY()

	UPROPERTY()
	FName SkillId;

	UPROPERTY()
	FText DisplayName;

	UPROPERTY()
	EROHPlayerClass PlayerClass = EROHPlayerClass::Warrior;

	UPROPERTY()
	int32 RequiredLevel = 1;

	/** 선행 스킬 (최소 1포인트 필요). None이면 없음 */
	UPROPERTY()
	FName PrereqSkillId;

	UPROPERTY()
	int32 MaxPoints = 20;

	UPROPERTY()
	TArray<FROHSkillSynergy> Synergies;
};

/**
 * 스킬트리: 하드 포인트 투자/검증 + 피해 배수 계산 (docs/02 §2)
 * 랭크 보너스: 랭크당 +12%, 시너지: 해당 스킬 하드 포인트당 +8% (기본값)
 */
UCLASS(ClassGroup = (ROH), meta = (BlueprintSpawnableComponent))
class ROH_API UROHSkillTreeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** 클래스별 스킬 정의 목록 */
	static const TArray<FROHSkillDef>& GetSkillDefs(EROHPlayerClass PlayerClass);
	static const FROHSkillDef* FindSkillDef(EROHPlayerClass PlayerClass, FName SkillId);

	void SetPlayerClass(EROHPlayerClass InClass) { PlayerClass = InClass; }
	EROHPlayerClass GetPlayerClass() const { return PlayerClass; }

	/** 스킬 포인트 1 투자. 실패 사유는 OutError로 */
	bool InvestPoint(FName SkillId, FString& OutError);

	int32 GetRank(FName SkillId) const;

	/** 랭크 + 시너지를 합친 피해 배수 (랭크 0이면 0 반환 → 미습득) */
	float GetDamageMultiplier(FName SkillId) const;

	const TMap<FName, int32>& GetHardPoints() const { return HardPoints; }
	void RestoreState(const TMap<FName, int32>& InHardPoints) { HardPoints = InHardPoints; }

private:
	class UROHProgressionComponent* GetProgression() const;

	UPROPERTY()
	EROHPlayerClass PlayerClass = EROHPlayerClass::Warrior;

	/** SkillId → 하드 포인트 */
	TMap<FName, int32> HardPoints;
};
