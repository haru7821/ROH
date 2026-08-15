#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Items/ROHItemTypes.h"
#include "ROHAccountSaveGame.generated.h"

/**
 * 계정 공유 세이브 (M5 최종 — docs/02 정복자/스태시): 슬롯 "ROH_Account".
 * 캐릭터 세이브(ROH_Default)와 분리 — 클래스 전환/캐릭터 로드와 무관하게 유지된다.
 */
UCLASS()
class ROH_API UROHAccountSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentVersion = 1;

	UPROPERTY()
	int32 SaveVersion = CurrentVersion;

	// --- 정복자 (만렙 이후 성장) ---
	UPROPERTY()
	int32 ParagonLevel = 0;

	UPROPERTY()
	int32 ParagonXP = 0;

	UPROPERTY()
	int32 ParagonPoints = 0;

	/** 분류(Offense/Defense/Precision/RuneAttune) → 투자 포인트 */
	UPROPERTY()
	TMap<FName, int32> ParagonAllocations;

	// --- 스태시 (계정 공유 창고) ---
	UPROPERTY()
	TArray<FROHItemInstance> StashItems;
};
