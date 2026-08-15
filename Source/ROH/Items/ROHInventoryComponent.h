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

	/** 인벤토리 아이템 제거 (합성 소모/추후 UI 버리기용) */
	bool RemoveItemAt(int32 ItemIndex);

	/** 인벤토리 인덱스의 장비를 장착 (해당 슬롯 기존 장비는 인벤토리로) */
	bool EquipItemByIndex(int32 ItemIndex);

	/** 인벤토리의 첫 번째 장비 아이템을 장착 (E 상호작용용) */
	bool EquipFirstEquippable();

	bool UnequipSlot(EROHEquipSlot Slot);

	/** 첫 번째 물약 사용 (즉시 회복). M4에서 벨트 슬롯으로 확장 */
	bool UseFirstPotion();

	/**
	 * 룬 소켓 (M5): 인벤토리의 룬을 인벤토리의 장비 빈 소켓에 삽입, 완성 시 룬워드 승격.
	 * 장착 중인 장비는 Items 배열에 없어 자연히 대상 제외 (해제 후 소켓).
	 */
	bool SocketRune(int32 ItemIndex, int32 RuneItemIndex, FString& OutError);

	/** 유니크 분해 (M5 2차): 유니크(고대 제외) 파괴 → 성유물 조각 2~4개. 결과/사유는 OutMessage */
	bool SalvageUnique(int32 ItemIndex, FString& OutMessage);

	/** 고대 합성 (M5 2차): 유니크 + 성유물 조각 5개 → Ancient 승격 (장착 해제 상태만) */
	bool ForgeAncient(int32 ItemIndex, FString& OutMessage);

	/** 고대 합성에 필요한 성유물 조각 수 */
	static constexpr int32 AncientForgeCost = 5;

	void AddGold(int32 Amount);
	bool SpendGold(int32 Amount);
	int32 GetGold() const { return Gold; }

	const TArray<FROHItemInstance>& GetItems() const { return Items; }
	const TMap<EROHEquipSlot, FROHItemInstance>& GetEquipped() const { return Equipped; }
	bool IsFull() const { return Items.Num() >= Capacity; }

	/** 세이브/로드: 현재 상태 내보내기/복원 (복원 시 장착 GE 재적용) */
	void ExportState(TArray<FROHItemInstance>& OutItems, TMap<EROHEquipSlot, FROHItemInstance>& OutEquipped, int32& OutGold) const;
	void RestoreState(const TArray<FROHItemInstance>& InItems, const TMap<EROHEquipSlot, FROHItemInstance>& InEquipped, int32 InGold);

protected:
	UAbilitySystemComponent* GetOwnerASC() const;
	UROHItemDatabase* GetDatabase() const;

	/** 장착 아이템의 베이스+접사를 무한 지속 GE로 적용 */
	void ApplyEquipEffect(EROHEquipSlot Slot, const FROHItemInstance& Item);
	void RemoveEquipEffect(EROHEquipSlot Slot);

	/**
	 * 세트 보너스 재계산 (M5 2차): 장착 조합에서 세트별 피스 수 집계 →
	 * 기존 세트 GE 전부 제거 후 도달 임계(누적)를 세트당 1개 무한 GE로 재적용.
	 * 장착 상태가 바뀌는 모든 경로(장착/해제/복원) 끝에서 호출할 것.
	 */
	void RefreshSetBonuses();

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Inventory")
	int32 Capacity = 40;

private:
	UPROPERTY()
	TArray<FROHItemInstance> Items;

	UPROPERTY()
	TMap<EROHEquipSlot, FROHItemInstance> Equipped;

	TMap<EROHEquipSlot, FActiveGameplayEffectHandle> EquipEffectHandles;

	/** 세트별 조합 보너스 GE 핸들 (키 = SetId) */
	TMap<FName, FActiveGameplayEffectHandle> SetBonusHandles;

	int32 Gold = 0;
};
