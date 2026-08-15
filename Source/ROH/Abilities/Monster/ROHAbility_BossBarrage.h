#pragma once

#include "CoreMinimal.h"
#include "Abilities/ROHGameplayAbility.h"
#include "ROHAbility_BossBarrage.generated.h"

/**
 * 모르가스 특수 패턴: 플레이어 방향 화염탄 3연발(부채꼴) + 하수인(졸개) 소환.
 * 투사체는 피해서 회피 가능. 하수인은 주변 졸개 수 상한까지만 소환.
 */
UCLASS()
class ROH_API UROHAbility_BossBarrage : public UROHGameplayAbility
{
	GENERATED_BODY()

public:
	UROHAbility_BossBarrage();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 보스 평타 대비 발당 피해 배수 */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float DamageMultiplier = 1.2f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float ProjectileSpeed = 1000.f;

	/** 부채꼴 연발의 좌우 편차 각도 */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float FanHalfAngle = 18.f;

	/** 소환 하수인 수 (보스 주변 졸개가 상한 미만일 때만) */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	int32 SummonCount = 2;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	int32 MaxNearbyMinions = 6;
};
