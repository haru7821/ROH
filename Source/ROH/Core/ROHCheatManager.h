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

	/**
	 * 테스트 세팅: 레벨 50 + 현재 클래스 전 스킬 투자.
	 * 예: ROHMaxOut (전 스킬 1랭크) / ROHMaxOut 20 (전 스킬 만렙)
	 */
	UFUNCTION(Exec)
	void ROHMaxOut(int32 RankPerSkill = 1);

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

	/** 보스 소환: ROHSpawnBoss(발타르) / ROHSpawnBoss 2(모르가스) */
	UFUNCTION(Exec)
	void ROHSpawnBoss(FString Which = TEXT("1"));

	/** 난이도 변경: normal / nightmare / hell (몬스터 강화는 새 스폰부터) */
	UFUNCTION(Exec)
	void ROHSetDifficulty(FString Name);

	/** 클래스 전환: warrior / elem. 예: ROHSetClass elem */
	UFUNCTION(Exec)
	void ROHSetClass(FString ClassName);

	UFUNCTION(Exec)
	void ROHSave();

	UFUNCTION(Exec)
	void ROHLoad();

	/** 활성화된 웨이포인트로 순간이동. 인자 없이 호출하면 지역 목록/활성 상태 출력. 예: ROHWarp 2 */
	UFUNCTION(Exec)
	void ROHWarp(int32 ZoneIndex = -1);

	// --- M5 룬/룬워드 ---

	/** 룬 지급: 티어 번호 또는 룬 ID. 예: ROHGiveRune 3 / ROHGiveRune Azak 2 */
	UFUNCTION(Exec)
	void ROHGiveRune(FString TierOrName, int32 Count = 1);

	/** 3:1 합성: 같은 티어 룬 3개 → 상위 티어 1개 (낮은 티어 우선) */
	UFUNCTION(Exec)
	void ROHTransmute();

	/** 룬 12종(보유 수 포함) + 룬워드 조합법 출력 */
	UFUNCTION(Exec)
	void ROHRunes();

	/** 인벤토리 장비에 룬 소켓. 예: ROHSocket 0 3 (0=장비 인덱스, 3=룬 인덱스) */
	UFUNCTION(Exec)
	void ROHSocket(int32 ItemIndex, int32 RuneItemIndex);

	// --- M5 2차 유니크/고대 ---

	/** 유니크 분해 → 성유물 조각 2~4개 (고대는 분해 불가) */
	UFUNCTION(Exec)
	void ROHSalvage(int32 ItemIndex);

	/** 고대 합성: 인벤토리의 유니크 + 성유물 조각 5개 → 고대 승격 */
	UFUNCTION(Exec)
	void ROHForgeAncient(int32 ItemIndex);

	/** 유니크 15종 목록 출력 */
	UFUNCTION(Exec)
	void ROHUniques();

	/** 세트 4종 목록 출력 (피스 구성/보너스 임계) */
	UFUNCTION(Exec)
	void ROHSets();

	// --- M5 3차 도박 ---

	/** 도박: 운명의 보석 3개 → 무기 뽑기 (50% 레어/30% 유니크/15% 세트/5% 고대 잭팟) */
	UFUNCTION(Exec)
	void ROHGamble();

	/** 운명의 보석 지급 (테스트용). 예: ROHGiveGem 9 */
	UFUNCTION(Exec)
	void ROHGiveGem(int32 Count = 3);

	/** 전부 무료 감정 (테스트용 — 정식 창구는 셀바, 개당 50골드) */
	UFUNCTION(Exec)
	void ROHIdentify();
};
