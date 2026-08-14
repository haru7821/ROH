#include "World/ROHMonsterSpawner.h"
#include "Character/ROHMonsterCharacter.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "NavigationSystem.h"

AROHMonsterSpawner::AROHMonsterSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	// 기본 구성: 3종 순환 스폰
	MonsterClasses.Add(AROHMonster_Grunt::StaticClass());
	MonsterClasses.Add(AROHMonster_Archer::StaticClass());
	MonsterClasses.Add(AROHMonster_Charger::StaticClass());
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
	}
}

void AROHMonsterSpawner::OnMonsterDeath(AROHCharacterBase* DeadCharacter)
{
	AliveCount = FMath::Max(0, AliveCount - 1);

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AROHMonsterSpawner::SpawnOne, RespawnDelay, false);
}
