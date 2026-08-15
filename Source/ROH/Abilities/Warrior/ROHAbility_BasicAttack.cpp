#include "Abilities/Warrior/ROHAbility_BasicAttack.h"
#include "Character/ROHCharacterBase.h"
#include "Character/ROHAttributeSet.h"
#include "Combat/ROHCombatStatics.h"
#include "ROHGameplayTags.h"
#include "AbilitySystemComponent.h"

UROHAbility_BasicAttack::UROHAbility_BasicAttack()
{
	CooldownDuration = 0.5f; // 기본 공격 주기 — 공속 스케일은 SpeedScaling이 적용
	CooldownTags.AddTag(ROHGameplayTags::Cooldown_BasicAttack);
	SpeedScaling = EROHSpeedScaling::Attack; // 공격 속도 스케일 (docs/10 §3.4)
}

void UROHAbility_BasicAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AROHCharacterBase* Character = GetROHCharacter();
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FaceLocation(GetCursorLocation());
	DebugDrawSwing(Character->GetActorLocation() + Character->GetActorForwardVector() * Range * 0.5f, Range * 0.5f);

	// 원피해 = 기본 피해 + 무기 공격력(FlatAP) — 힘/% 배율은 파이프라인이 적용 (docs/10 §3.1, b29 이관)
	const float AttackPower = Character->GetAttributeSet()->GetAttackPower();
	FROHDamageParams Damage;
	Damage.PhysicalDamage = BaseDamage + AttackPower;
	Damage.bUseAttackRoll = true;

	int32 HitCount = 0;
	for (AROHCharacterBase* Target : UROHCombatStatics::GetHostileTargetsInCone(Character, Range, ConeHalfAngle))
	{
		if (UROHCombatStatics::ApplyDamage(Character, Target, Damage))
		{
			++HitCount;
			PlayHitFeedback(Target);
		}
	}

	if (HitCount > 0 && RageGainPerHit > 0.f)
	{
		Character->GetAbilitySystemComponent()->ApplyModToAttribute(
			UROHAttributeSet::GetRageAttribute(), EGameplayModOp::Additive, RageGainPerHit * HitCount);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
