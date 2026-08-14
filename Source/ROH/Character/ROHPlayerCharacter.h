#pragma once

#include "CoreMinimal.h"
#include "Character/ROHCharacterBase.h"
#include "ROHPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UROHInventoryComponent;
class UROHProgressionComponent;
class UROHSkillTreeComponent;

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

	/** 슬롯 인덱스(DefaultAbilities 순서)로 어빌리티 발동. 0=기본공격, 1~4=스킬 */
	void ActivateAbilityBySlot(int32 SlotIndex);

	/** 습득한 액티브 스킬을 슬롯(1~4)에 배치. 실패 사유는 OutError로 (콘솔 ROHBindSkill) */
	bool BindSkillToSlot(int32 SlotIndex, FName SkillId, FString& OutError);

	static constexpr int32 MaxSkillSlot = 4;

	/** E 상호작용: 주변 드랍 습득 → 없으면 인벤토리 첫 장비 장착. NPC 대화 등으로 확장 예정 */
	void Interact();

	/** 리스폰: 위치 이동 + 상태/자원 복구 */
	void Revive(const FVector& Location);

	virtual void HandleDeath(AActor* Killer) override;

	UROHInventoryComponent* GetInventory() const { return Inventory; }
	UROHProgressionComponent* GetProgression() const { return Progression; }
	UROHSkillTreeComponent* GetSkillTree() const { return SkillTree; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "ROH|Inventory")
	TObjectPtr<UROHInventoryComponent> Inventory;

	UPROPERTY(VisibleAnywhere, Category = "ROH|Progression")
	TObjectPtr<UROHProgressionComponent> Progression;

	UPROPERTY(VisibleAnywhere, Category = "ROH|Progression")
	TObjectPtr<UROHSkillTreeComponent> SkillTree;

	UPROPERTY(VisibleAnywhere, Category = "ROH|Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "ROH|Camera")
	TObjectPtr<UCameraComponent> Camera;
};
