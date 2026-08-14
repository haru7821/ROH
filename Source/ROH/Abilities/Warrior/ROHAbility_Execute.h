#pragma once

#include "CoreMinimal.h"
#include "Abilities/ROHGameplayAbility.h"
#include "ROHAbility_Execute.generated.h"

/**
 * 처형: 전방 단일 대상에게 강력한 물리 피해.
 * 대상 생명력이 33% 이하면 피해 2배 (마무리 일격 — docs/02 무기술 계열).
 */
UCLASS()
class ROH_API UROHAbility_Execute : public UROHGameplayAbility
{
	GENERATED_BODY()

public:
	UROHAbility_Execute();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float BaseDamage = 70.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float Range = 250.f;

	/** 이 비율 이하 생명력의 대상에게 피해 2배 */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float ExecuteHealthRatio = 0.33f;
};
