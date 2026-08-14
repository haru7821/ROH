#include "Abilities/Monster/ROHAbility_MonsterAttack.h"
#include "Character/ROHMonsterCharacter.h"
#include "Combat/ROHCombatStatics.h"
#include "Combat/ROHProjectile.h"
#include "ROHGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

UROHAbility_MonsterMelee::UROHAbility_MonsterMelee()
{
	CooldownDuration = 1.f;
	CooldownTags.AddTag(ROHGameplayTags::Cooldown_Monster_Attack);
	HitStopSeconds = 0.f; // 몬스터 공격은 히트스톱 없음
}

void UROHAbility_MonsterMelee::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AROHMonsterCharacter* Monster = Cast<AROHMonsterCharacter>(GetROHCharacter());
	if (Monster)
	{
		FROHDamageParams Damage;
		Damage.PhysicalDamage = Monster->GetAttackDamage();
		Damage.bUseAttackRoll = true;

		for (AROHCharacterBase* Target : UROHCombatStatics::GetHostileTargetsInCone(Monster, Monster->GetAttackRange() * 1.2f, 60.f))
		{
			if (UROHCombatStatics::ApplyDamage(Monster, Target, Damage))
			{
				PlayHitFeedback(Target);
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

UROHAbility_MonsterRanged::UROHAbility_MonsterRanged()
{
	CooldownDuration = 1.5f;
	CooldownTags.AddTag(ROHGameplayTags::Cooldown_Monster_Attack);
	HitStopSeconds = 0.f;
	ProjectileClass = AROHProjectile::StaticClass();
}

void UROHAbility_MonsterRanged::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AROHMonsterCharacter* Monster = Cast<AROHMonsterCharacter>(GetROHCharacter());
	UWorld* World = Monster ? Monster->GetWorld() : nullptr;

	if (Monster && World && ProjectileClass)
	{
		AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(World, 0);
		if (PlayerActor)
		{
			const FVector AimTarget = PlayerActor->GetActorLocation();
			FaceLocation(AimTarget);

			const FVector SpawnLocation = Monster->GetActorLocation() + Monster->GetActorForwardVector() * 80.f;
			const FRotator SpawnRotation = (AimTarget - SpawnLocation).GetSafeNormal().Rotation();

			FROHDamageParams Damage;
			Damage.PhysicalDamage = Monster->GetAttackDamage();
			Damage.bUseAttackRoll = true;

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParams.Instigator = Monster;

			AROHProjectile* Projectile = World->SpawnActor<AROHProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
			if (Projectile)
			{
				Projectile->InitProjectile(Monster, Damage, ProjectileSpeed);
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
