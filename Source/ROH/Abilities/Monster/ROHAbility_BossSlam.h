#pragma once

#include "CoreMinimal.h"
#include "Abilities/ROHGameplayAbility.h"
#include "ROHAbility_BossSlam.generated.h"

/**
 * 보스 내려찍기: 예고 표시 후 시전 위치 반경에 광역 피해 + 넉백.
 * 예고 시간 동안 밖으로 빠져나가면 회피 가능 (명중 굴림 없음).
 * 발동 주기는 보스가 관리한다 (AROHBossCharacter::SlamInterval).
 */
UCLASS()
class ROH_API UROHAbility_BossSlam : public UROHGameplayAbility
{
	GENERATED_BODY()

public:
	UROHAbility_BossSlam();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 예고(장판 표시) 후 폭발까지 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float TelegraphTime = 0.9f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float SlamRadius = 350.f;

	/** 보스 평타 대비 피해 배수 */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float DamageMultiplier = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float KnockbackStrength = 700.f;
};
