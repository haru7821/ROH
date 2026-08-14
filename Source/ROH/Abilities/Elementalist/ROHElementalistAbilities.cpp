#include "Abilities/Elementalist/ROHElementalistAbilities.h"
#include "Abilities/ROHGameplayEffects.h"
#include "Character/ROHCharacterBase.h"
#include "Character/ROHAttributeSet.h"
#include "Combat/ROHCombatStatics.h"
#include "Combat/ROHProjectile.h"
#include "ROHGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

namespace
{
	/** 커서 방향으로 투사체 스폰 (원소술사 공용) */
	AROHProjectile* SpawnProjectileTowardCursor(AROHCharacterBase* Caster,
		const FVector& AimPoint, const FROHDamageParams& Damage, float Speed, float ExplosionRadius)
	{
		UWorld* World = Caster ? Caster->GetWorld() : nullptr;
		if (!World)
		{
			return nullptr;
		}

		const FVector SpawnLocation = Caster->GetActorLocation() + Caster->GetActorForwardVector() * 80.f;
		FVector AimTarget = AimPoint;
		AimTarget.Z = SpawnLocation.Z; // 지면 클릭 시 아래로 꽂히지 않게 수평 사격
		const FRotator SpawnRotation = (AimTarget - SpawnLocation).GetSafeNormal().Rotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Instigator = Caster;

		AROHProjectile* Projectile = World->SpawnActor<AROHProjectile>(AROHProjectile::StaticClass(), SpawnLocation, SpawnRotation, SpawnParams);
		if (Projectile)
		{
			Projectile->InitProjectile(Caster, Damage, Speed, ExplosionRadius);
		}
		return Projectile;
	}
}

// ---------- 기본 공격: 마법탄 ----------

UROHAbility_MagicBolt::UROHAbility_MagicBolt()
{
	CooldownDuration = 0.5f;
	CooldownTags.AddTag(ROHGameplayTags::Cooldown_BasicAttack);
	HitStopSeconds = 0.f;
}

