#include "Core/ROHGameMode.h"
#include "Core/ROHPlayerController.h"
#include "Character/ROHPlayerCharacter.h"
#include "World/ROHMonsterSpawner.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "ROH.h"

AROHGameMode::AROHGameMode()
{
	DefaultPawnClass = AROHPlayerCharacter::StaticClass();
	PlayerControllerClass = AROHPlayerController::StaticClass();
}

void AROHGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!bAutoPlaceSpawnerIfMissing)
	{
		return;
	}

	// 맵에 스포너가 하나도 없으면 플레이어 스타트 근처에 자동 배치
	for (TActorIterator<AROHMonsterSpawner> It(GetWorld()); It; ++It)
	{
		return;
	}

	FVector SpawnLocation(1500.f, 0.f, 100.f);
	if (const AActor* Start = FindPlayerStart(nullptr))
	{
		SpawnLocation = Start->GetActorLocation() + FVector(1500.f, 0.f, 0.f);
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	GetWorld()->SpawnActor<AROHMonsterSpawner>(AROHMonsterSpawner::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);
	UE_LOG(LogROH, Log, TEXT("맵에 몬스터 스포너가 없어 자동 배치했습니다 (%s)"), *SpawnLocation.ToCompactString());
}

void AROHGameMode::SchedulePlayerRespawn(AROHPlayerCharacter* DeadPlayer)
{
	if (!DeadPlayer)
	{
		return;
	}

	TWeakObjectPtr<AROHPlayerCharacter> WeakPlayer = DeadPlayer;
	TWeakObjectPtr<AROHGameMode> WeakThis = this;

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, [WeakThis, WeakPlayer]()
	{
		if (!WeakThis.IsValid() || !WeakPlayer.IsValid())
		{
			return;
		}
		FVector RespawnLocation = WeakPlayer->GetActorLocation();
		if (AActor* PlayerStart = WeakThis->FindPlayerStart(WeakPlayer->GetController()))
		{
			RespawnLocation = PlayerStart->GetActorLocation();
		}
		WeakPlayer->Revive(RespawnLocation);
	}, RespawnDelay, false);
}
