#include "Abilities/Warrior/ROHAbility_BattleShout.h"
#include "Abilities/ROHGameplayEffects.h"
#include "Character/ROHCharacterBase.h"
#include "Character/ROHAttributeSet.h"
#include "ROHGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"

UROHAbility_BattleShout::UROHAbility_BattleShout()
{
	SkillId = TEXT("BattleShout");
	CostAttribute = UROHAttributeSet::GetRageAttribute();
	CostAmount = 20.f;
	CooldownDuration = 15.f;
	CooldownTags.AddTag(ROHGameplayTags::Cooldown_Skill_BattleShout);
	HitStopSeconds = 0.f;
	SpeedScaling = EROHSpeedScaling::Attack; // 공격 속도 스케일 (docs/10 §3.4)
}

void UROHAbility_BattleShout::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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
	UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	if (Character && ASC)
	{
		const float Multiplier = GetSkillDamageMultiplier();
		const float AttackPowerBonus = BaseAttackPowerBonus * Multiplier;
		const float DefenseBonus = BaseDefenseBonus * Multiplier;

		// 재시전 시 기존 버프 제거 후 새로 적용 (지속시간/수치 갱신, 중첩 방지)
		ASC->RemoveActiveGameplayEffectBySourceEffect(UROHBattleShoutEffect::StaticClass(), nullptr);

		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(UROHBattleShoutEffect::StaticClass(), GetAbilityLevel());
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(ROHGameplayTags::Data_BuffAttackPower, AttackPowerBonus);
			SpecHandle.Data->SetSetByCallerMagnitude(ROHGameplayTags::Data_BuffDefense, DefenseBonus);
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
		}

		// 그레이박스 연출: 함성 파동 + 상태 메시지
		DebugDrawSwing(Character->GetActorLocation(), 300.f);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(3, 3.f, FColor::Yellow,
				FString::Printf(TEXT("전투의 함성! 공격력 +%.0f, 방어 +%.0f (%.0f초)"),
					AttackPowerBonus, DefenseBonus, UROHBattleShoutEffect::Duration));
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
