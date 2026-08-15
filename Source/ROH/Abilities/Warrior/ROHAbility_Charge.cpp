#include "Abilities/Warrior/ROHAbility_Charge.h"
#include "Character/ROHCharacterBase.h"
#include "Character/ROHAttributeSet.h"
#include "Combat/ROHCombatStatics.h"
#include "ROHGameplayTags.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

UROHAbility_Charge::UROHAbility_Charge()
{
	SkillId = TEXT("Charge");
	CostAttribute = UROHAttributeSet::GetRageAttribute();
	CostAmount = 15.f;
	CooldownDuration = 5.f;
	CooldownTags.AddTag(ROHGameplayTags::Cooldown_Skill_Charge);
	HitStopSeconds = 0.06f;
	SpeedScaling = EROHSpeedScaling::Attack; // 공격 속도 스케일 (docs/10 §3.4)
}

void UROHAbility_Charge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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

	// 돌진 경로: 커서 방향, 최대 사거리 클램프 (Teleport/LeapAttack의 목표 클램프 관례)
	const FVector Start = Character->GetActorLocation();
	FVector Delta = GetCursorLocation() - Start;
	Delta.Z = 0.f;
	if (Delta.IsNearlyZero())
	{
		Delta = Character->GetActorForwardVector().GetSafeNormal2D() * MaxChargeDistance;
	}
	else if (Delta.SizeSquared() > FMath::Square(MaxChargeDistance))
	{
		Delta = Delta.GetSafeNormal() * MaxChargeDistance;
	}
	const FVector End = Start + Delta;
	const float PathLength = Delta.Size();
	const FVector PathDirection = Delta.GetSafeNormal();
	FaceLocation(End);

	// 경로 회랑 + 도착점 판정 (돌진 시작 시점 스냅샷 — 결정적)
	// 원피해에 자체 STR 항 금지 — 힘/% 배율은 ApplyDamage 파이프라인이 일괄 적용 (docs/10 §3.1, b29)
	const float AttackPower = Character->GetAttributeSet()->GetAttackPower();
	FROHDamageParams Damage;
	Damage.PhysicalDamage = (BaseDamage + AttackPower) * GetSkillDamageMultiplier();
	Damage.bUseAttackRoll = true;

	for (AROHCharacterBase* Target : UROHCombatStatics::GetHostileTargetsInRadius(Character, Start, PathLength + ArrivalRadius))
	{
		const FVector ToTarget = Target->GetActorLocation() - Start;
		const float Along = FVector::DotProduct(ToTarget, PathDirection);
		const float ClampedAlong = FMath::Clamp(Along, 0.f, PathLength);
		const FVector Closest = Start + PathDirection * ClampedAlong;
		const float DistToPath = FVector::Dist2D(Target->GetActorLocation(), Closest);

		const bool bInCorridor = DistToPath <= PathHalfWidth;
		const bool bAtArrival = FVector::Dist2D(Target->GetActorLocation(), End) <= ArrivalRadius;
		if ((bInCorridor || bAtArrival) && UROHCombatStatics::ApplyDamage(Character, Target, Damage))
		{
			PlayHitFeedback(Target);
		}
	}

	// 이동 연출: 수평 돌진 (판정은 위에서 완료 — 실제 도달 거리가 짧아도 결과 불변)
	if (DashTime > KINDA_SMALL_NUMBER)
	{
		Character->LaunchCharacter(Delta / DashTime, true, false);
	}

	// 그레이박스: 돌진 경로 표시
	DrawDebugLine(Character->GetWorld(), Start, End, FColor::Yellow, false, 0.4f, 0, 6.f);
	DrawDebugCircle(Character->GetWorld(), End, ArrivalRadius, 16, FColor::Yellow, false, 0.4f, 0, 2.f,
		FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
