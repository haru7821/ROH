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
	InitHealthRegen(0.f);
	InitManaRegen(0.f);
	InitStrength(10.f);
	InitDexterity(10.f);
	InitVitality(10.f);
	InitEnergy(10.f);
	InitCharacterLevel(1.f);
	InitAttackRating(50.f);
	InitDefense(50.f);
	InitAttackPower(0.f);
	InitMagicFind(0.f);
	InitCritChance(0.f);
	InitCritDamage(150.f); // 치명타 기본 1.5배 (docs/10 §3.2)
	InitRunePower(0.f);
	InitAttackSpeedPct(0.f);
	InitCastSpeedPct(0.f);
	InitPhysicalResistance(0.f);
	InitFireResistance(0.f);
	InitColdResistance(0.f);
	InitLightningResistance(0.f);
	InitPoisonResistance(0.f);
	InitShadowResistance(0.f);
	InitMoveSpeed(600.f);
	InitHealthPct(0.f);
	InitManaPct(0.f);
	InitHealthRegenPct(0.f);
	InitManaRegenPct(0.f);
	InitPhysicalDamagePct(0.f);
	InitElementalDamagePct(0.f);
	InitGlobalDamagePct(0.f);
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

void UROHAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// 최대치가 내려가면 현재값도 따라 내림 (리스펙/버프 해제 등으로 MaxHealth·MaxMana 하락 시)
	if (Attribute == GetMaxHealthAttribute() && GetHealth() > NewValue)
	{
		SetHealth(FMath::Max(NewValue, 0.f));
	}
	else if (Attribute == GetMaxManaAttribute() && GetMana() > NewValue)
	{
		SetMana(FMath::Max(NewValue, 0.f));
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
