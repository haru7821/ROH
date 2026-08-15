#include "Abilities/Warrior/ROHAbility_Execute.h"
#include "Character/ROHCharacterBase.h"
#include "Character/ROHAttributeSet.h"
#include "Combat/ROHCombatStatics.h"
#include "ROHGameplayTags.h"

UROHAbility_Execute::UROHAbility_Execute()
{
	SkillId = TEXT("Execute");
	CostAttribute = UROHAttributeSet::GetRageAttribute();
	CostAmount = 30.f;
	CooldownDuration = 10.f;
	CooldownTags.AddTag(ROHGameplayTags::Cooldown_Skill_Execute);
	HitStopSeconds = 0.1f; // 처형은 가장 묵직한 히트스톱
	SpeedScaling = EROHSpeedScaling::Attack; // 공격 속도 스케일 (docs/10 §3.4)
}

void UROHAbility_Execute::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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
		// 힘/% 배율은 파이프라인이 적용 (docs/10 §3.1, b29 이관)
		const float AttackPower = Character->GetAttributeSet()->GetAttackPower();
		FROHDamageParams Damage;
		Damage.PhysicalDamage = (BaseDamage + AttackPower) * GetSkillDamageMultiplier();
		Damage.bUseAttackRoll = true;

		// 빈사 대상 처형 보너스
		const UROHAttributeSet* TargetAttributes = BestTarget->GetAttributeSet();
		if (TargetAttributes && TargetAttributes->GetMaxHealth() > 0.f
			&& TargetAttributes->GetHealth() <= TargetAttributes->GetMaxHealth() * ExecuteHealthRatio)
		{
			Damage.PhysicalDamage *= 2.f;
		}

		if (UROHCombatStatics::ApplyDamage(Character, BestTarget, Damage))
		{
			PlayHitFeedback(BestTarget);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
