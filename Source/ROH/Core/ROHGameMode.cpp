#include "Core/ROHGameMode.h"
#include "Core/ROHPlayerController.h"
#include "Character/ROHPlayerCharacter.h"
#include "Character/ROHPlayerClasses.h"
#include "World/ROHMonsterSpawner.h"
#include "World/ROHZoneManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "GameFramework/Pawn.h"             // 앵커용 플레이어 폰 위치 (b34)
#include "GameFramework/PlayerController.h" // GetFirstPlayerController 반환 타입 (b34)
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

	// 지역 매니저 배치는 플래그와 무관하게 항상 수행한다 (Sup b34): 매니저가 없으면 마을 안전지대
	// 개념 자체가 사라져 맵에 배치된 잔재 스포너가 마을에서 무제한 가동된다. 플래그는 이름 그대로
	// "폴백 단독 스포너 자동 배치" 여부만 통제한다.
	EnsureZoneManager();
}

void AROHGameMode::EnsureZoneManager()
{
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
		// 마을(0번 지역)이 실제 플레이어 위치를 감싸도록 앵커 (b34):
		// PlayerStart가 여러 개인 맵에서 FindPlayerStart(nullptr)는 엔진이 플레이어에게 실제로
		// 사용한 스타트와 다른 것을 고를 수 있다(ChoosePlayerStart = 미점유 스타트 중 무작위).
		// → 폰이 있으면 폰 위치가 정답. 아직 없으면 짧게 재시도한다.
		FVector AnchorLocation(0.f, 0.f, 100.f);
		bool bAnchorResolved = false;
		if (const APlayerController* FirstPC = GetWorld()->GetFirstPlayerController())
		{
			if (const APawn* PlayerPawn = FirstPC->GetPawn())
			{
				AnchorLocation = PlayerPawn->GetActorLocation();
				bAnchorResolved = true;
			}
		}

		if (!bAnchorResolved)
		{
			// 재시도 상한 (0.2초 × 10 = 2초). 스포너 초기 스폰(0.5초)보다 먼저 서는 것이 목표이고,
			// 늦어져도 ZoneManager의 마을 침입 정리 타이머가 백스톱이 된다.
			if (ZoneManagerRetryCount < 10)
			{
				++ZoneManagerRetryCount;
				GetWorld()->GetTimerManager().SetTimer(ZoneManagerRetryHandle, this,
					&AROHGameMode::EnsureZoneManager, 0.2f, false);
				return;
			}
			// 상한 도달: 폰을 못 찾았으니 기존 규칙(플레이어 스타트)으로 확정
			if (const AActor* Start = FindPlayerStart(nullptr))
			{
				AnchorLocation = Start->GetActorLocation();
			}
			UE_LOG(LogROH, Warning, TEXT("지역 매니저 앵커: 플레이어 폰을 찾지 못해 플레이어 스타트로 대체 (%s)"),
				*AnchorLocation.ToCompactString());
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
	if (!bAutoPlaceSpawnerIfMissing)
	{
		return;
	}
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
