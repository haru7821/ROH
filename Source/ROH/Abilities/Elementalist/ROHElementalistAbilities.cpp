#include "Abilities/Elementalist/ROHElementalistAbilities.h"
#include "Abilities/ROHGameplayEffects.h"
#include "Character/ROHCharacterBase.h"
#include "Character/ROHAttributeSet.h"
#include "Combat/ROHCombatStatics.h"
#include "Combat/ROHProjectile.h"
#include "ROHGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"

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
	CooldownTags.AddTag(ROHGameplayTags::Cooldown_Skill_Bash); // 스킬1 슬롯 쿨다운 공유 태그 재사용
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
	CooldownTags.AddTag(ROHGameplayTags::Cooldown_Skill_Whirlwind); // 스킬2 슬롯 쿨다운 태그 재사용
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
	CooldownTags.AddTag(ROHGameplayTags::Cooldown_Skill_Leap); // 스킬3 슬롯 쿨다운 태그 재사용
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
