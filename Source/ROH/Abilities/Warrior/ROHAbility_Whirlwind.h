#pragma once

#include "CoreMinimal.h"
#include "Abilities/ROHGameplayAbility.h"
#include "ROHAbility_Whirlwind.generated.h"

/**
 * 회전베기: 자신 주위 전방위 광역 물리 피해. 분노 소모.
 */
UCLASS()
class ROH_API UROHAbility_Whirlwind : public UROHGameplayAbility
{
	GENERATED_BODY()

public:
	UROHAbility_Whirlwind();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float BaseDamage = 22.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float Radius = 350.f;
};
