#pragma once

#include "CoreMinimal.h"
#include "Abilities/ROHGameplayAbility.h"
#include "ROHAbility_MonsterAttack.generated.h"

class AROHProjectile;

/**
 * 몬스터 근접 공격: 전방 원뿔 물리 피해.
 * 피해량/사거리는 소유 몬스터(AROHMonsterCharacter)의 스탯을 읽는다.
 */
UCLASS()
class ROH_API UROHAbility_MonsterMelee : public UROHGameplayAbility
{
	GENERATED_BODY()

public:
	UROHAbility_MonsterMelee();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

/**
 * 몬스터 원거리 공격: 플레이어를 향해 투사체 발사.
 */
UCLASS()
class ROH_API UROHAbility_MonsterRanged : public UROHGameplayAbility
{
	GENERATED_BODY()

public:
	UROHAbility_MonsterRanged();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	TSubclassOf<AROHProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float ProjectileSpeed = 900.f;
};
