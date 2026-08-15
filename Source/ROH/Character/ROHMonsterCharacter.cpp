#include "Character/ROHMonsterCharacter.h"
#include "Abilities/Monster/ROHAbility_MonsterAttack.h"
#include "Abilities/ROHAbilitySystemComponent.h" // TObjectPtr 멤버 호출에 완전한 타입 필요
#include "Campaign/ROHCampaignSubsystem.h"
#include "AI/ROHMonsterAIController.h"
#include "Character/ROHAttributeSet.h"
#include "Character/ROHBossCharacter.h"
#include "Items/ROHItemDatabase.h"
#include "Loot/ROHItemPickup.h"
#include "Progression/ROHProgressionComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
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

void AROHMonsterCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// AttackAbility는 DefaultAbilities 목록 밖에서 지정되므로 별도 부여 필요
	if (AttackAbility && AbilitySystemComponent && !AbilitySystemComponent->FindAbilitySpecFromClass(AttackAbility))
	{
		GrantAbility(AttackAbility);
	}

	// 난이도 스케일링 (어트리뷰트 초기화 이후 1회 — docs/04 M4)
	if (!bDifficultyScaled && AttributeSet && GetGameInstance())
	{
		bDifficultyScaled = true;
		if (const UROHCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UROHCampaignSubsystem>())
		{
			const FROHDifficultyParams Params = UROHCampaignSubsystem::GetDifficultyParams(Campaign->GetDifficulty());
			AttributeSet->SetMaxHealth(AttributeSet->GetMaxHealth() * Params.HealthMult);
			AttributeSet->SetHealth(AttributeSet->GetMaxHealth());
			AttributeSet->SetCharacterLevel(AttributeSet->GetCharacterLevel() + Params.MonsterLevelBonus); // 명중 + 드랍 ilvl
			AttributeSet->SetAttackRating(AttributeSet->GetAttackRating() * Params.AttackRatingMult);
			AttributeSet->SetDefense(AttributeSet->GetDefense() * Params.DefenseMult);
			AttackDamage *= Params.DamageMult;
			XPValue = FMath::RoundToInt(XPValue * Params.XPMult);
		}
	}
}

FROHDamageParams AROHMonsterCharacter::MakeAttackDamageParams() const
{
	FROHDamageParams DamageParams;
	switch (AttackDamageType)
	{
	case EROHDamageType::Fire:      DamageParams.FireDamage = AttackDamage; break;
	case EROHDamageType::Cold:      DamageParams.ColdDamage = AttackDamage; break;
	case EROHDamageType::Lightning: DamageParams.LightningDamage = AttackDamage; break;
	case EROHDamageType::Poison:    DamageParams.PoisonDamage = AttackDamage; break;
	case EROHDamageType::Shadow:    DamageParams.ShadowDamage = AttackDamage; break;
	default:                        DamageParams.PhysicalDamage = AttackDamage; break;
	}
	// 물리 공격만 명중 굴림, 원소 공격은 주문 취급으로 항상 명중 (docs/10 §5.3)
	DamageParams.bUseAttackRoll = (AttackDamageType == EROHDamageType::Physical);
	return DamageParams;
}

void AROHMonsterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 그레이박스 정예 표시: 발밑 링 (베이스의 몸통 색 규칙은 건드리지 않는다)
	// 정예 = 파랑, 고유 = 금색
	if (Rank != EROHMonsterRank::Normal && IsAlive())
	{
		const FColor RingColor = (Rank == EROHMonsterRank::Unique) ? FColor(255, 200, 0) : FColor(80, 140, 255);
		const float RingRadius = GetCapsuleComponent()->GetScaledCapsuleRadius() * 1.6f;
		const FVector FeetLocation = GetActorLocation()
			- FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 5.f);
		DrawDebugCircle(GetWorld(), FeetLocation, RingRadius, 24, RingColor, false, -1.f, 0, 3.f,
			FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
	}
}

