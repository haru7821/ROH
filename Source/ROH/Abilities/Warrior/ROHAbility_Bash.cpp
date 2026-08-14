#include "Abilities/Warrior/ROHAbility_Bash.h"
#include "Character/ROHCharacterBase.h"
#include "Character/ROHAttributeSet.h"
#include "Combat/ROHCombatStatics.h"
#include "ROHGameplayTags.h"

UROHAbility_Bash::UROHAbility_Bash()
{
	SkillId = TEXT("Bash");
	CostAttribute = UROHAttributeSet::GetRageAttribute();
	CostAmount = 15.f;
	CooldownDuration = 4.f;
	CooldownTags.AddTag(ROHGameplayTags::Cooldown_Skill_Bash);
	HitStopSeconds = 0.08f; // 단일 대상 강타는 히트스톱을 더 묵직하게
}

void UROHAbility_Bash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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

	FaceLocation(GetCursorLocation());
	DebugDrawSwing(Character->GetActorLocation() + Character->GetActorForwardVector() * Range * 0.5f, Range * 0.5f);

	// 가장 가까운 단일 대상
	AROHCharacterBase* BestTarget = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (AROHCharacterBase* Candidate : UROHCombatStatics::GetHostileTargetsInCone(Character, Range, 45.f))
	{
		const float DistSq = FVector::DistSquared(Character->GetActorLocation(), Candidate->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = Candidate;
		}
	}

	if (BestTarget)
	{
		const float Strength = Character->GetAttributeSet()->GetStrength();
		const float AttackPower = Character->GetAttributeSet()->GetAttackPower();
		FROHDamageParams Damage;
		Damage.PhysicalDamage = (BaseDamage + AttackPower) * (1.f + Strength * 0.01f) * GetSkillDamageMultiplier();
		Damage.bUseAttackRoll = true;

		if (UROHCombatStatics::ApplyDamage(Character, BestTarget, Damage))
		{
			UROHCombatStatics::ApplyKnockback(Character, BestTarget, KnockbackStrength);
			PlayHitFeedback(BestTarget);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
