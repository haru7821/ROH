#include "Abilities/ROHGameplayEffects.h"
#include "Character/ROHAttributeSet.h"
#include "ROHGameplayTags.h"

UROHDamageEffect::UROHDamageEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo DamageModifier;
	DamageModifier.Attribute = UROHAttributeSet::GetIncomingDamageAttribute();
	DamageModifier.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = ROHGameplayTags::Data_Damage;
	DamageModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Modifiers.Add(DamageModifier);
}

UROHCooldownEffect::UROHCooldownEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = ROHGameplayTags::Data_Cooldown;
	DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
}

UROHSlowEffect::UROHSlowEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.f));

	// 재적용 시 지속시간 갱신 (중첩 -500 둔화 방지)
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;

	FGameplayModifierInfo SlowModifier;
	SlowModifier.Attribute = UROHAttributeSet::GetMoveSpeedAttribute();
	SlowModifier.ModifierOp = EGameplayModOp::Additive;
	SlowModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-250.f));
	Modifiers.Add(SlowModifier);
}
