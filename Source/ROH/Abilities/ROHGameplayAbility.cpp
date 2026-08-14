#include "Abilities/ROHGameplayAbility.h"
#include "Abilities/ROHGameplayEffects.h"
#include "Character/ROHCharacterBase.h"
#include "Combat/ROHCombatStatics.h"
#include "ROHGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

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
		ASC->ApplyModToAttribute(CostAttribute, EGameplayModOp::Additive, -CostAmount);
	}
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
}
