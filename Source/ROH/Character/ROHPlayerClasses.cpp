#include "Character/ROHPlayerClasses.h"
#include "Progression/ROHSkillTreeComponent.h"
#include "Abilities/Warrior/ROHAbility_BasicAttack.h"
#include "Abilities/Warrior/ROHAbility_Bash.h"
#include "Abilities/Warrior/ROHAbility_Whirlwind.h"
#include "Abilities/Warrior/ROHAbility_LeapAttack.h"
#include "Abilities/Warrior/ROHAbility_Execute.h"
#include "Abilities/Elementalist/ROHElementalistAbilities.h"

AROHWarriorCharacter::AROHWarriorCharacter()
{
	// 전사 프리셋 (docs/02 §1.1)
	BaseStrength = 30.f;
	BaseDexterity = 20.f;
	BaseVitality = 25.f;
	BaseEnergy = 10.f;
	// docs/10 §2: 레벨1 생명력 = 100 + 25×5 = 225, 마나 = 20 + 10×2 = 40
	BaseMaxHealth = 100.f;
	HealthPerLevel = 10.f;
	BaseMaxMana = 20.f;
	ManaPerLevel = 2.f;

	DefaultAbilities.Add(UROHAbility_BasicAttack::StaticClass());
	DefaultAbilities.Add(UROHAbility_Bash::StaticClass());
	DefaultAbilities.Add(UROHAbility_Whirlwind::StaticClass());
	DefaultAbilities.Add(UROHAbility_LeapAttack::StaticClass());
	DefaultAbilities.Add(UROHAbility_Execute::StaticClass()); // 슬롯4 기본 (ROHBindSkill로 교체 가능)

	GetSkillTree()->SetPlayerClass(EROHPlayerClass::Warrior);
}

AROHElementalistCharacter::AROHElementalistCharacter()
{
	// 원소술사 프리셋 (docs/02 §1.2) — 유리대포
	BaseStrength = 10.f;
	BaseDexterity = 15.f;
	BaseVitality = 15.f;
	BaseEnergy = 30.f;
	// docs/10 §2: 레벨1 생명력 = 60 + 15×5 = 135, 마나 = 50 + 30×2 = 110
	BaseMaxHealth = 60.f;
	HealthPerLevel = 5.f;
	BaseMaxMana = 50.f;
	ManaPerLevel = 5.f;

	DefaultAbilities.Add(UROHAbility_MagicBolt::StaticClass());
	DefaultAbilities.Add(UROHAbility_Fireball::StaticClass());
	DefaultAbilities.Add(UROHAbility_FrostNova::StaticClass());
	DefaultAbilities.Add(UROHAbility_Teleport::StaticClass());
	DefaultAbilities.Add(UROHAbility_Meteor::StaticClass()); // 슬롯4 기본 (ROHBindSkill로 교체 가능)

	GetSkillTree()->SetPlayerClass(EROHPlayerClass::Elementalist);
}
