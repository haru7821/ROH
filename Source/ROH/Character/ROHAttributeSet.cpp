#include "Character/ROHAttributeSet.h"
#include "GameplayEffectExtension.h"

UROHAttributeSet::UROHAttributeSet()
{
	InitMaxHealth(100.f);
	InitHealth(100.f);
	InitMaxMana(50.f);
	InitMana(50.f);
	InitStrength(10.f);
	InitDexterity(10.f);
	InitVitality(10.f);
	InitEnergy(10.f);
	InitMoveSpeed(600.f);
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
	else if (Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 100.f, 1200.f);
	}
}

void UROHAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
	}
}
