#pragma once

#include "CoreMinimal.h"
#include "Abilities/ROHGameplayAbility.h"
#include "ROHAbility_BattleShout.generated.h"

/**
 * 전투의 함성: 15초간 공격력/방어 자기 버프 (docs/02 전장의 함성 계열).
 * 증가량은 랭크·시너지 배수(GetSkillDamageMultiplier)로 커진다.
 */
UCLASS()
class ROH_API UROHAbility_BattleShout : public UROHGameplayAbility
{
	GENERATED_BODY()

public:
	UROHAbility_BattleShout();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float BaseAttackPowerBonus = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float BaseDefenseBonus = 15.f;
};
