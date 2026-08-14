#include "Combat/ROHCombatStatics.h"
#include "Character/ROHCharacterBase.h"
#include "Character/ROHAttributeSet.h"
#include "Abilities/ROHGameplayEffects.h"
#include "ROHGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"
#include "ROH.h"

namespace
{
	float GetAttr(const UAbilitySystemComponent* ASC, const FGameplayAttribute& Attribute)
	{
		return ASC ? ASC->GetNumericAttribute(Attribute) : 0.f;
	}

	float MitigateByResistance(float Damage, float ResistancePercent)
	{
		if (Damage <= 0.f)
		{
			return 0.f;
		}
		const float Capped = FMath::Min(ResistancePercent, 75.f); // 저항 캡 75%
		return Damage * (1.f - Capped / 100.f);
	}
}

bool UROHCombatStatics::ApplyDamage(AROHCharacterBase* Source, AROHCharacterBase* Target, const FROHDamageParams& Params)
{
	if (!Source || !Target || Source == Target || !Target->IsAlive())
	{
		return false;
	}

	UAbilitySystemComponent* SourceASC = Source->GetAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
	if (!SourceASC || !TargetASC)
	{
		return false;
	}

	// 1) 명중 굴림 (공격만, 주문은 항상 명중)
	if (Params.bUseAttackRoll)
	{
		const float AR = FMath::Max(1.f, GetAttr(SourceASC, UROHAttributeSet::GetAttackRatingAttribute()));
		const float DEF = FMath::Max(1.f, GetAttr(TargetASC, UROHAttributeSet::GetDefenseAttribute()));
		const float ALvl = FMath::Max(1.f, GetAttr(SourceASC, UROHAttributeSet::GetCharacterLevelAttribute()));
		const float DLvl = FMath::Max(1.f, GetAttr(TargetASC, UROHAttributeSet::GetCharacterLevelAttribute()));

		const float HitChance = FMath::Clamp(
			2.f * AR / (AR + DEF) * (ALvl / (ALvl + DLvl)), 0.05f, 0.95f);

		if (FMath::FRand() > HitChance)
		{
			UE_LOG(LogROH, Verbose, TEXT("%s -> %s: 빗나감 (명중률 %.0f%%)"),
				*Source->GetName(), *Target->GetName(), HitChance * 100.f);
			return false;
		}
	}

	// 2) 유형별 저항 완화 후 합산
	float TotalDamage = 0.f;
	TotalDamage += MitigateByResistance(Params.PhysicalDamage, GetAttr(TargetASC, UROHAttributeSet::GetPhysicalResistanceAttribute()));
	TotalDamage += MitigateByResistance(Params.FireDamage, GetAttr(TargetASC, UROHAttributeSet::GetFireResistanceAttribute()));
	TotalDamage += MitigateByResistance(Params.ColdDamage, GetAttr(TargetASC, UROHAttributeSet::GetColdResistanceAttribute()));
	TotalDamage += MitigateByResistance(Params.LightningDamage, GetAttr(TargetASC, UROHAttributeSet::GetLightningResistanceAttribute()));

	if (TotalDamage <= 0.f)
	{
		return false;
	}

	// 3) 피해 GE 적용 (IncomingDamage → AttributeSet에서 Health 반영)
	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(Source);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(UROHDamageEffect::StaticClass(), 1.f, Context);
	if (!SpecHandle.IsValid())
	{
		return false;
	}
	SpecHandle.Data->SetSetByCallerMagnitude(ROHGameplayTags::Data_Damage, TotalDamage);
	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	return true;
}

TArray<AROHCharacterBase*> UROHCombatStatics::GetHostileTargetsInCone(AROHCharacterBase* Source, float Range, float HalfAngleDegrees)
{
	TArray<AROHCharacterBase*> Results;
	if (!Source)
	{
		return Results;
	}

	const FVector Origin = Source->GetActorLocation();
	const FVector Forward = Source->GetActorForwardVector().GetSafeNormal2D();
	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(HalfAngleDegrees, 1.f, 180.f)));
	const bool bOmnidirectional = HalfAngleDegrees >= 180.f;

	for (AROHCharacterBase* Candidate : GetHostileTargetsInRadius(Source, Origin, Range))
	{
		if (bOmnidirectional)
		{
			Results.Add(Candidate);
			continue;
		}
		const FVector ToTarget = (Candidate->GetActorLocation() - Origin).GetSafeNormal2D();
		if (FVector::DotProduct(Forward, ToTarget) >= CosHalfAngle)
		{
			Results.Add(Candidate);
		}
	}
	return Results;
}

TArray<AROHCharacterBase*> UROHCombatStatics::GetHostileTargetsInRadius(AROHCharacterBase* Source, const FVector& Origin, float Radius)
{
	TArray<AROHCharacterBase*> Results;
	if (!Source || !Source->GetWorld())
	{
		return Results;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ROHTargetSearch), false, Source);
	Source->GetWorld()->OverlapMultiByObjectType(
		Overlaps, Origin, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(Radius), QueryParams);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AROHCharacterBase* Candidate = Cast<AROHCharacterBase>(Overlap.GetActor());
		if (Candidate && Candidate != Source && Candidate->IsAlive() && Candidate->GetTeamId() != Source->GetTeamId())
		{
			Results.AddUnique(Candidate);
		}
	}
	return Results;
}

void UROHCombatStatics::ApplyKnockback(AROHCharacterBase* Source, AROHCharacterBase* Target, float Strength)
{
	if (!Source || !Target || Strength <= 0.f)
	{
		return;
	}
	const FVector Direction = (Target->GetActorLocation() - Source->GetActorLocation()).GetSafeNormal2D();
	Target->LaunchCharacter(Direction * Strength + FVector(0.f, 0.f, Strength * 0.3f), true, true);
}

void UROHCombatStatics::ApplyHitStop(AROHCharacterBase* Source, AROHCharacterBase* Target, float DurationSeconds)
{
	if (!Source || !Source->GetWorld() || DurationSeconds <= 0.f)
	{
		return;
	}

	TWeakObjectPtr<AROHCharacterBase> WeakSource = Source;
	TWeakObjectPtr<AROHCharacterBase> WeakTarget = Target;

	Source->CustomTimeDilation = 0.05f;
	if (Target)
	{
		Target->CustomTimeDilation = 0.05f;
	}

	FTimerHandle TimerHandle;
	Source->GetWorld()->GetTimerManager().SetTimer(TimerHandle, [WeakSource, WeakTarget]()
	{
		if (WeakSource.IsValid())
		{
			WeakSource->CustomTimeDilation = 1.f;
		}
		if (WeakTarget.IsValid())
		{
			WeakTarget->CustomTimeDilation = 1.f;
		}
	}, DurationSeconds, false);
}
