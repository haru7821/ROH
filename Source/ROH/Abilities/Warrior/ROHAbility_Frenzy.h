#pragma once

#include "CoreMinimal.h"
#include "Abilities/ROHGameplayAbility.h"
#include "ROHAbility_Frenzy.generated.h"

/**
 * 광란 (b30, 전장의 함성 계열): 8초간 공격 속도 +20% (+랭크당 +5%) 자기 버프.
 * Duration GE(UROHFrenzyEffect — AttackSpeedPct Additive)로 적용, 만료 시 자동 원복.
 * 버프 수치는 랭크 기준 고정식 (BattleShout의 배수식과 달리 스펙 명시 수치 — 시너지 없음).
 */
UCLASS()
class ROH_API UROHAbility_Frenzy : public UROHGameplayAbility
{
	GENERATED_BODY()

public:
	UROHAbility_Frenzy();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 랭크 1 기준 공격 속도 % 증가 */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float BaseAttackSpeedBonus = 20.f;

	/** 랭크당 추가 증가 (랭크 1 초과분) */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float BonusPerRank = 5.f;
};
