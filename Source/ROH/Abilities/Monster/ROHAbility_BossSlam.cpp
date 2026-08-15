#include "Abilities/Monster/ROHAbility_BossSlam.h"
#include "Character/ROHMonsterCharacter.h"
#include "Combat/ROHCombatStatics.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

UROHAbility_BossSlam::UROHAbility_BossSlam()
{
	HitStopSeconds = 0.f;
}

void UROHAbility_BossSlam::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AROHMonsterCharacter* Monster = Cast<AROHMonsterCharacter>(GetROHCharacter());
	if (Monster)
	{
		const FVector Center = Monster->GetActorLocation();

		FROHDamageParams Damage;
		Damage.PhysicalDamage = Monster->GetAttackDamage() * DamageMultiplier;
		Damage.bUseAttackRoll = false; // 장판 밖으로 나가는 것이 회피 수단

		// 예고 장판 (빨간 원) → 지연 후 폭발. 시전자 소멸/사망 시 취소.
		DrawDebugSphere(Monster->GetWorld(), Center, SlamRadius, 20, FColor::Red, false, TelegraphTime);

		const float Radius = SlamRadius;
		const float Knockback = KnockbackStrength;
		FTimerHandle TimerHandle;
		Monster->GetWorldTimerManager().SetTimer(TimerHandle,
			FTimerDelegate::CreateWeakLambda(Monster, [Monster, Center, Damage, Radius, Knockback]()
			{
				if (!Monster->IsAlive())
				{
					return;
				}
				DrawDebugSphere(Monster->GetWorld(), Center, Radius, 20, FColor::Orange, false, 0.4f);
				for (AROHCharacterBase* Target : UROHCombatStatics::GetHostileTargetsInRadius(Monster, Center, Radius))
				{
					if (UROHCombatStatics::ApplyDamage(Monster, Target, Damage))
					{
						UROHCombatStatics::ApplyKnockback(Monster, Target, Knockback);
					}
				}
			}), TelegraphTime, false);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
