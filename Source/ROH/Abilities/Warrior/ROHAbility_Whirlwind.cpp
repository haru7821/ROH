#include "Abilities/Warrior/ROHAbility_Whirlwind.h"
#include "Character/ROHCharacterBase.h"
#include "Character/ROHAttributeSet.h"
#include "Combat/ROHCombatStatics.h"
#include "ROHGameplayTags.h"

UROHAbility_Whirlwind::UROHAbility_Whirlwind()
{
	SkillId = TEXT("Whirlwind");
	CostAttribute = UROHAttributeSet::GetRageAttribute();
	CostAmount = 25.f;
	CooldownDuration = 6.f;
	CooldownTags.AddTag(ROHGameplayTags::Cooldown_Skill_Whirlwind);
	SpeedScaling = EROHSpeedScaling::Attack; // 공격 속도 스케일 (docs/10 §3.4)
}

void UROHAbility_Whirlwind::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CheckSkillInvested())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
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

	DebugDrawSwing(Character->GetActorLocation(), Radius);

	const float Strength = Character->GetAttributeSet()->GetStrength();
	const float AttackPower = Character->GetAttributeSet()->GetAttackPower();
	FROHDamageParams Damage;
	Damage.PhysicalDamage = (BaseDamage + AttackPower) * (1.f + Strength * 0.01f) * GetSkillDamageMultiplier();
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
