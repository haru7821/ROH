#include "Character/ROHPlayerClasses.h"
#include "Progression/ROHSkillTreeComponent.h"
#include "Abilities/Warrior/ROHAbility_BasicAttack.h"
#include "Abilities/Warrior/ROHAbility_Bash.h"
#include "Abilities/Warrior/ROHAbility_Whirlwind.h"
#include "Abilities/Warrior/ROHAbility_LeapAttack.h"
#include "Abilities/Elementalist/ROHElementalistAbilities.h"

AROHWarriorCharacter::AROHWarriorCharacter()
{
	// 전사 프리셋 (docs/02 §1.1)
	BaseStrength = 30.f;
	BaseDexterity = 20.f;
	BaseVitality = 25.f;
	BaseEnergy = 10.f;
	BaseMaxHealth = 50.f; // 최종 생명력 = 50 + 25×4 = 150

	DefaultAbilities.Add(UROHAbility_BasicAttack::StaticClass());
	DefaultAbilities.Add(UROHAbility_Bash::StaticClass());
	DefaultAbilities.Add(UROHAbility_Whirlwind::StaticClass());
	DefaultAbilities.Add(UROHAbility_LeapAttack::StaticClass());

	GetSkillTree()->SetPlayerClass(EROHPlayerClass::Warrior);
}

AROHElementalistCharacter::AROHElementalistCharacter()
{
	// 원소술사 프리셋 (docs/02 §1.2) — 유리대포
	BaseStrength = 10.f;
	BaseDexterity = 15.f;
	BaseVitality = 15.f;
	BaseEnergy = 30.f;
	BaseMaxHealth = 40.f; // 최종 생명력 = 40 + 15×4 = 100

	DefaultAbilities.Add(UROHAbility_MagicBolt::StaticClass());
	DefaultAbilities.Add(UROHAbility_Fireball::StaticClass());
	DefaultAbilities.Add(UROHAbility_FrostNova::StaticClass());
	DefaultAbilities.Add(UROHAbility_Teleport::StaticClass());

	GetSkillTree()->SetPlayerClass(EROHPlayerClass::Elementalist);
}
