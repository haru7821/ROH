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
