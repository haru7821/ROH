#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "ROHCheatManager.generated.h"

/**
 * 밸런스 반복 테스트용 치트 콘솔 (docs/03 §5).
 * PIE 콘솔(`)에서 사용. 예:
 *   ROHSetAttr Strength 50
 *   ROHDumpAttrs
 */
UCLASS()
class ROH_API UROHCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	/** 플레이어 어트리뷰트 베이스 값을 설정한다. 예: ROHSetAttr Health 999 */
	UFUNCTION(Exec)
	void ROHSetAttr(FName AttributeName, float Value);

	/** 플레이어의 모든 어트리뷰트를 로그/화면에 출력한다. */
	UFUNCTION(Exec)
	void ROHDumpAttrs();

	// --- M2 아이템/드랍 ---

	/** 아이템 생성 후 인벤토리에 추가. Quality: normal/magic/rare. 예: ROHGiveItem BattleAxe 6 rare */
	UFUNCTION(Exec)
	void ROHGiveItem(FName BaseId, int32 ItemLevel = 1, FString Quality = TEXT("magic"));

	/** 인벤토리/장비창/골드 출력 */
	UFUNCTION(Exec)
	void ROHDumpInventory();

	/** 인벤토리 인덱스의 장비 장착 */
	UFUNCTION(Exec)
	void ROHEquip(int32 ItemIndex);

	/** 물약 사용 */
	UFUNCTION(Exec)
	void ROHUsePotion();

	UFUNCTION(Exec)
	void ROHAddGold(int32 Amount);

	/** 상점 최소 구현: 골드 50으로 치유물약 구매 (NPC UI는 M4) */
	UFUNCTION(Exec)
	void ROHBuyPotion();

	/** 드랍 시뮬레이터: TC를 N회 굴려 등급/골드 통계 출력. 예: ROHSimulateDrops TC_Default 10000 */
	UFUNCTION(Exec)
	void ROHSimulateDrops(FName TCId, int32 Count = 10000);
};
