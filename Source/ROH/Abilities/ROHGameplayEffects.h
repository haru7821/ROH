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

/** 둔화: 3초간 이동속도 -250 (서리 신성 등 냉기 효과) */
UCLASS()
class ROH_API UROHSlowEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UROHSlowEffect();
};

/**
 * 전투의 함성 버프: 15초간 공격력/방어 증가.
 * 증가량은 SetByCaller(Data.Buff.AttackPower / Data.Buff.Defense)로 전달 (랭크·시너지 배수 반영).
 * 재시전 시 어빌리티가 기존 효과를 제거 후 새로 적용한다 (중첩 방지).
 */
UCLASS()
class ROH_API UROHBattleShoutEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UROHBattleShoutEffect();

	static constexpr float Duration = 15.f;
};

/**
 * 광란 버프 (b30): 8초간 공격 속도 % 증가 (AttackSpeedPct Additive — 만료 시 자동 원복).
 * 증가량은 SetByCaller(Data.Buff.AttackSpeed)로 전달 (기본 20% + 랭크당 5%).
 * 재시전 시 어빌리티가 기존 효과를 제거 후 새로 적용한다 (BattleShout과 동일 중첩 방지 패턴).
 * 버프 중 발동한 공격 어빌리티의 쿨다운은 ApplyCooldown의 %AS 항으로 자연 단축된다 (docs/10 §3.4).
 */
UCLASS()
class ROH_API UROHFrenzyEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UROHFrenzyEffect();

	static constexpr float Duration = 8.f;
};
