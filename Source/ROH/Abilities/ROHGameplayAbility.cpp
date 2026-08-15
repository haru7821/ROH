#include "Abilities/ROHGameplayAbility.h"
#include "Abilities/ROHGameplayEffects.h"
#include "Character/ROHCharacterBase.h"
#include "Combat/ROHCombatStatics.h"
#include "Progression/ROHSkillTreeComponent.h"
#include "Engine/Engine.h"
#include "ROHGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "DrawDebugHelpers.h"

UROHGameplayAbility::UROHGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	CooldownGameplayEffectClass = UROHCooldownEffect::StaticClass();
	ActivationBlockedTags.AddTag(ROHGameplayTags::State_Dead);
}

bool UROHGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!CostAttribute.IsValid() || CostAmount <= 0.f)
	{
		return true;
	}
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	return ASC && ASC->GetNumericAttribute(CostAttribute) >= CostAmount;
}

void UROHGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!CostAttribute.IsValid() || CostAmount <= 0.f)
	{
		return;
	}
	if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		const float Before = ASC->GetNumericAttribute(CostAttribute);
		ASC->ApplyModToAttribute(CostAttribute, EGameplayModOp::Additive, -CostAmount);
		const float After = ASC->GetNumericAttribute(CostAttribute);
		UE_LOG(LogROH, Verbose, TEXT("ApplyCost: %s %.1f → %.1f (-%.1f)"), *CostAttribute.GetName(), Before, After, CostAmount);
	}
}

bool UROHGameplayAbility::CommitAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, FGameplayTagContainer* OptionalRelevantTags)
{
	// 그레이박스 피드백: 자원 차감/불발 사유가 화면에 보이지 않으면 유저가 판단할 수 없다
	const AROHCharacterBase* Character = GetROHCharacter();
	const bool bShowFeedback = GEngine && Character && Character->IsPlayerControlled();

	auto ResourceName = [this]() -> FString
	{
		const FString AttributeName = CostAttribute.GetName();
		if (AttributeName == TEXT("Rage")) { return TEXT("분노"); }
		if (AttributeName == TEXT("Mana")) { return TEXT("마나"); }
		return AttributeName;
	};

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const bool bHasCost = CostAttribute.IsValid() && CostAmount > 0.f && ASC;
	const float CostBefore = bHasCost ? ASC->GetNumericAttribute(CostAttribute) : 0.f;

	if (Super::CommitAbility(Handle, ActorInfo, ActivationInfo, OptionalRelevantTags))
	{
		if (bHasCost)
		{
			// 방어: 엔진 커밋 경로에서 비용이 실제로 차감되지 않았으면 직접 차감
			// (차감 여부를 커밋 전후 값으로 검증 — 원인 추적용 경고 로그 포함)
			float Remaining = ASC->GetNumericAttribute(CostAttribute);
			if (FMath::IsNearlyEqual(Remaining, CostBefore))
			{
				// base 기준으로 차감 (current 유래 값 사용 시 활성 GE의 current 보정만큼 부풀 수 있음)
				ASC->SetNumericAttributeBase(CostAttribute, FMath::Max(0.f, ASC->GetNumericAttributeBase(CostAttribute) - CostAmount));
				Remaining = ASC->GetNumericAttribute(CostAttribute);
				UE_LOG(LogROH, Warning, TEXT("CommitAbility: ApplyCost 미반영 감지 → 직접 차감 (%s %.1f→%.1f)"),
					*CostAttribute.GetName(), CostBefore, Remaining);
			}
			if (bShowFeedback)
			{
				GEngine->AddOnScreenDebugMessage(6, 1.5f, FColor::Silver,
					FString::Printf(TEXT("%s -%.0f (잔여 %.0f)"), *ResourceName(), CostAmount, Remaining));
			}
		}
		return true;
	}

	if (bShowFeedback)
	{
		const FString SkillName = SkillId.IsNone() ? TEXT("기본 공격") : SkillId.ToString();
		FString Reason;
		if (!CheckCooldown(Handle, ActorInfo))
		{
			Reason = FString::Printf(TEXT("%s: 쿨다운 중"), *SkillName);
		}
		else if (!CheckCost(Handle, ActorInfo))
		{
			const float Current = ASC && CostAttribute.IsValid() ? ASC->GetNumericAttribute(CostAttribute) : 0.f;
			Reason = FString::Printf(TEXT("%s: %s 부족 (%.0f 필요, 현재 %.0f)"),
				*SkillName, *ResourceName(), CostAmount, Current);
		}
		else
		{
			Reason = FString::Printf(TEXT("%s: 발동 불가"), *SkillName);
		}
		GEngine->AddOnScreenDebugMessage(4, 2.f, FColor::Orange, Reason);
	}
	return false;
}

