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

/** 얼음화살: 저비용 단일 대상 냉기 투사체 (냉기 계열 1티어) */
UCLASS()
class ROH_API UROHAbility_IceBolt : public UROHGameplayAbility
{
	GENERATED_BODY()

public:
	UROHAbility_IceBolt();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float BaseDamage = 16.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float ProjectileSpeed = 1500.f;
};

/** 운석: 커서 지점에 1초 후 낙하하는 대형 화염 폭발 (화염 계열 상위기) */
UCLASS()
class ROH_API UROHAbility_Meteor : public UROHGameplayAbility
{
	GENERATED_BODY()

public:
	UROHAbility_Meteor();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float BaseDamage = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float ImpactRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float ImpactDelay = 1.f;
};

/** 눈보라: 커서 지점 반경에 3회 냉기 파동 + 둔화 (냉기 계열 상위기) */
UCLASS()
class ROH_API UROHAbility_Blizzard : public UROHGameplayAbility
{
	GENERATED_BODY()

public:
	UROHAbility_Blizzard();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float DamagePerPulse = 18.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float Radius = 350.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	int32 PulseCount = 3;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float PulseInterval = 0.7f;
};

/**
 * 정전기장: 주변 적의 현재 생명력 비례 번개 피해 (디아블로2 스태틱 필드 방식).
 * 체력이 많은 적일수록 강력 — 보스전 오프너.
 */
UCLASS()
class ROH_API UROHAbility_StaticField : public UROHGameplayAbility
{
	GENERATED_BODY()

public:
	UROHAbility_StaticField();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float Radius = 400.f;

	/** 현재 생명력 대비 기본 피해 비율 (랭크 배수 적용, 최대 50%) */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float BaseHealthPercent = 0.2f;
};

/** 불꽃 파도 (b30): 커서 방향 전방 부채꼴 화염 피해 (화염 계열 중위기) */
UCLASS()
class ROH_API UROHAbility_FlameWave : public UROHGameplayAbility
{
	GENERATED_BODY()

public:
	UROHAbility_FlameWave();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float BaseDamage = 32.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float Range = 600.f;

	/** 부채꼴 반각 (도) */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float ConeHalfAngle = 45.f;
};

/**
 * 연쇄 번개 (b30, 번개 계열 상위기): 커서 방향 첫 대상 명중 후 근접한 다음 적으로
 * 최대 3회 연쇄 (같은 적 재타격 금지), 연쇄마다 피해 ×0.7.
 */
UCLASS()
class ROH_API UROHAbility_ChainLightning : public UROHGameplayAbility
{
	GENERATED_BODY()

public:
	UROHAbility_ChainLightning();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float BaseDamage = 30.f;

	/** 첫 대상 탐색 사거리 (커서 방향 부채꼴) */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float CastRange = 800.f;

	/** 연쇄 탐색 반경 (직전 피격자 기준) */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float ChainRadius = 500.f;

	/** 첫 대상 이후 최대 연쇄 횟수 */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	int32 MaxChains = 3;

	/** 연쇄당 피해 감쇄 배율 */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float ChainDamageFalloff = 0.7f;
};
