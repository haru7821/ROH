#include "Character/ROHMonsterCharacter.h"
#include "Abilities/Monster/ROHAbility_MonsterAttack.h"
#include "AI/ROHMonsterAIController.h"
#include "Character/ROHAttributeSet.h"
#include "Items/ROHItemDatabase.h"
#include "Loot/ROHItemPickup.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

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

	DropLoot(Killer);
	DetachFromControllerPendingDestroy();
	SetLifeSpan(CorpseLifetime);
}

void AROHMonsterCharacter::DropLoot(AActor* Killer)
{
	UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UROHItemDatabase* Database = GameInstance ? GameInstance->GetSubsystem<UROHItemDatabase>() : nullptr;
	if (!Database || TreasureClassId.IsNone())
	{
		return;
	}

	// 처치자의 MF 적용 (docs/02 §3.4)
	float MagicFind = 0.f;
	if (const IAbilitySystemInterface* KillerASI = Cast<IAbilitySystemInterface>(Killer))
	{
		if (const UAbilitySystemComponent* KillerASC = KillerASI->GetAbilitySystemComponent())
		{
			MagicFind = KillerASC->GetNumericAttribute(UROHAttributeSet::GetMagicFindAttribute());
		}
	}

	const int32 ItemLevel = AttributeSet ? FMath::RoundToInt(AttributeSet->GetCharacterLevel()) : 1;
	const FROHDropResult Drops = Database->RollTreasureClass(TreasureClassId, ItemLevel, MagicFind);

	int32 SpawnIndex = 0;
	auto NextDropLocation = [this, &SpawnIndex]()
	{
		// 시체 주위로 원형 산개
		const float Angle = SpawnIndex * 137.5f; // 황금각: 겹침 최소화
		++SpawnIndex;
		const float Radius = 80.f + 30.f * SpawnIndex;
		return GetActorLocation() + FVector(
			FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
			FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
			0.f);
	};

	// 지연 스폰: 내용물 설정 후 FinishSpawning — 스폰 순간 겹쳐 있던 플레이어의 오버랩이
	// 초기화 전에 발화해 습득 불가로 남는 문제 방지
	for (const FROHItemInstance& Item : Drops.Items)
	{
		const FTransform SpawnTransform(FRotator::ZeroRotator, NextDropLocation());
		if (AROHItemPickup* Pickup = World->SpawnActorDeferred<AROHItemPickup>(AROHItemPickup::StaticClass(), SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
		{
			Pickup->InitAsItem(Item);
			Pickup->FinishSpawning(SpawnTransform);
		}
	}
	if (Drops.Gold > 0)
	{
		const FTransform SpawnTransform(FRotator::ZeroRotator, NextDropLocation());
		if (AROHItemPickup* Pickup = World->SpawnActorDeferred<AROHItemPickup>(AROHItemPickup::StaticClass(), SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
		{
			Pickup->InitAsGold(Drops.Gold);
			Pickup->FinishSpawning(SpawnTransform);
		}
	}
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