const FGameplayTagContainer* UROHGameplayAbility::GetCooldownTags() const
{
	FGameplayTagContainer* MutableTags = const_cast<FGameplayTagContainer*>(&TempCooldownTags);
	MutableTags->Reset();
	if (const FGameplayTagContainer* ParentTags = Super::GetCooldownTags())
	{
		MutableTags->AppendTags(*ParentTags);
	}
	MutableTags->AppendTags(CooldownTags);
	return MutableTags;
}

void UROHGameplayAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (CooldownDuration <= 0.f || CooldownTags.IsEmpty())
	{
		return;
	}
	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(UROHCooldownEffect::StaticClass(), GetAbilityLevel());
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->DynamicGrantedTags.AppendTags(CooldownTags);
		SpecHandle.Data->SetSetByCallerMagnitude(ROHGameplayTags::Data_Cooldown, CooldownDuration);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
}

AROHCharacterBase* UROHGameplayAbility::GetROHCharacter() const
{
	return Cast<AROHCharacterBase>(GetAvatarActorFromActorInfo());
}

FVector UROHGameplayAbility::GetCursorLocation() const
{
	const AROHCharacterBase* Character = GetROHCharacter();
	if (!Character)
	{
		return FVector::ZeroVector;
	}

	if (const APlayerController* PC = Cast<APlayerController>(Character->GetController()))
	{
		FHitResult Hit;
		if (PC->GetHitResultUnderCursor(ECC_Visibility, true, Hit))
		{
			return Hit.Location;
		}
	}
	return Character->GetActorLocation() + Character->GetActorForwardVector() * 300.f;
}

void UROHGameplayAbility::FaceLocation(const FVector& Location) const
{
	AROHCharacterBase* Character = GetROHCharacter();
	if (!Character)
	{
		return;
	}
	const FVector Direction = (Location - Character->GetActorLocation()).GetSafeNormal2D();
	if (!Direction.IsNearlyZero())
	{
		Character->SetActorRotation(Direction.Rotation());
	}
}

void UROHGameplayAbility::PlayHitFeedback(AROHCharacterBase* Target) const
{
	AROHCharacterBase* Character = GetROHCharacter();
	if (!Character || !Target)
	{
		return;
	}

	const FVector HitLocation = Target->GetActorLocation();

	if (HitEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(Character->GetWorld(), HitEffect, HitLocation);
	}
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(Character->GetWorld(), HitSound, HitLocation);
	}
	if (HitCameraShake)
	{
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			PC->ClientStartCameraShake(HitCameraShake);
		}
	}
	if (HitStopSeconds > 0.f)
	{
		UROHCombatStatics::ApplyHitStop(Character, Target, HitStopSeconds);
	}

	// 그레이박스 타격 표시: 맞은 대상 위치에 빨간 구
	DrawDebugSphere(Character->GetWorld(), HitLocation + FVector(0.f, 0.f, 50.f), 40.f, 12, FColor::Red, false, 0.25f);
}

void UROHGameplayAbility::DebugDrawSwing(const FVector& Center, float Radius) const
{
	if (const AROHCharacterBase* Character = GetROHCharacter())
	{
		DrawDebugSphere(Character->GetWorld(), Center, Radius, 16, FColor::Yellow, false, 0.2f);
	}
}

bool UROHGameplayAbility::CheckSkillInvested() const
{
	if (SkillId.IsNone())
	{
		return true;
	}
	const AROHCharacterBase* Character = GetROHCharacter();
	const UROHSkillTreeComponent* SkillTree = Character ? Character->FindComponentByClass<UROHSkillTreeComponent>() : nullptr;
	if (SkillTree && SkillTree->GetRank(SkillId) > 0)
	{
		return true;
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(4, 3.f, FColor::Orange,
			FString::Printf(TEXT("스킬 미습득: %s (콘솔 ROHSkillUp %s)"), *SkillId.ToString(), *SkillId.ToString()));
	}
	return false;
}

float UROHGameplayAbility::GetSkillDamageMultiplier() const
{
	if (SkillId.IsNone())
	{
		return 1.f;
	}
	const AROHCharacterBase* Character = GetROHCharacter();
	const UROHSkillTreeComponent* SkillTree = Character ? Character->FindComponentByClass<UROHSkillTreeComponent>() : nullptr;
	const float Multiplier = SkillTree ? SkillTree->GetDamageMultiplier(SkillId) : 1.f;
	return Multiplier > 0.f ? Multiplier : 1.f;
}
