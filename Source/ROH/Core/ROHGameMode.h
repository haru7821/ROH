#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ROHGameMode.generated.h"

class AROHPlayerCharacter;

UCLASS()
class ROH_API AROHGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AROHGameMode();

	/** 플레이어 사망 시 호출: RespawnDelay 후 플레이어 스타트에서 부활 */
	void SchedulePlayerRespawn(AROHPlayerCharacter* DeadPlayer);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Respawn")
	float RespawnDelay = 3.f;
};