void AROHMonsterCharacter::PromoteToRank(EROHMonsterRank NewRank)
{
	// 승급은 1회만. 보스는 자체 스탯/연출이 있어 제외 (docs/04 M4)
	if (Rank != EROHMonsterRank::Normal || NewRank == EROHMonsterRank::Normal
		|| IsA<AROHBossCharacter>() || !AttributeSet)
	{
		return;
	}
	Rank = NewRank;

	// PossessedBy의 난이도 스케일링이 끝난 값 위에 곱연산 — 배율이 난이도와 합성된다
	const bool bUnique = (NewRank == EROHMonsterRank::Unique);
	const float HealthMult = bUnique ? 5.f : 2.5f;
	const float DamageMult = bUnique ? 1.6f : 1.3f;
	const int32 XPMult = bUnique ? 6 : 3;

	AttributeSet->SetMaxHealth(AttributeSet->GetMaxHealth() * HealthMult);
	AttributeSet->SetHealth(AttributeSet->GetMaxHealth());
	AttributeSet->SetAttackRating(AttributeSet->GetAttackRating() * DamageMult);
	AttackDamage *= DamageMult;
	XPValue *= XPMult;

	if (bUnique)
	{
		AttributeSet->SetMoveSpeed(AttributeSet->GetMoveSpeed() + 60.f);
		// 어트리뷰트 변경 델리게이트가 발화하지 않는 경로 대비 직접 반영 (클램프 반영된 값 사용)
		GetCharacterMovement()->MaxWalkSpeed = AttributeSet->GetMoveSpeed();
	}

	SetActorScale3D(GetActorScale3D() * (bUnique ? 1.3f : 1.15f));
}

void AROHMonsterCharacter::HandleDeath(AActor* Killer)
{
	if (!IsAlive())
	{
		return;
	}
	Super::HandleDeath(Killer);

	// 처치자 경험치 지급 (docs/02 §2.1)
	if (const AROHCharacterBase* KillerCharacter = Cast<AROHCharacterBase>(Killer))
	{
		if (UROHProgressionComponent* Progression = KillerCharacter->FindComponentByClass<UROHProgressionComponent>())
		{
			Progression->GrantXP(XPValue);
		}
	}

	// 퀘스트 진행 집계 (docs/04 M4)
	if (GetGameInstance())
	{
		if (UROHCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UROHCampaignSubsystem>())
		{
			Campaign->NotifyMonsterKilled(this, Killer);
		}
	}

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

	// 정예 추가 굴림: 정예 2회, 고유 3회 + MF 보너스 (D2 방식 — docs/02 §3.4)
	int32 RollCount = 1;
	float RankMagicFindBonus = 0.f;
	if (Rank == EROHMonsterRank::Champion)
	{
		RollCount = 2;
	}
	else if (Rank == EROHMonsterRank::Unique)
	{
		RollCount = 3;
		RankMagicFindBonus = 100.f;
	}

	const int32 ItemLevel = AttributeSet ? FMath::RoundToInt(AttributeSet->GetCharacterLevel()) : 1;
	FROHDropResult Drops;
	for (int32 Roll = 0; Roll < RollCount; ++Roll)
	{
		const FROHDropResult RollResult = Database->RollTreasureClass(TreasureClassId, ItemLevel, MagicFind + RankMagicFindBonus);
		Drops.Items.Append(RollResult.Items);
		Drops.Gold += RollResult.Gold;
	}

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
	XPValue = 15;
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
	XPValue = 12;
}

AROHMonster_Charger::AROHMonster_Charger()
{
	BaseMaxHealth = 60.f;
	BaseMoveSpeed = 650.f;
	AttackDamage = 15.f;
	AttackRange = 200.f;
	AttackInterval = 1.2f;
	XPValue = 18;
}

AROHMonster_Brute::AROHMonster_Brute()
{
	BaseMaxHealth = 220.f;
	BaseMoveSpeed = 280.f;
	AttackDamage = 24.f;
	AttackRange = 220.f;
	AttackInterval = 2.2f;
	XPValue = 35;

	// 덩치 표현: 캡슐 확대 (그레이박스 메시는 캡슐에 맞춰 스케일됨)
	GetCapsuleComponent()->SetCapsuleSize(52.f, 110.f);
}

AROHMonster_Hexer::AROHMonster_Hexer()
{
	BaseMaxHealth = 45.f;
	BaseMoveSpeed = 330.f;
	AttackDamage = 12.f;
	AttackRange = 850.f;
	PreferredRange = 650.f;
	AttackInterval = 2.4f;
	AttackAbility = UROHAbility_MonsterRanged::StaticClass();
	AttackDamageType = EROHDamageType::Shadow; // 주술사: 그림자 속성 (docs/10 §5.3)
	XPValue = 20;
}

AROHMonster_Stalker::AROHMonster_Stalker()
{
	BaseMaxHealth = 55.f;
	BaseMoveSpeed = 700.f;
	AttackDamage = 12.f;
	AttackRange = 190.f;
	AttackInterval = 1.f;
	XPValue = 22;

	// 작고 빠른 실루엣: 캡슐 축소
	GetCapsuleComponent()->SetCapsuleSize(30.f, 80.f);
}
