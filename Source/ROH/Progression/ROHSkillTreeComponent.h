#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttributeSet.h"
#include "GameplayEffectTypes.h"
#include "Templates/SubclassOf.h"
#include "ROHSkillTreeComponent.generated.h"

class UROHGameplayAbility;
class UAbilitySystemComponent;

UENUM(BlueprintType)
enum class EROHPlayerClass : uint8
{
	Warrior,
	Elementalist
};

UENUM(BlueprintType)
enum class EROHSkillKind : uint8
{
	Active,  // 슬롯에 배치해 발동 (ROHBindSkill)
	Passive  // 투자 즉시 스탯 보정 GE 적용
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

/** 패시브 스킬의 랭크당 어트리뷰트 보정 */
USTRUCT()
struct FROHPassiveBonus
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayAttribute Attribute;

	UPROPERTY()
	float PerRank = 0.f;
};

/** 스킬 정의 (C++ 레지스트리, DataTable 이관 가능 구조 — docs/02 §2.5) */
USTRUCT()
struct FROHSkillDef
{
	GENERATED_BODY()

	UPROPERTY()
	FName SkillId;

	UPROPERTY()
	FText DisplayName;

	/** 소속 계열 표시명 (무기술/전장의 함성/투지, 화염/냉기/번개 — docs/02 §1) */
	UPROPERTY()
	FText TreeName;

	UPROPERTY()
	EROHPlayerClass PlayerClass = EROHPlayerClass::Warrior;

	UPROPERTY()
	EROHSkillKind Kind = EROHSkillKind::Active;

	UPROPERTY()
	int32 RequiredLevel = 1;

	/** 선행 스킬 (최소 1포인트 필요). None이면 없음 */
	UPROPERTY()
	FName PrereqSkillId;

	UPROPERTY()
	int32 MaxPoints = 20;

	UPROPERTY()
	TArray<FROHSkillSynergy> Synergies;

	/** 액티브 스킬의 발동 어빌리티 (슬롯 배치용). 패시브는 None */
	UPROPERTY()
	TSubclassOf<UROHGameplayAbility> AbilityClass;

	/** 패시브 스킬의 랭크당 보정 목록 */
	UPROPERTY()
	TArray<FROHPassiveBonus> PassiveBonuses;
};

/**
 * 스킬트리: 하드 포인트 투자/검증 + 피해 배수 계산 + 패시브 보정 적용 (docs/02 §2)
 * 랭크 보너스: 랭크당 +12%, 시너지: 해당 스킬 하드 포인트당 +8% (기본값)
 */
UCLASS(ClassGroup = (ROH), meta = (BlueprintSpawnableComponent))
class ROH_API UROHSkillTreeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** 클래스별 스킬 정의 목록 */
	static const TArray<FROHSkillDef>& GetSkillDefs(EROHPlayerClass InPlayerClass);
	static const FROHSkillDef* FindSkillDef(EROHPlayerClass InPlayerClass, FName SkillId);

	void SetPlayerClass(EROHPlayerClass InClass) { PlayerClass = InClass; }
	EROHPlayerClass GetPlayerClass() const { return PlayerClass; }

	/** 스킬 포인트 1 투자. 실패 사유는 OutError로 */
	bool InvestPoint(FName SkillId, FString& OutError);

	int32 GetRank(FName SkillId) const;

	/** 랭크 + 시너지를 합친 피해 배수 (랭크 0이면 0 반환 → 미습득) */
	float GetDamageMultiplier(FName SkillId) const;

	/**
	 * 리스펙: 모든 하드 포인트를 환불하고 패시브 효과를 제거한다. 환불된 포인트 수 반환.
	 * (그레이박스: 무제한. 정식 규칙은 난이도당 1회 — docs/02 §1.3)
	 */
	int32 ResetAllPoints();

	const TMap<FName, int32>& GetHardPoints() const { return HardPoints; }
	void RestoreState(const TMap<FName, int32>& InHardPoints);

	/** 변경 버전 (UI 3차 — b31): 투자/리스펙/복원/슬롯 배치 시 증가. 열린 창의 dirty 폴링용 */
	int32 GetChangeSerial() const { return ChangeSerial; }

	/** 외부 변이 통지 (슬롯 배치 등 — AROHPlayerCharacter::BindSkillToSlot이 호출, b31) */
	void BumpChangeSerial() { ++ChangeSerial; }

private:
	class UROHProgressionComponent* GetProgression() const;
	UAbilitySystemComponent* GetOwnerASC() const;

	/** 패시브 스킬의 보정 GE를 현재 랭크 기준으로 재적용 */
	void RefreshPassiveEffect(const FROHSkillDef& Def);
	void RemoveAllPassiveEffects();

	UPROPERTY()
	EROHPlayerClass PlayerClass = EROHPlayerClass::Warrior;

	/** SkillId → 하드 포인트 */
	TMap<FName, int32> HardPoints;

	/** 패시브 SkillId → 적용 중인 보정 GE 핸들 */
	TMap<FName, FActiveGameplayEffectHandle> PassiveEffectHandles;

	/** 변경 버전 (b31 — dirty 폴링) */
	int32 ChangeSerial = 0;
};
