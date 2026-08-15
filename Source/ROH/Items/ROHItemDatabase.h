#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Items/ROHItemTypes.h"
#include "ROHItemDatabase.generated.h"

/** 장착 보정 한 줄 (코드 전용 — 장착 GE/툴팁 공용 해석 결과) */
struct FROHResolvedBonus
{
	FGameplayAttribute Attribute;
	float Value = 0.f;
};

/**
 * 아이템의 장착 보정 해석 결과 (UI 2차): 출처별로 구분해 툴팁 섹션 표시에 쓰고,
 * 장착 GE 빌드(UROHInventoryComponent::ApplyEquipEffect)는 전 배열을 그대로 모디파이어로 올린다.
 * 수치의 단일 소스 — 여기 밖에서 고대 ×1.5/룬 위력 합산을 재계산하지 말 것.
 */
struct FROHResolvedEquipBonuses
{
	/** 베이스 성능 (무기 피해 평균 → AttackPower, 방어구 → Defense) */
	TArray<FROHResolvedBonus> BaseBonuses;

	/** 접사 (라이브 해석 — Item.Affixes와 같은 순서/개수, 무효 어트리뷰트도 자리 유지) */
	TArray<FROHResolvedBonus> AffixBonuses;

	/** 소켓 룬 보너스 (DB 미등록 룬은 제외 — GE와 동일 규칙) */
	TArray<FROHResolvedBonus> RuneBonuses;

	/** 완성 룬워드 보너스 */
	TArray<FROHResolvedBonus> RunewordBonuses;

	/** 세트 피스 자체 옵션 (조합 보너스는 RefreshSetBonuses의 별도 GE) */
	TArray<FROHResolvedBonus> SetPieceBonuses;

	/** 유니크/고대 고정 옵션 (고대 ×1.5 반영치) */
	TArray<FROHResolvedBonus> UniqueBonuses;

	/** 총 룬 위력: 룬 + 룬워드 + 유니크 (+고대 +3) 합산 */
	float TotalRunePower = 0.f;
};

/**
 * 아이템/접사/트레저클래스 데이터 저장소 + 생성기 (docs/03 §3.2).
 * M2: 기본 데이터를 C++로 등록. 추후 DataTable 애셋이 있으면 그것으로 대체 가능한 구조.
 * 순수 로직(월드 불필요)이라 자동화 테스트에서 NewObject로 직접 사용 가능.
 */
