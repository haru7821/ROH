#pragma once

#include "CoreMinimal.h"
#include "Abilities/ROHGameplayAbility.h"
#include "ROHElementalistAbilities.generated.h"

class AROHProjectile;

/** 원소술사 기본 공격: 번개 화살 투사체. 마나 무소모 (docs/02 §1.2) */
UCLASS()
class ROH_API UROHAbility_MagicBolt : public UROHGameplayAbility
{
	GENERATED_BODY()

public:
	UROHAbility_MagicBolt();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float BaseDamage = 12.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float ProjectileSpeed = 1400.f;
};

/** 화염구: 폭발하는 화염 투사체 */
UCLASS()
class ROH_API UROHAbility_Fireball : public UROHGameplayAbility
{
	GENERATED_BODY()

public:
	UROHAbility_Fireball();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float BaseDamage = 28.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float ProjectileSpeed = 1100.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float ExplosionRadius = 250.f;
};

/** 서리 신성: 자기 주변 냉기 폭발 + 둔화 */
UCLASS()
class ROH_API UROHAbility_FrostNova : public UROHGameplayAbility
{
	GENERATED_BODY()

public:
	UROHAbility_FrostNova();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float BaseDamage = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float Radius = 350.f;
};

/** 텔레포트: 커서 지점으로 순간이동 */
UCLASS()
class ROH_API UROHAbility_Teleport : public UROHGameplayAbility
{
	GENERATED_BODY()

public:
	UROHAbility_Teleport();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float MaxDistance = 1200.f;
};
