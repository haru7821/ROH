#include "Progression/ROHSkillTreeComponent.h"
#include "Progression/ROHProgressionComponent.h"
#include "Character/ROHAttributeSet.h"
#include "Abilities/Warrior/ROHAbility_Bash.h"
#include "Abilities/Warrior/ROHAbility_Whirlwind.h"
#include "Abilities/Warrior/ROHAbility_LeapAttack.h"
#include "Abilities/Warrior/ROHAbility_Execute.h"
#include "Abilities/Warrior/ROHAbility_BattleShout.h"
#include "Abilities/Elementalist/ROHElementalistAbilities.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "GameFramework/Actor.h"
#include "ROH.h"
#include <initializer_list>

namespace
{
	FROHSkillDef MakeActive(FName Id, const TCHAR* Name, const TCHAR* Tree, EROHPlayerClass PlayerClass,
		int32 ReqLevel, FName Prereq, TSubclassOf<UROHGameplayAbility> Ability,
		std::initializer_list<FROHSkillSynergy> Synergies)
	{
		FROHSkillDef Def;
		Def.SkillId = Id;
		Def.DisplayName = FText::FromString(Name);
		Def.TreeName = FText::FromString(Tree);
		Def.PlayerClass = PlayerClass;
		Def.Kind = EROHSkillKind::Active;
		Def.RequiredLevel = ReqLevel;
		Def.PrereqSkillId = Prereq;
		Def.AbilityClass = Ability;
		Def.Synergies = Synergies;
		return Def;
	}

	FROHSkillDef MakePassive(FName Id, const TCHAR* Name, const TCHAR* Tree, EROHPlayerClass PlayerClass,
		int32 ReqLevel, FName Prereq, std::initializer_list<FROHPassiveBonus> Bonuses)
	{
		FROHSkillDef Def;
		Def.SkillId = Id;
		Def.DisplayName = FText::FromString(Name);
		Def.TreeName = FText::FromString(Tree);
		Def.PlayerClass = PlayerClass;
		Def.Kind = EROHSkillKind::Passive;
		Def.RequiredLevel = ReqLevel;
		Def.PrereqSkillId = Prereq;
		Def.PassiveBonuses = Bonuses;
		return Def;
	}

	FROHSkillSynergy Syn(FName Id, float Pct = 8.f)
	{
		FROHSkillSynergy Synergy;
		Synergy.SkillId = Id;
		Synergy.PerPointPercent = Pct;
		return Synergy;
	}

	FROHPassiveBonus Bonus(const FGameplayAttribute& Attribute, float PerRank)
	{
		FROHPassiveBonus Result;
		Result.Attribute = Attribute;
		Result.PerRank = PerRank;
		return Result;
	}
}

