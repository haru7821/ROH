#include "Progression/ROHProgressionComponent.h"
#include "Character/ROHCharacterBase.h"
#include "Character/ROHAttributeSet.h"
#include "Save/ROHAccountSubsystem.h"
#include "AbilitySystemComponent.h" // SetNumericAttributeBase / GetNumericAttributeBase
#include "Engine/Engine.h"
#include "Engine/GameInstance.h" // GetSubsystem<T>() 템플릿 인스턴스화에 완전한 타입 필요
#include "ROH.h"

namespace
{
	/**
	 * 성장/분배는 반드시 BaseValue 기준으로 가감한다 (Sup b29 지적).
	 * Set(Get()+Δ)는 CurrentValue(장비 Flat·% 배율 GE 적용 후)를 Base에 되써서
	 * 장비 보정이 베이스에 눌어붙고, % 배율 GE가 걸려 있으면 레벨업마다 지수로 불어난다.
	 */
	void ProgressionAddToBase(UROHAttributeSet* Attributes, const FGameplayAttribute& Attribute, float Delta)
	{
		if (UAbilitySystemComponent* ASC = Attributes ? Attributes->GetOwningAbilitySystemComponent() : nullptr)
		{
			ASC->SetNumericAttributeBase(Attribute, ASC->GetNumericAttributeBase(Attribute) + Delta);
		}
	}
}

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
	if (Amount <= 0)
	{
		return;
	}

	// 만렙: 경험치 전액을 정복자로 (M5 최종 — 계정 공유 성장)
	if (Level >= MaxLevel)
	{
		RouteToParagon(Amount);
		return;
	}

	XP += Amount;
	while (Level < MaxLevel && XP >= XPForNextLevel(Level))
	{
		XP -= XPForNextLevel(Level);
		LevelUp();
	}
	// 만렙 도달 프레임의 초과분도 정복자로 이월
	if (Level >= MaxLevel && XP > 0)
	{
		RouteToParagon(XP);
		XP = 0;
	}
}

void UROHProgressionComponent::RouteToParagon(int32 Amount) const
{
	const AActor* Owner = GetOwner();
	UGameInstance* GameInstance = Owner ? Owner->GetGameInstance() : nullptr;
	if (UROHAccountSubsystem* Account = GameInstance ? GameInstance->GetSubsystem<UROHAccountSubsystem>() : nullptr)
	{
		Account->GrantParagonXP(Amount);
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

		// 레벨당 HP/MP 성장 (docs/10 §2 — 클래스 프리셋 HealthPerLevel/ManaPerLevel)
		if (const AROHCharacterBase* OwnerCharacter = Cast<AROHCharacterBase>(GetOwner()))
		{
			ProgressionAddToBase(Attributes, UROHAttributeSet::GetMaxHealthAttribute(), OwnerCharacter->GetHealthPerLevel());
			ProgressionAddToBase(Attributes, UROHAttributeSet::GetMaxManaAttribute(), OwnerCharacter->GetManaPerLevel());
		}

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

	// 파생 공식은 docs/10 §2 / CharacterBase::InitializeAttributes와 일치시킬 것
	if (StatName == TEXT("Strength"))
	{
		ProgressionAddToBase(Attributes, UROHAttributeSet::GetStrengthAttribute(), Delta);
	}
	else if (StatName == TEXT("Dexterity"))
	{
		ProgressionAddToBase(Attributes, UROHAttributeSet::GetDexterityAttribute(), Delta);
		ProgressionAddToBase(Attributes, UROHAttributeSet::GetAttackRatingAttribute(), 5.f * Delta);
		ProgressionAddToBase(Attributes, UROHAttributeSet::GetDefenseAttribute(), 2.f * Delta);
	}
	else if (StatName == TEXT("Vitality"))
	{
		ProgressionAddToBase(Attributes, UROHAttributeSet::GetVitalityAttribute(), Delta);
		ProgressionAddToBase(Attributes, UROHAttributeSet::GetMaxHealthAttribute(), 5.f * Delta);
		ProgressionAddToBase(Attributes, UROHAttributeSet::GetHealthAttribute(), 5.f * Delta);
	}
	else if (StatName == TEXT("Energy"))
	{
		ProgressionAddToBase(Attributes, UROHAttributeSet::GetEnergyAttribute(), Delta);
		ProgressionAddToBase(Attributes, UROHAttributeSet::GetMaxManaAttribute(), 2.f * Delta);
		ProgressionAddToBase(Attributes, UROHAttributeSet::GetManaAttribute(), 2.f * Delta);
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

		// 레벨 성장분 재적용 (docs/10 §2): 새 폰은 레벨1 기준으로 초기화되어 있다.
		// Health/Mana도 함께 올려 기존 복원 결과(분배 재적용 후 사실상 만피)와 동일하게 유지
		if (const AROHCharacterBase* OwnerCharacter = Cast<AROHCharacterBase>(GetOwner()))
		{
			const float LevelDelta = static_cast<float>(Level - 1);
			ProgressionAddToBase(Attributes, UROHAttributeSet::GetMaxHealthAttribute(), LevelDelta * OwnerCharacter->GetHealthPerLevel());
			ProgressionAddToBase(Attributes, UROHAttributeSet::GetHealthAttribute(), LevelDelta * OwnerCharacter->GetHealthPerLevel());
			ProgressionAddToBase(Attributes, UROHAttributeSet::GetMaxManaAttribute(), LevelDelta * OwnerCharacter->GetManaPerLevel());
			ProgressionAddToBase(Attributes, UROHAttributeSet::GetManaAttribute(), LevelDelta * OwnerCharacter->GetManaPerLevel());
		}
	}
	// 분배 내역 재적용 (InitializeAttributes 직후 호출 전제)
	for (const auto& Pair : AllocatedStats)
	{
		ApplyStatToAttributes(Pair.Key, Pair.Value);
	}
}
