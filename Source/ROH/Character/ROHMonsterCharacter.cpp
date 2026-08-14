#include "Character/ROHMonsterCharacter.h"
#include "Abilities/Monster/ROHAbility_MonsterAttack.h"
#include "AI/ROHMonsterAIController.h"
#include "Character/ROHAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"

AROHMonsterCharacter::AROHMonsterCharacter()
{
	TeamId = 1;
	AIControllerClass = AROHMonsterAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AttackAbility = UROHAbility_MonsterMelee::StaticClass();

	BaseMaxHealth = 80.f;
	BaseMoveSpeed = 400.f;
}

void AROHMonsterCharacter::HandleDeath(AActor* Killer)
{
	if (!IsAlive())
	{
		return;
	}
	Super::HandleDeath(Killer);

	DetachFromControllerPendingDestroy();
	SetLifeSpan(CorpseLifetime);
}

AROHMonster_Grunt::AROHMonster_Grunt()
{
	BaseMaxHealth = 80.f;
	BaseMoveSpeed = 400.f;
	AttackDamage = 10.f;
	AttackRange = 180.f;
	AttackInterval = 1.5f;
}

AROHMonster_Archer::AROHMonster_Archer()
{
	BaseMaxHealth = 50.f;
	BaseMoveSpeed = 350.f;
	AttackDamage = 8.f;
	AttackRange = 900.f;
	PreferredRange = 700.f;
	AttackInterval = 2.f;
	AttackAbility = UROHAbility_MonsterRanged::StaticClass();
}

AROHMonster_Charger::AROHMonster_Charger()
{
	BaseMaxHealth = 60.f;
	BaseMoveSpeed = 650.f;
	AttackDamage = 15.f;
	AttackRange = 200.f;
	AttackInterval = 1.2f;
}
