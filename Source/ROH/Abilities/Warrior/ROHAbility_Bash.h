#pragma once

#include "CoreMinimal.h"
#include "Abilities/ROHGameplayAbility.h"
#include "ROHAbility_Bash.generated.h"

/**
 * 강타: 전방 단일 대상에게 큰 물리 피해 + 넉백. 분노 소모 (docs/02 무기술 계열).
 */
UCLASS()
class ROH_API UROHAbility_Bash : public UROHGameplayAbility
{
	GENERATED_BODY()

public:
	UROHAbility_Bash();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float BaseDamage = 45.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float Range = 250.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float KnockbackStrength = 700.f;
};
