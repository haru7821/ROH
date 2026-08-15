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

	/** 스킬 슬롯 배치 (인덱스 0~3 = 슬롯 1~4, None = 기본 배치 유지). 구버전 세이브엔 없음 → 빈 배열 */
	UPROPERTY()
	TArray<FName> BoundSkillSlots;

	// --- 소지품 ---
	UPROPERTY()
	TArray<FROHItemInstance> InventoryItems;

	UPROPERTY()
	TMap<EROHEquipSlot, FROHItemInstance> EquippedItems;

	UPROPERTY()
	int32 Gold = 0;

	// --- 캠페인 (구버전 세이브엔 없음 → 기본값) ---
	/** EROHDifficulty 값 (0 노말 / 1 악몽 / 2 지옥) */
	UPROPERTY()
	uint8 Difficulty = 0;

	UPROPERTY()
	int32 QuestStage = 0;

	UPROPERTY()
	int32 QuestKills = 0;

	/** 퀘스트 단계 도달 전에 처치한 보스 기록 (자동 정산용) */
	UPROPERTY()
	bool bBaltarKilledEarly = false;

	UPROPERTY()
	bool bMorgathKilledEarly = false;

	/** 활성화된 웨이포인트 지역 인덱스 (구버전 세이브엔 없음 → 빈 배열 = 마을만) */
	UPROPERTY()
	TArray<int32> ActivatedWaypoints;
};
