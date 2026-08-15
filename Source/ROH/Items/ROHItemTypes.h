#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "AttributeSet.h"
#include "ROHItemTypes.generated.h"

/** 아이템 등급 (docs/02 §3.1) */
UENUM(BlueprintType)
enum class EROHItemQuality : uint8
{
	Normal,   // 흰색 — 접사 없음, M5에서 룬워드 베이스
	Magic,    // 파란색 — 접두 1 + 접미 1
	Rare,     // 노란색 — 접사 3~6
	Unique,   // 금색 — M5
	Runeword  // 주황색 — M5
};

UENUM(BlueprintType)
enum class EROHEquipSlot : uint8
{
	None,
	Weapon,
	Shield,
	Helm,
	Chest,
	Boots
};

UENUM(BlueprintType)
enum class EROHItemKind : uint8
{
	Equipment,
	Potion,
	Rune // M5: 소켓/룬워드 재료 (인벤토리 보관형, 장착 불가)
};

/** 베이스 아이템 정의 (데이터 주도 — M2는 C++ 기본 데이터, 추후 DataTable 애셋으로 이관 가능) */
USTRUCT(BlueprintType)
struct FROHItemBaseDef : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName BaseId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EROHItemKind Kind = EROHItemKind::Equipment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EROHEquipSlot Slot = EROHEquipSlot::None;

	/** 무기: 물리 피해 범위 (평균값이 AttackPower로 반영) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	float DamageMin = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	float DamageMax = 0.f;

	/** 방어구: Defense 어트리뷰트에 가산 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	float Armor = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 RequiredLevel = 1;

	/** 물약: 즉시 회복량 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	float PotionHealAmount = 0.f;

	/** 상점 구매 가격 (판매가는 1/4) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 GoldValue = 10;

	/** 격자 크기 (인벤토리 UI용, M6) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FIntPoint GridSize = FIntPoint(1, 1);
};

/** 접사 정의: 어트리뷰트 가산형 (docs/02 §3.2) */
USTRUCT(BlueprintType)
struct FROHAffixDef : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	FName AffixId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	FText DisplayName;

	/** true = 접두사, false = 접미사 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	bool bPrefix = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	FGameplayAttribute Attribute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	float MinValue = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	float MaxValue = 5.f;

	/** 이 접사가 굴려지기 위한 최소 아이템 레벨 (고티어 접사의 고난이도 파밍 동기) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	int32 RequiredItemLevel = 1;

	/** 허용 슬롯. 비어 있으면 모든 장비 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	TArray<EROHEquipSlot> AllowedSlots;
};

/** 아이템 인스턴스에 굴려진 접사 하나 */
USTRUCT(BlueprintType)
struct FROHAffixRoll
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	FName AffixId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	FGameplayAttribute Attribute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	float Value = 0.f;
};

/** 아이템 인스턴스 = 베이스 참조 + 굴림 결과 (docs/03 §3.2) */
USTRUCT(BlueprintType)
struct FROHItemInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FGuid InstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName BaseId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EROHItemQuality Quality = EROHItemQuality::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 ItemLevel = 1;

	/** 재현 가능한 굴림용 시드 (디버깅/검증) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 Seed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TArray<FROHAffixRoll> Affixes;

	// --- 소켓/룬워드 (M5 — 추가 필드라 구버전 세이브는 기본값으로 로드) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 MaxSockets = 0;

	/** 삽입된 룬 ID (삽입 순서 유지 — 룬워드는 순서까지 일치해야 완성) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TArray<FName> SocketedRunes;

	/** 완성된 룬워드 ID (None = 미완성). 보너스는 장착 시 DB에서 해석 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName RunewordId;

	bool IsValid() const { return !BaseId.IsNone(); }
};

/**
 * 룬 정의 (M5, docs/02 §4 / docs/10 §5.2). 레지스트리는 코드 전용이라 리플렉션 불필요.
 * 소켓 시 BonusAttribute/BonusValue 가산 + RunePower(티어×0.5)가 최종 피해 배율에 합산된다.
 */
struct FROHRuneDef
{
	FName RuneId;
	FText DisplayName;
	int32 Tier = 1;
	FGameplayAttribute BonusAttribute;
	float BonusValue = 0.f;
	float RunePower = 0.f;
};

/** 룬워드 보너스 한 줄 (코드 레지스트리 전용) */
struct FROHRunewordBonus
{
	FGameplayAttribute Attribute;
	float Value = 0.f;
};

/**
 * 룬워드 정의 (M5): 일반(Normal) 등급 + 소켓 수 일치 + 룬 순서 일치 시 완성.
 * 완성 시 아이템 Quality가 Runeword로 승격되고 장착 시 Bonuses/RunePower가 추가 적용된다.
 */
struct FROHRunewordDef
{
	FName RunewordId;
	FText DisplayName;
	EROHEquipSlot RequiredSlot = EROHEquipSlot::Weapon;
	TArray<FName> RuneSequence;
	TArray<FROHRunewordBonus> Bonuses;
	float RunePower = 0.f;
};

UENUM(BlueprintType)
enum class EROHTreasureEntryType : uint8
{
	NoDrop,
	Gold,
	BaseItem,  // Ref = BaseId
	SubTable,  // Ref = 하위 TC Id
	Rune       // M5: 티어 추첨형 룬 드랍 (Ref 미사용 — ilvl 게이트 + 저티어 가중)
};

/** 트레저 클래스 항목 (가중치 추첨) */
USTRUCT(BlueprintType)
struct FROHTreasureEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	EROHTreasureEntryType Type = EROHTreasureEntryType::NoDrop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	FName Ref;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	int32 Weight = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	int32 GoldMin = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	int32 GoldMax = 25;
};

/** 계층형 트레저 클래스 (docs/02 §3.4) */
USTRUCT(BlueprintType)
struct FROHTreasureClassDef : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	FName TCId;

	/** 이 TC에서 몇 번 추첨할지 (보스는 다회 추첨) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	int32 Picks = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TArray<FROHTreasureEntry> Entries;
};

/** 드랍 굴림 결과 */
USTRUCT(BlueprintType)
struct FROHDropResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	TArray<FROHItemInstance> Items;

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	int32 Gold = 0;
};
