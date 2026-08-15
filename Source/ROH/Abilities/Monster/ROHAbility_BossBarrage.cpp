#include "Abilities/Monster/ROHAbility_BossBarrage.h"
#include "Character/ROHMonsterCharacter.h"
#include "Combat/ROHCombatStatics.h"
#include "Combat/ROHProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine/World.h"

UROHAbility_BossBarrage::UROHAbility_BossBarrage()
{
	HitStopSeconds = 0.f;
}

void UROHAbility_BossBarrage::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AROHMonsterCharacter* Monster = Cast<AROHMonsterCharacter>(GetROHCharacter());
	UWorld* World = Monster ? Monster->GetWorld() : nullptr;
	AActor* PlayerActor = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;

	if (Monster && World && PlayerActor)
	{
		const FVector AimTarget = PlayerActor->GetActorLocation();
		FaceLocation(AimTarget);

		// 화염탄 3연발 (부채꼴)
		const FVector SpawnLocation = Monster->GetActorLocation() + Monster->GetActorForwardVector() * 90.f;
		FVector FlatTarget = AimTarget;
		FlatTarget.Z = SpawnLocation.Z;
		const FRotator BaseRotation = (FlatTarget - SpawnLocation).GetSafeNormal().Rotation();

		FROHDamageParams Damage;
		Damage.FireDamage = Monster->GetAttackDamage() * DamageMultiplier;
		Damage.bUseAttackRoll = false; // 투사체 회피가 대응 수단

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Instigator = Monster;

		const float FanAngles[3] = { -FanHalfAngle, 0.f, FanHalfAngle };
		for (const float Angle : FanAngles)
		{
			const FRotator ShotRotation = BaseRotation + FRotator(0.f, Angle, 0.f);
			if (AROHProjectile* Projectile = World->SpawnActor<AROHProjectile>(AROHProjectile::StaticClass(), SpawnLocation, ShotRotation, SpawnParams))
			{
				Projectile->InitProjectile(Monster, Damage, ProjectileSpeed, 0.f);
			}
		}

		// 하수인 소환 (주변 졸개 상한 체크)
		int32 NearbyMinions = 0;
		for (TActorIterator<AROHMonster_Grunt> It(World); It; ++It)
		{
			if (It->IsAlive() && FVector::Dist2D(It->GetActorLocation(), Monster->GetActorLocation()) < 3000.f)
			{
				++NearbyMinions;
			}
		}
		const int32 ToSpawn = FMath::Min(SummonCount, MaxNearbyMinions - NearbyMinions);
		if (ToSpawn > 0)
		{
			FActorSpawnParameters MinionParams;
			MinionParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			for (int32 i = 0; i < ToSpawn; ++i)
			{
				const float Side = (i % 2 == 0) ? 1.f : -1.f;
				const FVector MinionLocation = Monster->GetActorLocation()
					+ Monster->GetActorRightVector() * Side * 250.f
					+ Monster->GetActorForwardVector() * 100.f;
				World->SpawnActor<AROHMonster_Grunt>(AROHMonster_Grunt::StaticClass(), MinionLocation, Monster->GetActorRotation(), MinionParams);
			}
			DebugDrawSwing(Monster->GetActorLocation(), 300.f);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