const TArray<FROHSkillDef>& UROHSkillTreeComponent::GetSkillDefs(EROHPlayerClass InPlayerClass)
{
	// M3 2차: 클래스당 10스킬 × 3계열 (목표 24~30은 M4 이후 증분 — docs/02 §2.1)
	// 티어: 요구 레벨 1 / 6 / 12 / 18 / 24
	static const TArray<FROHSkillDef> WarriorSkills = {
		// --- 무기술 (Arms) ---
		MakeActive(TEXT("Bash"), TEXT("강타"), TEXT("무기술"), EROHPlayerClass::Warrior, 1, NAME_None,
			UROHAbility_Bash::StaticClass(), { Syn(TEXT("Whirlwind")), Syn(TEXT("Execute")) }),
		MakeActive(TEXT("Whirlwind"), TEXT("회전베기"), TEXT("무기술"), EROHPlayerClass::Warrior, 6, TEXT("Bash"),
			UROHAbility_Whirlwind::StaticClass(), { Syn(TEXT("Bash")), Syn(TEXT("Execute")) }),
		MakeActive(TEXT("Leap"), TEXT("도약공격"), TEXT("무기술"), EROHPlayerClass::Warrior, 12, TEXT("Whirlwind"),
			UROHAbility_LeapAttack::StaticClass(), { Syn(TEXT("Bash")), Syn(TEXT("Whirlwind")) }),
		MakeActive(TEXT("Execute"), TEXT("처형"), TEXT("무기술"), EROHPlayerClass::Warrior, 18, TEXT("Bash"),
			UROHAbility_Execute::StaticClass(), { Syn(TEXT("Bash"), 10.f), Syn(TEXT("Whirlwind")) }),

		// --- 전장의 함성 (Warcries) ---
		MakeActive(TEXT("BattleShout"), TEXT("전투의 함성"), TEXT("전장의 함성"), EROHPlayerClass::Warrior, 6, NAME_None,
			UROHAbility_BattleShout::StaticClass(), { Syn(TEXT("IronWill")) }),
		MakePassive(TEXT("IronWill"), TEXT("불굴"), TEXT("전장의 함성"), EROHPlayerClass::Warrior, 18, TEXT("BattleShout"),
			{ Bonus(UROHAttributeSet::GetFireResistanceAttribute(), 2.f),
			  Bonus(UROHAttributeSet::GetColdResistanceAttribute(), 2.f),
			  Bonus(UROHAttributeSet::GetLightningResistanceAttribute(), 2.f),
			  Bonus(UROHAttributeSet::GetPoisonResistanceAttribute(), 2.f),
			  Bonus(UROHAttributeSet::GetShadowResistanceAttribute(), 2.f),
			  Bonus(UROHAttributeSet::GetPhysicalResistanceAttribute(), 1.f) }), // PhysicalResistance = PDR (docs/10 §4.3)

		// --- 투지 (Fortitude) ---
		MakePassive(TEXT("WeaponMastery"), TEXT("무기 숙련"), TEXT("투지"), EROHPlayerClass::Warrior, 1, NAME_None,
			{ Bonus(UROHAttributeSet::GetAttackPowerAttribute(), 2.f) }),
		MakePassive(TEXT("IronSkin"), TEXT("철벽"), TEXT("투지"), EROHPlayerClass::Warrior, 6, NAME_None,
			{ Bonus(UROHAttributeSet::GetDefenseAttribute(), 6.f) }),
		MakePassive(TEXT("Toughness"), TEXT("강인함"), TEXT("투지"), EROHPlayerClass::Warrior, 12, TEXT("IronSkin"),
			{ Bonus(UROHAttributeSet::GetMaxHealthAttribute(), 8.f) }),
		MakePassive(TEXT("BattleFocus"), TEXT("전투 감각"), TEXT("투지"), EROHPlayerClass::Warrior, 24, TEXT("WeaponMastery"),
			{ Bonus(UROHAttributeSet::GetAttackRatingAttribute(), 25.f) }),
	};

	static const TArray<FROHSkillDef> ElementalistSkills = {
		// --- 화염 (Fire) ---
		MakeActive(TEXT("Fireball"), TEXT("화염구"), TEXT("화염"), EROHPlayerClass::Elementalist, 1, NAME_None,
			UROHAbility_Fireball::StaticClass(), { Syn(TEXT("Ignite"), 16.f), Syn(TEXT("Meteor"), 16.f) }),
		MakePassive(TEXT("Ignite"), TEXT("발화"), TEXT("화염"), EROHPlayerClass::Elementalist, 6, TEXT("Fireball"),
			{ Bonus(UROHAttributeSet::GetEnergyAttribute(), 2.f) }),
		MakeActive(TEXT("Meteor"), TEXT("운석"), TEXT("화염"), EROHPlayerClass::Elementalist, 18, TEXT("Fireball"),
			UROHAbility_Meteor::StaticClass(), { Syn(TEXT("Fireball")), Syn(TEXT("Ignite")) }),

		// --- 냉기 (Cold) ---
		MakeActive(TEXT("IceBolt"), TEXT("얼음화살"), TEXT("냉기"), EROHPlayerClass::Elementalist, 1, NAME_None,
			UROHAbility_IceBolt::StaticClass(), { Syn(TEXT("FrostNova")), Syn(TEXT("Blizzard")) }),
		MakeActive(TEXT("FrostNova"), TEXT("서리 신성"), TEXT("냉기"), EROHPlayerClass::Elementalist, 6, NAME_None,
			UROHAbility_FrostNova::StaticClass(), { Syn(TEXT("IceBolt")), Syn(TEXT("Blizzard")) }),
		MakePassive(TEXT("IceArmor"), TEXT("빙갑"), TEXT("냉기"), EROHPlayerClass::Elementalist, 12, NAME_None,
			{ Bonus(UROHAttributeSet::GetDefenseAttribute(), 8.f) }),
		MakeActive(TEXT("Blizzard"), TEXT("눈보라"), TEXT("냉기"), EROHPlayerClass::Elementalist, 24, TEXT("FrostNova"),
			UROHAbility_Blizzard::StaticClass(), { Syn(TEXT("IceBolt")), Syn(TEXT("FrostNova")) }),

		// --- 번개 (Lightning) ---
		MakePassive(TEXT("EnergyFocus"), TEXT("에너지 집중"), TEXT("번개"), EROHPlayerClass::Elementalist, 1, NAME_None,
			{ Bonus(UROHAttributeSet::GetMaxManaAttribute(), 5.f) }),
		MakeActive(TEXT("Teleport"), TEXT("텔레포트"), TEXT("번개"), EROHPlayerClass::Elementalist, 12, NAME_None,
			UROHAbility_Teleport::StaticClass(), {}),
		MakeActive(TEXT("StaticField"), TEXT("정전기장"), TEXT("번개"), EROHPlayerClass::Elementalist, 18, NAME_None,
			UROHAbility_StaticField::StaticClass(), {}),
	};

	return InPlayerClass == EROHPlayerClass::Warrior ? WarriorSkills : ElementalistSkills;
}

