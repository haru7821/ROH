#include "Core/ROHGameMode.h"
#include "Core/ROHPlayerController.h"
#include "Character/ROHPlayerCharacter.h"
#include "Character/ROHPlayerClasses.h"
#include "World/ROHMonsterSpawner.h"
#include "World/ROHZoneManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "ROH.h"

AROHGameMode::AROHGameMode()
{
	DefaultPawnClass = AROHWarriorCharacter::StaticClass();
	PlayerControllerClass = AROHPlayerController::StaticClass();
}

AROHPlayerCharacter* AROHGameMode::RespawnPlayerAs(APlayerController* PlayerController, TSubclassOf<AROHPlayerCharacter> NewClass)
{
	if (!PlayerController || !NewClass)
	{
		return nullptr;
	}

	FTransform SpawnTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 100.f));
	if (APawn* OldPawn = PlayerController->GetPawn())
	{
		SpawnTransform = OldPawn->GetActorTransform();
		PlayerController->UnPossess();
		OldPawn->Destroy();
	}
	else if (const AActor* Start = FindPlayerStart(PlayerController))
	{
		SpawnTransform = Start->GetActorTransform();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AROHPlayerCharacter* NewPawn = GetWorld()->SpawnActor<AROHPlayerCharacter>(NewClass, SpawnTransform, SpawnParams);
	if (!NewPawn)
	{
		// 원래 위치 스폰 실패 시 플레이어 스타트에서 재시도
		if (const AActor* Start = FindPlayerStart(PlayerController))
		{
			NewPawn = GetWorld()->SpawnActor<AROHPlayerCharacter>(NewClass, Start->GetActorTransform(), SpawnParams);
		}
	}
	if (NewPawn)
	{
		PlayerController->Possess(NewPawn);
	}
	else
	{
		UE_LOG(LogROH, Error, TEXT("RespawnPlayerAs: 폰 스폰 실패 (%s)"), *GetNameSafe(NewClass));
	}
	return NewPawn;
}

void AROHGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!bAutoPlaceSpawnerIfMissing)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// 1) 지역 매니저 우선: 존재하면(또는 자동 배치되면) 스포너/보스/웨이포인트 배치는 매니저가 담당
	AROHZoneManager* ZoneManager = nullptr;
	for (TActorIterator<AROHZoneManager> It(GetWorld()); It; ++It)
	{
		ZoneManager = *It;
		break;
	}
	if (!ZoneManager)
	{
		// 마을(0번 지역)이 플레이어 스타트를 감싸도록 스타트 위치에 앵커
		FVector AnchorLocation(0.f, 0.f, 100.f);
		if (const AActor* Start = FindPlayerStart(nullptr))
		{
			AnchorLocation = Start->GetActorLocation();
		}
		ZoneManager = GetWorld()->SpawnActor<AROHZoneManager>(
			AROHZoneManager::StaticClass(), AnchorLocation, FRotator::ZeroRotator, SpawnParams);
		if (ZoneManager)
		{
			UE_LOG(LogROH, Log, TEXT("지역 매니저 자동 배치 (%s)"), *AnchorLocation.ToCompactString());
		}
	}
	if (ZoneManager)
	{
		return;
	}

	// 2) 폴백: 지역 매니저 스폰 실패 시에만 기존 단독 스포너 자동 배치 유지
	for (TActorIterator<AROHMonsterSpawner> It(GetWorld()); It; ++It)
	{
		return;
	}

	FVector SpawnLocation(1500.f, 0.f, 100.f);
	if (const AActor* Start = FindPlayerStart(nullptr))
	{
		SpawnLocation = Start->GetActorLocation() + FVector(1500.f, 0.f, 0.f);
	}
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
