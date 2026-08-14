#include "Progression/ROHProgressionComponent.h"
#include "Character/ROHCharacterBase.h"
#include "Character/ROHAttributeSet.h"
#include "Engine/Engine.h"
#include "ROH.h"

UROHAttributeSet* UROHProgressionComponent::GetAttributeSet() const
{
	const AROHCharacterBase* OwnerCharacter = Cast<AROHCharacterBase>(GetOwner());
	return OwnerCharacter ? OwnerCharacter->GetAttributeSet() : nullptr;
}

int32 UROHProgressionComponent::XPForNextLevel(int32 InLevel)
{
	// 레벨 1→2: 100, 이후 완만한 지수 증가 (docs/01: 노말 클리어 ≈ 레벨 25~30)
	return FMath::RoundToInt(100.f * FMath::Pow(static_cast<float>(InLevel), 1.5f));
}

void UROHProgressionComponent::GrantXP(int32 Amount)
{
	if (Amount <= 0 || Level >= MaxLevel)
	{
		return;
	}

	XP += Amount;
	while (Level < MaxLevel && XP >= XPForNextLevel(Level))
	{
		XP -= XPForNextLevel(Level);
		LevelUp();
	}
	if (Level >= MaxLevel)
	{
		XP = 0; // 정복자 전환은 M5
	}
}

void UROHProgressionComponent::LevelUp()
{
	++Level;
	StatPoints += 5;
	SkillPoints += 1;

	if (UROHAttributeSet* Attributes = GetAttributeSet())
	{
		Attributes->SetCharacterLevel(static_cast<float>(Level));
		// 레벨업 보너스: 완전 회복
		Attributes->SetHealth(Attributes->GetMaxHealth());
		Attributes->SetMana(Attributes->GetMaxMana());
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(3, 4.f, FColor::Emerald,
			FString::Printf(TEXT("레벨 업! Lv %d (스탯 +5, 스킬 +1 — 콘솔 ROHAllocStat / ROHSkillUp)"), Level));
	}
	UE_LOG(LogROH, Log, TEXT("레벨 업: %d"), Level);
}

bool UROHProgressionComponent::SpendSkillPoint()
{
	if (SkillPoints <= 0)
	{
		return false;
	}
	--SkillPoints;
	return true;
}

bool UROHProgressionComponent::AllocateStat(FName StatName)
{
	if (StatPoints <= 0)
	{
		return false;
	}

	static const FName ValidStats[] = { TEXT("Strength"), TEXT("Dexterity"), TEXT("Vitality"), TEXT("Energy") };
	bool bValid = false;
	for (const FName& Valid : ValidStats)
	{
		if (StatName == Valid)
		{
			bValid = true;
			break;
		}
	}
	if (!bValid)
	{
		return false;
	}

	--StatPoints;
	++AllocatedStats.FindOrAdd(StatName);
	ApplyStatToAttributes(StatName, 1);
	return true;
}

void UROHProgressionComponent::ApplyStatToAttributes(FName StatName, int32 Points) const
{
	UROHAttributeSet* Attributes = GetAttributeSet();
	if (!Attributes || Points <= 0)
	{
		return;
	}
	const float Delta = static_cast<float>(Points);

	// 파생 공식은 docs/02 §1.3 / CharacterBase::InitializeAttributes와 일치시킬 것
	if (StatName == TEXT("Strength"))
	{
		Attributes->SetStrength(Attributes->GetStrength() + Delta);
	}
	else if (StatName == TEXT("Dexterity"))
	{
		Attributes->SetDexterity(Attributes->GetDexterity() + Delta);
		Attributes->SetAttackRating(Attributes->GetAttackRating() + 5.f * Delta);
		Attributes->SetDefense(Attributes->GetDefense() + 2.f * Delta);
	}
	else if (StatName == TEXT("Vitality"))
	{
		Attributes->SetVitality(Attributes->GetVitality() + Delta);
		Attributes->SetMaxHealth(Attributes->GetMaxHealth() + 4.f * Delta);
		Attributes->SetHealth(Attributes->GetHealth() + 4.f * Delta);
	}
	else if (StatName == TEXT("Energy"))
	{
		Attributes->SetEnergy(Attributes->GetEnergy() + Delta);
		Attributes->SetMaxMana(Attributes->GetMaxMana() + 2.f * Delta);
		Attributes->SetMana(Attributes->GetMana() + 2.f * Delta);
	}
}

void UROHProgressionComponent::RestoreState(int32 InLevel, int32 InXP, int32 InStatPoints, int32 InSkillPoints, const TMap<FName, int32>& InAllocated)
{
	Level = FMath::Clamp(InLevel, 1, MaxLevel);
	XP = FMath::Max(0, InXP);
	StatPoints = FMath::Max(0, InStatPoints);
	SkillPoints = FMath::Max(0, InSkillPoints);
	AllocatedStats = InAllocated;

	if (UROHAttributeSet* Attributes = GetAttributeSet())
	{
		Attributes->SetCharacterLevel(static_cast<float>(Level));
	}
	// 분배 내역 재적용 (InitializeAttributes 직후 호출 전제)
	for (const auto& Pair : AllocatedStats)
	{
		ApplyStatToAttributes(Pair.Key, Pair.Value);
	}
}
