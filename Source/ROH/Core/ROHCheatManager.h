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

	// --- M3 성장/세이브 ---

	/** 경험치 지급. 예: ROHGiveXP 500 */
	UFUNCTION(Exec)
	void ROHGiveXP(int32 Amount);

	/** 스탯 분배. 예: ROHAllocStat Strength 5 (Strength/Dexterity/Vitality/Energy) */
	UFUNCTION(Exec)
	void ROHAllocStat(FName StatName, int32 Count = 1);

	/** 스킬 포인트 투자. 예: ROHSkillUp Bash */
	UFUNCTION(Exec)
	void ROHSkillUp(FName SkillId);

	/** 스킬트리 현황 출력 (계열/종류/랭크/요구 조건/시너지) */
	UFUNCTION(Exec)
	void ROHSkillInfo();

	/** 습득한 액티브 스킬을 슬롯 1~4에 배치. 예: ROHBindSkill 4 StaticField */
	UFUNCTION(Exec)
	void ROHBindSkill(int32 Slot, FName SkillId);

	/** 리스펙: 모든 스킬 포인트 환불 (그레이박스: 무제한. 정식은 난이도당 1회) */
	UFUNCTION(Exec)
	void ROHRespec();

	// --- M4 캠페인 ---

	/** 보스 발타르를 전방에 소환 (패턴 전투 테스트용) */
	UFUNCTION(Exec)
	void ROHSpawnBoss();

	/** 클래스 전환: warrior / elem. 예: ROHSetClass elem */
	UFUNCTION(Exec)
	void ROHSetClass(FString ClassName);

	UFUNCTION(Exec)
	void ROHSave();

	UFUNCTION(Exec)
	void ROHLoad();
};