const FROHSkillDef* UROHSkillTreeComponent::FindSkillDef(EROHPlayerClass InPlayerClass, FName SkillId)
{
	for (const FROHSkillDef& Def : GetSkillDefs(InPlayerClass))
	{
		if (Def.SkillId == SkillId)
		{
			return &Def;
		}
	}
	return nullptr;
}

UROHProgressionComponent* UROHSkillTreeComponent::GetProgression() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UROHProgressionComponent>() : nullptr;
}

UAbilitySystemComponent* UROHSkillTreeComponent::GetOwnerASC() const
{
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		return ASI->GetAbilitySystemComponent();
	}
	return nullptr;
}

int32 UROHSkillTreeComponent::GetRank(FName SkillId) const
{
	const int32* Found = HardPoints.Find(SkillId);
	return Found ? *Found : 0;
}

bool UROHSkillTreeComponent::InvestPoint(FName SkillId, FString& OutError)
{
	const FROHSkillDef* Def = FindSkillDef(PlayerClass, SkillId);
	if (!Def)
	{
		OutError = TEXT("알 수 없는 스킬입니다. ROHSkillInfo로 목록을 확인하세요.");
		return false;
	}

	UROHProgressionComponent* Progression = GetProgression();
	if (!Progression)
	{
		OutError = TEXT("성장 컴포넌트가 없습니다.");
		return false;
	}
	if (Progression->GetLevel() < Def->RequiredLevel)
	{
		OutError = FString::Printf(TEXT("레벨 %d 필요 (현재 %d)"), Def->RequiredLevel, Progression->GetLevel());
		return false;
	}
	if (!Def->PrereqSkillId.IsNone() && GetRank(Def->PrereqSkillId) < 1)
	{
		OutError = FString::Printf(TEXT("선행 스킬 필요: %s"), *Def->PrereqSkillId.ToString());
		return false;
	}
	if (GetRank(SkillId) >= Def->MaxPoints)
	{
		OutError = TEXT("이미 최대 랭크입니다.");
		return false;
	}
	if (!Progression->SpendSkillPoint())
	{
		OutError = TEXT("스킬 포인트가 없습니다. 레벨업으로 획득하세요.");
		return false;
	}

	++HardPoints.FindOrAdd(SkillId);

	if (Def->Kind == EROHSkillKind::Passive)
	{
		RefreshPassiveEffect(*Def);
	}
	return true;
}

