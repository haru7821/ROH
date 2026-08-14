#pragma once

#include "CoreMinimal.h"
#include "Abilities/ROHGameplayAbility.h"
#include "ROHAbility_BasicAttack.generated.h"

/**
 * 전사 기본 공격: 전방 원뿔 근접 타격. 적중 시 분노 획득 (docs/02 §1.1).
 */
UCLASS()
class ROH_API UROHAbility_BasicAttack : public UROHGameplayAbility
{
	GENERATED_BODY()

public:
	UROHAbility_BasicAttack();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Attack")
	float BaseDamage = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Attack")
	float Range = 220.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Attack")
	float ConeHalfAngle = 60.f;

	/** 적중한 대상 1체당 획득 분노 */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Attack")
	float RageGainPerHit = 10.f;
};
