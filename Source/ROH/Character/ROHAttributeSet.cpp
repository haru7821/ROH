#include "Character/ROHAttributeSet.h"
#include "Character/ROHCharacterBase.h"
#include "GameplayEffectExtension.h"

UROHAttributeSet::UROHAttributeSet()
{
	InitMaxHealth(100.f);
	InitHealth(100.f);
	InitMaxMana(50.f);
	InitMana(50.f);
	InitMaxRage(100.f);
	InitRage(0.f);
	InitStrength(10.f);
	InitDexterity(10.f);
	InitVitality(10.f);
	InitEnergy(10.f);
	InitCharacterLevel(1.f);
	InitAttackRating(50.f);
	InitDefense(50.f);
	InitPhysicalResistance(0.f);
	InitFireResistance(0.f);
	InitColdResistance(0.f);
	InitLightningResistance(0.f);
	InitMoveSpeed(600.f);
	InitIncomingDamage(0.f);
}

void UROHAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMana());
	}
	else if (Attribute == GetRageAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxRage());
	}
	else if (Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 100.f, 1200.f);
	}
}

void UROHAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float Damage = GetIncomingDamage();
		SetIncomingDamage(0.f);

		if (Damage > 0.f && GetHealth() > 0.f)
		{
			SetHealth(FMath::Clamp(GetHealth() - Damage, 0.f, GetMaxHealth()));

			if (AROHCharacterBase* OwnerCharacter = Cast<AROHCharacterBase>(GetOwningActor()))
			{
				AActor* InstigatorActor = Data.EffectSpec.GetContext().GetOriginalInstigator();
				OwnerCharacter->HandleDamageTaken(Damage, InstigatorActor);

				if (GetHealth() <= 0.f)
				{
					OwnerCharacter->HandleDeath(InstigatorActor);
				}
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
	}
	else if (Data.EvaluatedData.Attribute == GetRageAttribute())
	{
		SetRage(FMath::Clamp(GetRage(), 0.f, GetMaxRage()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxRageAttribute())
	{
		SetRage(FMath::Clamp(GetRage(), 0.f, GetMaxRage()));
	}
}
