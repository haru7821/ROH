#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ROHGameplayEffects.generated.h"

/**
 * 피해 적용용 인스턴트 GE.
 * SetByCaller(Data.Damage)로 전달된 최종 피해량을 IncomingDamage 메타 어트리뷰트에 더한다.
 * 실제 Health 반영은 UROHAttributeSet::PostGameplayEffectExecute에서 수행.
 */
UCLASS()
class ROH_API UROHDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UROHDamageEffect();
};

/**
 * 공용 쿨다운 GE.
 * SetByCaller(Data.Cooldown)로 지속시간을 받고, 어빌리티가 DynamicGrantedTags로
 * 자신의 쿨다운 태그를 부여한다 (UROHGameplayAbility::ApplyCooldown 참고).
 */
UCLASS()
class ROH_API UROHCooldownEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UROHCooldownEffect();
};
