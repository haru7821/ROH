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

	/**
	 * 원소 저항 감쇄 (docs/10 §4.2): RESfinal = FlatRES + INT(Energy)×0.1, 캡 [-100, 75].
	 * 난이도 페널티는 플레이어 페널티 GE가 Flat 어트리뷰트에 이미 반영하므로 여기서 추가 감산하지 않는다.
	 */
	float MitigateElemental(float Damage, float FlatResist, float TargetEnergy)
	{
		if (Damage <= 0.f)
		{
			return 0.f;
		}
		const float ResFinal = FMath::Clamp(FlatResist + TargetEnergy * 0.1f, -100.f, 75.f);
		return Damage * (1.f - ResFinal / 100.f);
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

	// ---- 최종 피해 파이프라인 (docs/10 §5.3) ----
	// 명중 굴림 → 치명타 굴림 → 유형별 감쇄(원소 저항 §4.2 / PDR §4.3)
	// → 산포(0.95~1.05) → 치명 배율 → 룬 배율(§5.2) → GE 적용

	// 1) 명중 굴림 (공격만, 주문은 항상 명중 — §3.3, 기존 공식 유지)
	if (Params.bUseAttackRoll)
	{
		const float AR = FMath::Max(1.f, GetAttr(SourceASC, UROHAttributeSet::GetAttackRatingAttribute()));
		// 방어 등급 (docs/10 §4.1): DR = Defense × (1 + VIT/100)
		const float DEF = FMath::Max(1.f, GetAttr(TargetASC, UROHAttributeSet::GetDefenseAttribute())
			* (1.f + GetAttr(TargetASC, UROHAttributeSet::GetVitalityAttribute()) / 100.f));
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

	// 2) 치명타 굴림 (§3.2): 5% + DEX/50 + FlatCrit%, 캡 95% — ApplyDamage 1회당 1굴림
	const float CritChancePct = FMath::Clamp(
		5.f + GetAttr(SourceASC, UROHAttributeSet::GetDexterityAttribute()) / 50.f
		+ GetAttr(SourceASC, UROHAttributeSet::GetCritChanceAttribute()), 0.f, 95.f);
	const bool bCrit = FMath::FRand() * 100.f < CritChancePct;
	const float CritMult = bCrit ? GetAttr(SourceASC, UROHAttributeSet::GetCritDamageAttribute()) / 100.f : 1.f;

	// 3) 유형별 감쇄 후 합산 — Energy가 문서의 INT 역할을 승계 (docs/10 서두)
	const float TargetEnergy = GetAttr(TargetASC, UROHAttributeSet::GetEnergyAttribute());
	float TotalDamage = 0.f;

	// 물리: PDR (§4.3) = FlatPDR(PhysicalResistance) + VIT/100, 캡 [0, 90]
	if (Params.PhysicalDamage > 0.f)
	{
		const float PDR = FMath::Clamp(
			GetAttr(TargetASC, UROHAttributeSet::GetPhysicalResistanceAttribute())
			+ GetAttr(TargetASC, UROHAttributeSet::GetVitalityAttribute()) / 100.f, 0.f, 90.f);
		TotalDamage += Params.PhysicalDamage * (1.f - PDR / 100.f);
	}
	TotalDamage += MitigateElemental(Params.FireDamage, GetAttr(TargetASC, UROHAttributeSet::GetFireResistanceAttribute()), TargetEnergy);
	TotalDamage += MitigateElemental(Params.ColdDamage, GetAttr(TargetASC, UROHAttributeSet::GetColdResistanceAttribute()), TargetEnergy);
	TotalDamage += MitigateElemental(Params.LightningDamage, GetAttr(TargetASC, UROHAttributeSet::GetLightningResistanceAttribute()), TargetEnergy);
	TotalDamage += MitigateElemental(Params.PoisonDamage, GetAttr(TargetASC, UROHAttributeSet::GetPoisonResistanceAttribute()), TargetEnergy);
	TotalDamage += MitigateElemental(Params.ShadowDamage, GetAttr(TargetASC, UROHAttributeSet::GetShadowResistanceAttribute()), TargetEnergy);

	// 4) 산포 → 5) 치명 배율 → 6) 룬 배율 (음수 룬 없음 — 하한 0)
	TotalDamage *= FMath::FRandRange(0.95f, 1.05f);
	TotalDamage *= CritMult;
	TotalDamage *= 1.f + FMath::Max(0.f, GetAttr(SourceASC, UROHAttributeSet::GetRunePowerAttribute())) / 100.f;

	// 명중한 이상 0 피해라도 '적중'으로 처리 — 적중 시 부가 효과(둔화 등) 유지
	TotalDamage = FMath::Max(TotalDamage, 0.f);

	if (bCrit)
	{
		UE_LOG(LogROH, Verbose, TEXT("%s -> %s: 치명타! (%.2f배, 피해 %.0f)"),
			*Source->GetName(), *Target->GetName(), CritMult, TotalDamage);
	}

	// 7) 피해 GE 적용 (IncomingDamage → AttributeSet에서 Health 반영)
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
