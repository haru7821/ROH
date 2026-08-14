#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Items/ROHItemTypes.h"
#include "ROHSaveGame.generated.h"

/**
 * 캐릭터 세이브 데이터 (docs/03 §3.5)
 * SaveVersion으로 마이그레이션 대비. M5에서 계정 공유 데이터(스태시/정복자) 분리 예정.
 */
UCLASS()
class ROH_API UROHSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentVersion = 1;

	UPROPERTY()
	int32 SaveVersion = CurrentVersion;

	/** "Warrior" / "Elementalist" */
	UPROPERTY()
	FString PlayerClassName = TEXT("Warrior");

	// --- 성장 ---
	UPROPERTY()
	int32 Level = 1;

	UPROPERTY()
	int32 XP = 0;

	UPROPERTY()
	int32 StatPoints = 0;

	UPROPERTY()
	int32 SkillPoints = 0;

	UPROPERTY()
	TMap<FName, int32> AllocatedStats;

	/** 스킬트리 하드 포인트 */
	UPROPERTY()
	TMap<FName, int32> SkillHardPoints;

	// --- 소지품 ---
	UPROPERTY()
	TArray<FROHItemInstance> InventoryItems;

	UPROPERTY()
	TMap<EROHEquipSlot, FROHItemInstance> EquippedItems;

	UPROPERTY()
	int32 Gold = 0;
};
