#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/ROHItemTypes.h"
#include "ROHItemPickup.generated.h"

class USphereComponent;
class UTextRenderComponent;

/**
 * 지면 드랍 아이템/골드. 등급 색상 이름표 표시, 플레이어 접촉 시 습득.
 * (클릭 습득 + 이름표 UMG 배칭은 UI 마일스톤에서 교체)
 */
UCLASS()
class ROH_API AROHItemPickup : public AActor
{
	GENERATED_BODY()

public:
	AROHItemPickup();

	/** 아이템 드랍으로 초기화 */
	void InitAsItem(const FROHItemInstance& InItem);

	/** 골드 드랍으로 초기화 */
	void InitAsGold(int32 InGoldAmount);

	/** 플레이어에게 내용물 지급 시도. 성공 시 자신을 파괴하고 true */
	bool TryGive(class AROHPlayerCharacter* Player);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void RefreshLabel();

	UPROPERTY(VisibleAnywhere, Category = "ROH|Pickup")
	TObjectPtr<USphereComponent> PickupSphere;

	UPROPERTY(VisibleAnywhere, Category = "ROH|Pickup")
	TObjectPtr<UTextRenderComponent> NameLabel;

private:
	FROHItemInstance Item;
	int32 GoldAmount = 0;
};
