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
	CostAttribute = UROHAttributeSet::GetRageAttribute();
	CostAmount = 20.f;
	CooldownDuration = 8.f;
	CooldownTags.AddTag(ROHGameplayTags::Cooldown_Skill_Leap);
}

void UROHAbility_LeapAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
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

	// 착지 이벤트를 못 받는 경우 대비 안전 타이머
	Character->GetWorld()->GetTimerManager().SetTimer(SafetyTimerHandle, [this]()
	{
		DoImpact();
	}, LeapTime * 3.f, false);
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
		const float Strength = Character->GetAttributeSet()->GetStrength();
		FROHDamageParams Damage;
		Damage.PhysicalDamage = BaseDamage * (1.f + Strength * 0.01f);
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
		if (Character->GetWorld())
		{
			Character->GetWorld()->GetTimerManager().ClearTimer(SafetyTimerHandle);
		}
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