UCLASS()
class ROH_API UROHItemDatabase : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** 테스트에서 서브시스템 초기화 없이 데이터를 구축할 수 있도록 공개 */
	void BuildDefaultData();

	const FROHItemBaseDef* FindBase(FName BaseId) const;
	const FROHAffixDef* FindAffix(FName AffixId) const;
	const FROHTreasureClassDef* FindTreasureClass(FName TCId) const;

	// --- 룬/룬워드 (M5 — docs/02 §4, docs/10 §5.2) ---
	const FROHRuneDef* FindRune(FName RuneId) const;
	const FROHRuneDef* FindRuneByTier(int32 Tier) const;
	/** 룬 아이템 베이스("Rune_<Id>")에서 룬 정의 역조회. 룬 베이스가 아니면 null */
	const FROHRuneDef* FindRuneByBaseId(FName BaseId) const;
	const TArray<FROHRuneDef>& GetRunes() const { return Runes; }
	static FName GetRuneBaseId(FName RuneId);

	const FROHRunewordDef* FindRuneword(FName RunewordId) const;
	const TArray<FROHRunewordDef>& GetRunewords() const { return Runewords; }
	/** 룬워드 완성 검사 (일반 등급 + 슬롯 + 소켓 수 + 룬 순서 일치). 미완성이면 null */
	const FROHRunewordDef* MatchRuneword(const FROHItemInstance& Item) const;

	// --- 유니크/고대 (M5 2차) ---
	const FROHUniqueDef* FindUnique(FName UniqueId) const;
	const TArray<FROHUniqueDef>& GetUniques() const { return Uniques; }

	// --- 세트 (M5 2차, 고대 유적지 테마) ---
	const FROHSetDef* FindSet(FName SetId) const;
	/** 피스 ID로 피스 정의 조회. OutSet에 소속 세트 반환 (null 허용) */
	const FROHSetPieceDef* FindSetPiece(FName PieceId, const FROHSetDef** OutSet = nullptr) const;
	const TArray<FROHSetDef>& GetSets() const { return Sets; }

	/** 고대 보정: 유니크 고정 옵션 ×1.5, 룬 위력 +3 (ROHForgeAncient) */
	static constexpr float AncientBonusMult = 1.5f;
	static constexpr float AncientRunePowerBonus = 3.f;

	/** 등급 판정: 매직파인드(MF)는 체감 곡선 적용 (docs/02 §3.4) */
	EROHItemQuality RollQuality(int32 ItemLevel, float MagicFind, FRandomStream& Rng) const;

	/**
	 * 아이템 인스턴스 생성: 등급에 따라 접사 굴림. Seed 0이면 무작위 시드.
	 * bAsUnidentifiedDrop: 몬스터 드랍 경로 전용 — 마법+ 장비를 미감정 상태로 (docs/12).
	 * 상점/치트/도박 생성은 기본값(false) = 감정 완료.
	 */
	FROHItemInstance GenerateItem(FName BaseId, int32 ItemLevel, EROHItemQuality Quality, int32 Seed = 0, bool bAsUnidentifiedDrop = false) const;

	/** 계층형 TC 굴림: NoDrop/골드/베이스/하위TC → 등급 판정 → 접사 굴림 */
	FROHDropResult RollTreasureClass(FName TCId, int32 ItemLevel, float MagicFind) const;

	/** 아이템 표시명 (등급 반영: "강철의 단검" 등) */
	FText GetItemDisplayName(const FROHItemInstance& Instance) const;

	/**
	 * 장착 보정 해석 (UI 2차 — 장착 GE/툴팁의 단일 소스): 베이스 성능 + 접사 + 소켓 룬 +
	 * 룬워드 + 세트 피스 자체 옵션 + 유니크/고대(옵션 ×1.5, 룬 위력 +3)를 출처별로 반환.
	 */
	FROHResolvedEquipBonuses ResolveEquipBonuses(const FROHItemInstance& Item) const;

	/**
	 * 아이템 툴팁 (UI 2차 — 호버 상세, 멀티라인): 이름/등급/종류/기본 성능/접사/소켓 룬/
	 * 룬워드/유니크·세트 옵션/골드 가치. 미감정은 "감정 필요"만 표시하고 옵션 숨김.
	 * 수치는 ResolveEquipBonuses 공용 해석 — 장착 GE와 항상 일치.
	 */
	FText GetItemTooltip(const FROHItemInstance& Item) const;

	/** 어트리뷰트 보정 표기 ("공격력 +5", "치명타 확률 +3%") — 툴팁/정복자 창 공용 */
	static FString FormatAttributeBonus(const FGameplayAttribute& Attribute, float Value);

	/** 등급 한글명 (툴팁: 일반/마법/레어/유니크/룬워드/고대/세트) */
	static const TCHAR* GetQualityLabel(EROHItemQuality Quality);

	/** 세이브 로드 후 접사 Attribute 참조를 DB 기준으로 재해석 (경로 직렬화 의존 제거 — 캐릭터/계정 세이브 공용) */
	void RefreshItemAffixes(FROHItemInstance& Item) const;

	static FColor GetQualityColor(EROHItemQuality Quality);

private:
	void RollAffixes(FROHItemInstance& Instance, const FROHItemBaseDef& Base, FRandomStream& Rng) const;
	TArray<const FROHAffixDef*> GetEligibleAffixes(const FROHItemBaseDef& Base, int32 ItemLevel, bool bPrefix) const;
	/** 장비 소켓 굴림 (무기/방패/투구/흉갑만, ilvl 게이트) */
	void RollSockets(FROHItemInstance& Instance, const FROHItemBaseDef& Base, FRandomStream& Rng) const;
	/** 룬 드랍 티어 추첨 (ilvl 게이트 + 티어당 가중치 반감) 후 인스턴스 생성 */
	FROHItemInstance GenerateRuneDrop(int32 ItemLevel, FRandomStream& Rng) const;

	TMap<FName, FROHItemBaseDef> Bases;
	TMap<FName, FROHAffixDef> Affixes;
	TMap<FName, FROHTreasureClassDef> TreasureClasses;

	/** 룬 12종 (인덱스 = 티어-1) / 룬워드 8종 / 유니크 15종 / 세트 4종 — 코드 레지스트리 (M5) */
	TArray<FROHRuneDef> Runes;
	TArray<FROHRunewordDef> Runewords;
	TArray<FROHUniqueDef> Uniques;
	TArray<FROHSetDef> Sets;
};
