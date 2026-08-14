#include "Abilities/Warrior/ROHAbility_Whirlwind.h"
#include "Character/ROHCharacterBase.h"
#include "Character/ROHAttributeSet.h"
#include "Combat/ROHCombatStatics.h"
#include "ROHGameplayTags.h"

UROHAbility_Whirlwind::UROHAbility_Whirlwind()
{
	CostAttribute = UROHAttributeSet::GetRageAttribute();
	CostAmount = 25.f;
	CooldownDuration = 6.f;
	CooldownTags.AddTag(ROHGameplayTags::Cooldown_Skill_Whirlwind);
}

void UROHAbility_Whirlwind::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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

	const float Strength = Character->GetAttributeSet()->GetStrength();
	FROHDamageParams Damage;
	Damage.PhysicalDamage = BaseDamage * (1.f + Strength * 0.01f);
	Damage.bUseAttackRoll = true;

	for (AROHCharacterBase* Target : UROHCombatStatics::GetHostileTargetsInRadius(Character, Character->GetActorLocation(), Radius))
	{
		if (UROHCombatStatics::ApplyDamage(Character, Target, Damage))
		{
			PlayHitFeedback(Target);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