void UROHAbility_MagicBolt::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AROHCharacterBase* Caster = GetROHCharacter();
	if (Caster)
	{
		const FVector Aim = GetCursorLocation();
		FaceLocation(Aim);

		// 주문 피해 = 기본 피해 × (1 + 에너지 × 1%) — 명중 굴림 없음 (주문)
		const float Energy = Caster->GetAttributeSet()->GetEnergy();
		FROHDamageParams Damage;
		Damage.LightningDamage = BaseDamage * (1.f + Energy * 0.01f);
		Damage.bUseAttackRoll = false;

		SpawnProjectileTowardCursor(Caster, Aim, Damage, ProjectileSpeed, 0.f);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

// ---------- 화염구 ----------

UROHAbility_Fireball::UROHAbility_Fireball()
{
	SkillId = TEXT("Fireball");
	CostAttribute = UROHAttributeSet::GetManaAttribute();
	CostAmount = 12.f;
	CooldownDuration = 2.5f;
	CooldownTags.AddTag(ROHGameplayTags::Cooldown_Skill_Fireball);
	HitStopSeconds = 0.f;
}

void UROHAbility_Fireball::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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

	AROHCharacterBase* Caster = GetROHCharacter();
	if (Caster)
	{
		const FVector Aim = GetCursorLocation();
		FaceLocation(Aim);

		const float Energy = Caster->GetAttributeSet()->GetEnergy();
		FROHDamageParams Damage;
		Damage.FireDamage = BaseDamage * (1.f + Energy * 0.01f) * GetSkillDamageMultiplier();
		Damage.bUseAttackRoll = false;

		SpawnProjectileTowardCursor(Caster, Aim, Damage, ProjectileSpeed, ExplosionRadius);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

// ---------- 서리 신성 ----------

UROHAbility_FrostNova::UROHAbility_FrostNova()
{
	SkillId = TEXT("FrostNova");
	CostAttribute = UROHAttributeSet::GetManaAttribute();
	CostAmount = 18.f;
	CooldownDuration = 8.f;
	CooldownTags.AddTag(ROHGameplayTags::Cooldown_Skill_FrostNova);
}

void UROHAbility_FrostNova::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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

	AROHCharacterBase* Caster = GetROHCharacter();
	if (Caster)
	{
		DebugDrawSwing(Caster->GetActorLocation(), Radius);

		const float Energy = Caster->GetAttributeSet()->GetEnergy();
		FROHDamageParams Damage;
		Damage.ColdDamage = BaseDamage * (1.f + Energy * 0.01f) * GetSkillDamageMultiplier();
		Damage.bUseAttackRoll = false;

		for (AROHCharacterBase* Target : UROHCombatStatics::GetHostileTargetsInRadius(Caster, Caster->GetActorLocation(), Radius))
		{
			if (UROHCombatStatics::ApplyDamage(Caster, Target, Damage))
			{
				// 냉기 둔화 (3초)
				if (UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent())
				{
					TargetASC->ApplyGameplayEffectToSelf(
						UROHSlowEffect::StaticClass()->GetDefaultObject<UGameplayEffect>(), 1.f, TargetASC->MakeEffectContext());
				}
				PlayHitFeedback(Target);
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

// ---------- 텔레포트 ----------

UROHAbility_Teleport::UROHAbility_Teleport()
{
	SkillId = TEXT("Teleport");
	CostAttribute = UROHAttributeSet::GetManaAttribute();
	CostAmount = 15.f;
	CooldownDuration = 6.f;
	CooldownTags.AddTag(ROHGameplayTags::Cooldown_Skill_Teleport);
}

void UROHAbility_Teleport::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CheckSkillInvested())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AROHCharacterBase* Caster = GetROHCharacter();
	if (!Caster)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FVector Start = Caster->GetActorLocation();
	FVector Target = GetCursorLocation();
	Target.Z = Start.Z;

	FVector Delta = Target - Start;
	if (Delta.SizeSquared() > FMath::Square(MaxDistance))
	{
		Delta = Delta.GetSafeNormal() * MaxDistance;
	}

	// 목적지가 막혀 이동에 실패하면 비용/쿨다운을 소모하지 않는다
	if (!Caster->TeleportTo(Start + Delta, Caster->GetActorRotation()))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	FaceLocation(Start + Delta);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

// ---------- 얼음화살 ----------

UROHAbility_IceBolt::UROHAbility_IceBolt()
{
	SkillId = TEXT("IceBolt");
	CostAttribute = UROHAttributeSet::GetManaAttribute();
	CostAmount = 4.f;
	CooldownDuration = 0.8f;
	CooldownTags.AddTag(ROHGameplayTags::Cooldown_Skill_IceBolt);
	HitStopSeconds = 0.f;
}

void UROHAbility_IceBolt::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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

	AROHCharacterBase* Caster = GetROHCharacter();
	if (Caster)
	{
		const FVector Aim = GetCursorLocation();
		FaceLocation(Aim);

		const float Energy = Caster->GetAttributeSet()->GetEnergy();
		FROHDamageParams Damage;
		Damage.ColdDamage = BaseDamage * (1.f + Energy * 0.01f) * GetSkillDamageMultiplier();
		Damage.bUseAttackRoll = false;

		SpawnProjectileTowardCursor(Caster, Aim, Damage, ProjectileSpeed, 0.f);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

// ---------- 운석 ----------

UROHAbility_Meteor::UROHAbility_Meteor()
{
	SkillId = TEXT("Meteor");
	CostAttribute = UROHAttributeSet::GetManaAttribute();
	CostAmount = 25.f;
	CooldownDuration = 6.f;
	CooldownTags.AddTag(ROHGameplayTags::Cooldown_Skill_Meteor);
	HitStopSeconds = 0.f;
}

void UROHAbility_Meteor::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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

	AROHCharacterBase* Caster = GetROHCharacter();
	if (Caster)
	{
		const FVector Target = GetCursorLocation();
		FaceLocation(Target);

		const float Energy = Caster->GetAttributeSet()->GetEnergy();
		FROHDamageParams Damage;
		Damage.FireDamage = BaseDamage * (1.f + Energy * 0.01f) * GetSkillDamageMultiplier();
		Damage.bUseAttackRoll = false;

		// 낙하 예고 표시 후 지연 폭발 (어빌리티는 즉시 종료 — 시전자 소멸 시 타이머는 무시된다)
		DrawDebugSphere(Caster->GetWorld(), Target, ImpactRadius, 16, FColor::Red, false, ImpactDelay);

		const float Radius = ImpactRadius;
		FTimerHandle TimerHandle;
		Caster->GetWorldTimerManager().SetTimer(TimerHandle,
			FTimerDelegate::CreateWeakLambda(Caster, [Caster, Target, Damage, Radius]()
			{
				if (!Caster->IsAlive())
				{
					return; // 시전자가 죽었으면 낙하 취소
				}
				DrawDebugSphere(Caster->GetWorld(), Target, Radius, 16, FColor::Orange, false, 0.4f);
				for (AROHCharacterBase* Victim : UROHCombatStatics::GetHostileTargetsInRadius(Caster, Target, Radius))
				{
					if (UROHCombatStatics::ApplyDamage(Caster, Victim, Damage))
					{
						DrawDebugSphere(Caster->GetWorld(), Victim->GetActorLocation() + FVector(0.f, 0.f, 50.f), 40.f, 12, FColor::Red, false, 0.25f);
					}
				}
			}), ImpactDelay, false);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

// ---------- 눈보라 ----------

UROHAbility_Blizzard::UROHAbility_Blizzard()
{
	SkillId = TEXT("Blizzard");
	CostAttribute = UROHAttributeSet::GetManaAttribute();
	CostAmount = 30.f;
	CooldownDuration = 10.f;
	CooldownTags.AddTag(ROHGameplayTags::Cooldown_Skill_Blizzard);
	HitStopSeconds = 0.f;
}

void UROHAbility_Blizzard::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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

	AROHCharacterBase* Caster = GetROHCharacter();
	if (Caster && PulseCount > 0)
	{
		const FVector Target = GetCursorLocation();
		FaceLocation(Target);

		const float Energy = Caster->GetAttributeSet()->GetEnergy();
		FROHDamageParams Damage;
		Damage.ColdDamage = DamagePerPulse * (1.f + Energy * 0.01f) * GetSkillDamageMultiplier();
		Damage.bUseAttackRoll = false;

		// 폭풍 범위 예고
		DrawDebugSphere(Caster->GetWorld(), Target, Radius, 16, FColor::Cyan, false, PulseInterval * PulseCount);

		// 반복 타이머로 파동 3회 — 카운터/핸들은 공유 포인터로 람다와 수명 공유.
		// 시전자 소멸 시 파동은 실행되지 않고, 타이머는 월드 정리 시 함께 제거된다.
		const float PulseRadius = Radius;
		TSharedRef<int32> PulsesLeft = MakeShared<int32>(PulseCount);
		TSharedRef<FTimerHandle> TimerRef = MakeShared<FTimerHandle>();
		Caster->GetWorldTimerManager().SetTimer(*TimerRef,
			FTimerDelegate::CreateWeakLambda(Caster, [Caster, Target, Damage, PulseRadius, PulsesLeft, TimerRef]()
			{
				if (!Caster->IsAlive())
				{
					Caster->GetWorldTimerManager().ClearTimer(*TimerRef); // 시전자가 죽었으면 폭풍 중단
					return;
				}
				DrawDebugSphere(Caster->GetWorld(), Target, PulseRadius, 16, FColor::Blue, false, 0.3f);
				for (AROHCharacterBase* Victim : UROHCombatStatics::GetHostileTargetsInRadius(Caster, Target, PulseRadius))
				{
					if (UROHCombatStatics::ApplyDamage(Caster, Victim, Damage))
					{
						// 냉기 둔화 (파동마다 갱신)
						if (UAbilitySystemComponent* VictimASC = Victim->GetAbilitySystemComponent())
						{
							VictimASC->ApplyGameplayEffectToSelf(
								UROHSlowEffect::StaticClass()->GetDefaultObject<UGameplayEffect>(), 1.f, VictimASC->MakeEffectContext());
						}
						DrawDebugSphere(Caster->GetWorld(), Victim->GetActorLocation() + FVector(0.f, 0.f, 50.f), 40.f, 12, FColor::Red, false, 0.25f);
					}
				}
				if (--(*PulsesLeft) <= 0)
				{
					Caster->GetWorldTimerManager().ClearTimer(*TimerRef);
				}
			}), PulseInterval, true);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

// ---------- 정전기장 ----------

UROHAbility_StaticField::UROHAbility_StaticField()
{
	SkillId = TEXT("StaticField");
	CostAttribute = UROHAttributeSet::GetManaAttribute();
	CostAmount = 10.f;
	CooldownDuration = 1.5f;
	CooldownTags.AddTag(ROHGameplayTags::Cooldown_Skill_StaticField);
	HitStopSeconds = 0.f;
}

void UROHAbility_StaticField::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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

	AROHCharacterBase* Caster = GetROHCharacter();
	if (Caster)
	{
		DebugDrawSwing(Caster->GetActorLocation(), Radius);

		// 현재 생명력 비례 — 랭크 배수 반영, 50% 상한 (즉사기 방지)
		const float HealthPercent = FMath::Min(BaseHealthPercent * GetSkillDamageMultiplier(), 0.5f);

		for (AROHCharacterBase* Victim : UROHCombatStatics::GetHostileTargetsInRadius(Caster, Caster->GetActorLocation(), Radius))
		{
			const UROHAttributeSet* VictimAttributes = Victim->GetAttributeSet();
			if (!VictimAttributes)
			{
				continue;
			}
			FROHDamageParams Damage;
			Damage.LightningDamage = VictimAttributes->GetHealth() * HealthPercent;
			Damage.bUseAttackRoll = false;
			if (Damage.LightningDamage > 0.f && UROHCombatStatics::ApplyDamage(Caster, Victim, Damage))
			{
				PlayHitFeedback(Victim);
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
