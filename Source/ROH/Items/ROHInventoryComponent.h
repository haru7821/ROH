#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/ROHItemTypes.h"
#include "GameplayEffectTypes.h"
#include "ROHInventoryComponent.generated.h"

class UAbilitySystemComponent;
class UROHItemDatabase;

/**
 * 플레이어 인벤토리 + 장비창 + 골드 (docs/02 §5).
 * 장착 시 베이스 성능(무기 공격력/방어구 방어)과 접사를 런타임 GameplayEffect로 적용,
 * 해제 시 제거 — 모든 스탯 보정을 GE로 일원화하는 원칙 유지 (docs/03 §3.1).
 * M2는 슬롯 목록형(40칸), 격자 배치 UI는 M6.
 */
UCLASS(ClassGroup = (ROH), meta = (BlueprintSpawnableComponent))
class ROH_API UROHInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UROHInventoryComponent();

	/** 습득. 가득 차면 false */
	bool AddItem(const FROHItemInstance& Item);

	/** 인벤토리 인덱스의 장비를 장착 (해당 슬롯 기존 장비는 인벤토리로) */
	bool EquipItemByIndex(int32 ItemIndex);

	/** 인벤토리의 첫 번째 장비 아이템을 장착 (E 상호작용용) */
	bool EquipFirstEquippable();

	bool UnequipSlot(EROHEquipSlot Slot);

	/** 첫 번째 물약 사용 (즉시 회복). M4에서 벨트 슬롯으로 확장 */
	bool UseFirstPotion();

	void AddGold(int32 Amount);
	bool SpendGold(int32 Amount);
	int32 GetGold() const { return Gold; }

	const TArray<FROHItemInstance>& GetItems() const { return Items; }
	const TMap<EROHEquipSlot, FROHItemInstance>& GetEquipped() const { return Equipped; }
	bool IsFull() const { return Items.Num() >= Capacity; }

protected:
	UAbilitySystemComponent* GetOwnerASC() const;
	UROHItemDatabase* GetDatabase() const;

	/** 장착 아이템의 베이스+접사를 무한 지속 GE로 적용 */
	void ApplyEquipEffect(EROHEquipSlot Slot, const FROHItemInstance& Item);
	void RemoveEquipEffect(EROHEquipSlot Slot);

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Inventory")
	int32 Capacity = 40;

private:
	UPROPERTY()
	TArray<FROHItemInstance> Items;

	UPROPERTY()
	TMap<EROHEquipSlot, FROHItemInstance> Equipped;

	TMap<EROHEquipSlot, FActiveGameplayEffectHandle> EquipEffectHandles;

	int32 Gold = 0;
};