float UROHSkillTreeComponent::GetDamageMultiplier(FName SkillId) const
{
	const int32 Rank = GetRank(SkillId);
	if (Rank <= 0)
	{
		return 0.f;
	}

	float Multiplier = 1.f + 0.12f * (Rank - 1);

	if (const FROHSkillDef* Def = FindSkillDef(PlayerClass, SkillId))
	{
		for (const FROHSkillSynergy& Synergy : Def->Synergies)
		{
			Multiplier += GetRank(Synergy.SkillId) * Synergy.PerPointPercent * 0.01f;
		}
	}
	return Multiplier;
}

int32 UROHSkillTreeComponent::ResetAllPoints()
{
	int32 Total = 0;
	for (const auto& Pair : HardPoints)
	{
		Total += Pair.Value;
	}
	if (Total <= 0)
	{
		return 0;
	}

	RemoveAllPassiveEffects();
	HardPoints.Empty();

	if (UROHProgressionComponent* Progression = GetProgression())
	{
		for (int32 i = 0; i < Total; ++i)
		{
			Progression->RefundSkillPoint();
		}
	}
	return Total;
}

void UROHSkillTreeComponent::RestoreState(const TMap<FName, int32>& InHardPoints)
{
	// 세이브 로드: 새 폰 기준이라 기존 패시브는 없지만, 재호출 안전을 위해 정리 후 재적용
	RemoveAllPassiveEffects();
	HardPoints = InHardPoints;

	for (const FROHSkillDef& Def : GetSkillDefs(PlayerClass))
	{
		if (Def.Kind == EROHSkillKind::Passive && GetRank(Def.SkillId) > 0)
		{
			RefreshPassiveEffect(Def);
		}
	}
}

void UROHSkillTreeComponent::RefreshPassiveEffect(const FROHSkillDef& Def)
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		UE_LOG(LogROH, Warning, TEXT("패시브 적용 실패 (ASC 없음): %s"), *Def.SkillId.ToString());
		return;
	}

	if (const FActiveGameplayEffectHandle* OldHandle = PassiveEffectHandles.Find(Def.SkillId))
	{
		ASC->RemoveActiveGameplayEffect(*OldHandle);
		PassiveEffectHandles.Remove(Def.SkillId);
	}

	const int32 Rank = GetRank(Def.SkillId);
	if (Rank <= 0 || Def.PassiveBonuses.IsEmpty())
	{
		return;
	}

	// 장비 GE와 동일 패턴: 런타임 무한 GE, 이름 자동 유니크 (Items/ROHInventoryComponent 참고)
	UGameplayEffect* PassiveEffect = NewObject<UGameplayEffect>(GetTransientPackage());
	PassiveEffect->DurationPolicy = EGameplayEffectDurationType::Infinite;

	for (const FROHPassiveBonus& PassiveBonus : Def.PassiveBonuses)
	{
		if (!PassiveBonus.Attribute.IsValid() || FMath::IsNearlyZero(PassiveBonus.PerRank))
		{
			continue;
		}
		FGameplayModifierInfo Modifier;
		Modifier.Attribute = PassiveBonus.Attribute;
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(PassiveBonus.PerRank * Rank));
		PassiveEffect->Modifiers.Add(Modifier);
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);
	const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectToSelf(PassiveEffect, 1.f, Context);
	PassiveEffectHandles.Add(Def.SkillId, Handle);
}

void UROHSkillTreeComponent::RemoveAllPassiveEffects()
{
	if (UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		for (const auto& Pair : PassiveEffectHandles)
		{
			ASC->RemoveActiveGameplayEffect(Pair.Value);
		}
	}
	PassiveEffectHandles.Empty();
}
