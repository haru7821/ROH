#pragma once

#include "CoreMinimal.h"
#include "Abilities/ROHGameplayAbility.h"
#include "Engine/TimerHandle.h"
#include "ROHAbility_LeapAttack.generated.h"

/**
 * 도약공격: 커서 지점으로 도약, 착지 시 광역 피해 + 넉백. 분노 소모.
 */
UCLASS()
class ROH_API UROHAbility_LeapAttack : public UROHGameplayAbility
{
	GENERATED_BODY()

public:
	UROHAbility_LeapAttack();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION()
	void OnLanded(const FHitResult& Hit);

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float BaseDamage = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float MaxLeapDistance = 900.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float ImpactRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float KnockbackStrength = 500.f;

	/** 도약 체공 시간(초). 착지 속도/포물선 높이를 결정 */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float LeapTime = 0.5f;

private:
	void DoImpact();

	FTimerHandle SafetyTimerHandle;
	bool bImpactDone = false;
};
