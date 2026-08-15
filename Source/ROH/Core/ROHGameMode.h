#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/TimerHandle.h" // FTimerHandle (지역 매니저 앵커 재시도, b34)
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

	/** 클래스 전환: 현재 폰을 파괴하고 지정 클래스로 재스폰 (치트/로드용) */
	AROHPlayerCharacter* RespawnPlayerAs(APlayerController* PlayerController, TSubclassOf<AROHPlayerCharacter> NewClass);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Respawn")
	float RespawnDelay = 3.f;

	/**
	 * 폴백 단독 스포너 자동 배치 여부 (그레이박스 테스트 편의).
	 * 지역 매니저 배치·마을 안전지대 강제는 이 플래그와 무관하게 항상 동작한다 (b34).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Debug")
	bool bAutoPlaceSpawnerIfMissing = true;

private:
	/**
	 * 지역 매니저 확보 (b34): 맵에 있으면 그대로, 없으면 자동 배치.
	 * 앵커는 **실제 플레이어 폰 위치**를 쓴다 — 맵에 PlayerStart가 여러 개면
	 * FindPlayerStart(nullptr)의 선택(ChoosePlayerStart는 미점유 스타트 중 무작위)이
	 * 엔진이 실제 플레이어에게 쓴 것과 어긋나 마을 경계가 플레이어와 안 맞기 때문.
	 * 폰이 아직 없으면 짧게 재시도한다 (스포너 초기 스폰 0.5초 전에 매니저가 서도록).
	 */
	void EnsureZoneManager();

	FTimerHandle ZoneManagerRetryHandle;
	int32 ZoneManagerRetryCount = 0;
};
