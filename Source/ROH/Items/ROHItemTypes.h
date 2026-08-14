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
	Potion
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

	bool IsValid() const { return !BaseId.IsNone(); }
};

UENUM(BlueprintType)
enum class EROHTreasureEntryType : uint8
{
	NoDrop,
	Gold,
	BaseItem,  // Ref = BaseId
	SubTable   // Ref = 하위 TC Id
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
