#include "Abilities/Warrior/ROHAbility_LeapAttack.h"
#include "Character/ROHCharacterBase.h"
#include "Character/ROHAttributeSet.h"
#include "Combat/ROHCombatStatics.h"
#include "ROHGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UROHAbility_LeapAttack::UROHAbility_LeapAttack()
{
	SkillId = TEXT("Leap");
	CostAttribute = UROHAttributeSet::GetRageAttribute();
	CostAmount = 20.f;
	CooldownDuration = 8.f;
	CooldownTags.AddTag(ROHGameplayTags::Cooldown_Skill_Leap);
	SpeedScaling = EROHSpeedScaling::Attack; // 공격 속도 스케일 (docs/10 §3.4)
}

void UROHAbility_LeapAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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
	if (!Character || !Character->GetWorld())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bImpactDone = false;

	// 목표 지점: 커서 위치, 최대 사거리로 클램프
	const FVector Start = Character->GetActorLocation();
	FVector Target = GetCursorLocation();
	FVector Delta = Target - Start;
	Delta.Z = 0.f;
	if (Delta.SizeSquared() > FMath::Square(MaxLeapDistance))
	{
		Delta = Delta.GetSafeNormal() * MaxLeapDistance;
	}
	FaceLocation(Start + Delta);

	// 포물선 발사 속도: 수평 = 거리/시간, 수직 = 중력 보상
	const float Gravity = FMath::Abs(Character->GetWorld()->GetGravityZ());
	const FVector HorizontalVelocity = Delta / FMath::Max(LeapTime, 0.2f);
	const float VerticalVelocity = 0.5f * Gravity * FMath::Max(LeapTime, 0.2f);

	Character->LandedDelegate.AddDynamic(this, &UROHAbility_LeapAttack::OnLanded);
	Character->LaunchCharacter(HorizontalVelocity + FVector(0.f, 0.f, VerticalVelocity), true, true);

	// 착지 이벤트를 못 받는 경우 대비 안전 타이머 (약참조 캡처: 어빌리티 파괴 후 발화 방지)
	Character->GetWorld()->GetTimerManager().SetTimer(SafetyTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			DoImpact();
		}), LeapTime * 3.f, false);
}

void UROHAbility_LeapAttack::OnLanded(const FHitResult& Hit)
{
	DoImpact();
}

void UROHAbility_LeapAttack::DoImpact()
{
	if (bImpactDone)
	{
		return;
	}
	bImpactDone = true;

	AROHCharacterBase* Character = GetROHCharacter();
	if (Character)
	{
		DebugDrawSwing(Character->GetActorLocation(), ImpactRadius);

		const float Strength = Character->GetAttributeSet()->GetStrength();
		const float AttackPower = Character->GetAttributeSet()->GetAttackPower();
		FROHDamageParams Damage;
		Damage.PhysicalDamage = (BaseDamage + AttackPower) * (1.f + Strength * 0.01f) * GetSkillDamageMultiplier();
		Damage.bUseAttackRoll = true;

		for (AROHCharacterBase* Target : UROHCombatStatics::GetHostileTargetsInRadius(Character, Character->GetActorLocation(), ImpactRadius))
		{
			if (UROHCombatStatics::ApplyDamage(Character, Target, Damage))
			{
				UROHCombatStatics::ApplyKnockback(Character, Target, KnockbackStrength);
				PlayHitFeedback(Target);
			}
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UROHAbility_LeapAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AROHCharacterBase* Character = GetROHCharacter())
	{
		Character->LandedDelegate.RemoveDynamic(this, &UROHAbility_LeapAttack::OnLanded);
	}
	// 아바타가 이미 무효한 경우에도 타이머는 반드시 해제
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SafetyTimerHandle);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
