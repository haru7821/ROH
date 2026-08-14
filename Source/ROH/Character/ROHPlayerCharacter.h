#pragma once

#include "CoreMinimal.h"
#include "Character/ROHCharacterBase.h"
#include "ROHPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UROHInventoryComponent;

/**
 * 쿼터뷰 플레이어 캐릭터.
 * M1: 전사 프리셋 (분노 자원, 근접 스킬 4종). M3에서 클래스 선택/원소술사 분리.
 */
UCLASS()
class ROH_API AROHPlayerCharacter : public AROHCharacterBase
{
	GENERATED_BODY()

public:
	AROHPlayerCharacter();

	/** 슬롯 인덱스(DefaultAbilities 순서)로 어빌리티 발동. 0=기본공격, 1~3=스킬 */
	void ActivateAbilityBySlot(int32 SlotIndex);

	/** 리스폰: 위치 이동 + 상태/자원 복구 */
	void Revive(const FVector& Location);

	virtual void HandleDeath(AActor* Killer) override;

	UROHInventoryComponent* GetInventory() const { return Inventory; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "ROH|Inventory")
	TObjectPtr<UROHInventoryComponent> Inventory;

	UPROPERTY(VisibleAnywhere, Category = "ROH|Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "ROH|Camera")
	TObjectPtr<UCameraComponent> Camera;
};
