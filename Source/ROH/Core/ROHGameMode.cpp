#include "Core/ROHGameMode.h"
#include "Core/ROHPlayerController.h"
#include "Character/ROHPlayerCharacter.h"
#include "Engine/World.h"
#include "TimerManager.h"

AROHGameMode::AROHGameMode()
{
	DefaultPawnClass = AROHPlayerCharacter::StaticClass();
	PlayerControllerClass = AROHPlayerController::StaticClass();
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
