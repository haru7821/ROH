#include "World/ROHMonsterSpawner.h"
#include "Character/ROHMonsterCharacter.h"
#include "Character/ROHAttributeSet.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "NavigationSystem.h"

AROHMonsterSpawner::AROHMonsterSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	// 기본 구성: 6종 순환 스폰 (docs/04 M4 몬스터 확충)
	MonsterClasses.Add(AROHMonster_Grunt::StaticClass());
	MonsterClasses.Add(AROHMonster_Archer::StaticClass());
	MonsterClasses.Add(AROHMonster_Charger::StaticClass());
	MonsterClasses.Add(AROHMonster_Brute::StaticClass());
	MonsterClasses.Add(AROHMonster_Hexer::StaticClass());
	MonsterClasses.Add(AROHMonster_Stalker::StaticClass());
}

void AROHMonsterSpawner::ConfigureSpawner(const TArray<TSubclassOf<AROHMonsterCharacter>>& InClasses, int32 InMaxAlive, int32 InMonsterLevelBonus)
{
	if (InClasses.Num() > 0)
	{
		MonsterClasses = InClasses;
	}
	MaxAlive = FMath::Max(1, InMaxAlive);
	MonsterLevelBonus = FMath::Max(0, InMonsterLevelBonus);
	bZoneOwned = true; // 지역 매니저가 만든 스포너 표시 (안전지대 정리에서 제외)
}

void AROHMonsterSpawner::BeginPlay()
{
	Super::BeginPlay();

	// 내비게이션 데이터 로드가 끝난 뒤 초기 스폰 (첫 프레임엔 랜덤 지점 탐색이 실패할 수 있음)
	FTimerHandle InitialSpawnHandle;
	GetWorld()->GetTimerManager().SetTimer(InitialSpawnHandle, this, &AROHMonsterSpawner::SpawnInitialBatch, 0.5f, false);
}

void AROHMonsterSpawner::SpawnInitialBatch()
{
	for (int32 i = 0; i < MaxAlive; ++i)
	{
		SpawnOne();
	}
}

void AROHMonsterSpawner::SpawnOne()
{
	if (MonsterClasses.Num() == 0 || !GetWorld() || AliveCount >= MaxAlive)
	{
		return;
	}

	TSubclassOf<AROHMonsterCharacter> MonsterClass = MonsterClasses[NextClassIndex % MonsterClasses.Num()];
	++NextClassIndex;
	if (!MonsterClass)
	{
		return;
	}

	// 반경 내 내비게이션 가능한 무작위 지점
	FVector SpawnLocation = GetActorLocation();
	if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		FNavLocation NavLocation;
		if (NavSys->GetRandomReachablePointInRadius(GetActorLocation(), SpawnRadius, NavLocation))
		{
			SpawnLocation = NavLocation.Location;
		}
	}
	SpawnLocation.Z += 100.f;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AROHMonsterCharacter* Monster = GetWorld()->SpawnActor<AROHMonsterCharacter>(MonsterClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
	if (Monster)
	{
		++AliveCount;
		Monster->OnDeath.AddDynamic(this, &AROHMonsterSpawner::OnMonsterDeath);

		// 지역 레벨 가산: 빙의(동기) 난이도 스케일링 이후 합성 — PromoteToRank와 동일 규칙
		if (MonsterLevelBonus > 0)
		{
			if (UROHAttributeSet* Attributes = Monster->GetAttributeSet())
			{
				Attributes->SetCharacterLevel(Attributes->GetCharacterLevel() + MonsterLevelBonus);
			}
		}

		// 정예 판정: 고유 3%, 정예 10% (docs/04 M4 챔피언/유니크 변형)
		// SpawnActor 직후 = 빙의 난이도 스케일링 이후이므로 배율이 난이도와 합성된다
		const float EliteRoll = FMath::FRand();
		if (EliteRoll < 0.03f)
		{
			Monster->PromoteToRank(EROHMonsterRank::Unique);
		}
		else if (EliteRoll < 0.13f)
		{
			Monster->PromoteToRank(EROHMonsterRank::Champion);
		}
	}
}

void AROHMonsterSpawner::OnMonsterDeath(AROHCharacterBase* DeadCharacter)
{
	AliveCount = FMath::Max(0, AliveCount - 1);

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AROHMonsterSpawner::SpawnOne, RespawnDelay, false);
}
