#include "Progression/ROHSkillTreeComponent.h"
#include "Progression/ROHProgressionComponent.h"
#include "GameFramework/Actor.h"
#include <initializer_list>

namespace
{
	FROHSkillDef MakeSkill(FName Id, const TCHAR* Name, EROHPlayerClass PlayerClass,
		int32 ReqLevel, FName Prereq, std::initializer_list<FROHSkillSynergy> Synergies)
	{
		FROHSkillDef Def;
		Def.SkillId = Id;
		Def.DisplayName = FText::FromString(Name);
		Def.PlayerClass = PlayerClass;
		Def.RequiredLevel = ReqLevel;
		Def.PrereqSkillId = Prereq;
		Def.Synergies = Synergies;
		return Def;
	}

	FROHSkillSynergy Syn(FName Id, float Pct = 8.f)
	{
		FROHSkillSynergy Synergy;
		Synergy.SkillId = Id;
		Synergy.PerPointPercent = Pct;
		return Synergy;
	}
}

const TArray<FROHSkillDef>& UROHSkillTreeComponent::GetSkillDefs(EROHPlayerClass PlayerClass)
{
	// M3 1차: 클래스당 3스킬 (트리 전체 확장은 M3 후속 — docs/02 §2.1)
	static const TArray<FROHSkillDef> WarriorSkills = {
		MakeSkill(TEXT("Bash"), TEXT("강타"), EROHPlayerClass::Warrior, 1, NAME_None,
			{ Syn(TEXT("Whirlwind")) }),
		MakeSkill(TEXT("Whirlwind"), TEXT("회전베기"), EROHPlayerClass::Warrior, 6, TEXT("Bash"),
			{ Syn(TEXT("Bash")) }),
		MakeSkill(TEXT("Leap"), TEXT("도약공격"), EROHPlayerClass::Warrior, 12, TEXT("Whirlwind"),
			{ Syn(TEXT("Bash")), Syn(TEXT("Whirlwind")) }),
	};

	static const TArray<FROHSkillDef> ElementalistSkills = {
		MakeSkill(TEXT("Fireball"), TEXT("화염구"), EROHPlayerClass::Elementalist, 1, NAME_None,
			{ Syn(TEXT("FrostNova")) }),
		MakeSkill(TEXT("FrostNova"), TEXT("서리 신성"), EROHPlayerClass::Elementalist, 6, NAME_None,
			{ Syn(TEXT("Fireball")) }),
		MakeSkill(TEXT("Teleport"), TEXT("텔레포트"), EROHPlayerClass::Elementalist, 12, NAME_None, {}),
	};

	return PlayerClass == EROHPlayerClass::Warrior ? WarriorSkills : ElementalistSkills;
}

const FROHSkillDef* UROHSkillTreeComponent::FindSkillDef(EROHPlayerClass PlayerClass, FName SkillId)
{
	for (const FROHSkillDef& Def : GetSkillDefs(PlayerClass))
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
