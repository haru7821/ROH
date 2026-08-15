#include "Abilities/Warrior/ROHAbility_Frenzy.h"
#include "Abilities/ROHGameplayEffects.h"
#include "Character/ROHCharacterBase.h"
#include "Character/ROHAttributeSet.h"
#include "Progression/ROHSkillTreeComponent.h"
#include "ROHGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"

UROHAbility_Frenzy::UROHAbility_Frenzy()
{
	SkillId = TEXT("Frenzy");
	CostAttribute = UROHAttributeSet::GetRageAttribute();
	CostAmount = 20.f;
	CooldownDuration = 12.f; // 지속 8초 — 상시 유지 불가 간극
	CooldownTags.AddTag(ROHGameplayTags::Cooldown_Skill_Frenzy);
	HitStopSeconds = 0.f;
	SpeedScaling = EROHSpeedScaling::Attack; // 공격 속도 스케일 (docs/10 §3.4)
}

void UROHAbility_Frenzy::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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
		// 수치는 랭크 기준: 20% + (랭크-1)×5% (스펙 명시 — b30)
		const UROHSkillTreeComponent* SkillTree = Character->FindComponentByClass<UROHSkillTreeComponent>();
		const int32 Rank = SkillTree ? SkillTree->GetRank(SkillId) : 1;
		const float AttackSpeedBonus = BaseAttackSpeedBonus + BonusPerRank * FMath::Max(0, Rank - 1);

		// 재시전 시 기존 버프 제거 후 새로 적용 (지속시간/수치 갱신, 중첩 방지 — BattleShout 관례)
		ASC->RemoveActiveGameplayEffectBySourceEffect(UROHFrenzyEffect::StaticClass(), nullptr);

		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(UROHFrenzyEffect::StaticClass(), GetAbilityLevel());
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(ROHGameplayTags::Data_BuffAttackSpeed, AttackSpeedBonus);
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
		}

		// 그레이박스 연출: 상태 메시지 (버프 중 공격 어빌리티 쿨다운은 %AS 경유로 자동 단축)
		DebugDrawSwing(Character->GetActorLocation(), 220.f);
		if (GEngine)
		{
			// 키 7: BattleShout(키 3)와 동시 사용 시 문구가 덮이지 않게 분리 (Sup b30 지적)
		GEngine->AddOnScreenDebugMessage(7, 3.f, FColor::Red,
				FString::Printf(TEXT("광란! 공격 속도 +%.0f%% (%.0f초)"),
					AttackSpeedBonus, UROHFrenzyEffect::Duration));
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
