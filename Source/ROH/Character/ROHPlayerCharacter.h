#pragma once

#include "CoreMinimal.h"
#include "Character/ROHCharacterBase.h"
#include "GameplayEffectTypes.h" // FActiveGameplayEffectHandle
#include "ROHPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UROHInventoryComponent;
class UROHProgressionComponent;
class UROHSkillTreeComponent;
class AROHWaypoint;
class AROHZoneManager;

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

	/** 세이브용: 슬롯 1~4의 스킬 ID (역조회, 매칭 없으면 None) */
	TArray<FName> ExportBoundSkills() const;

	/** 로드용: 저장된 슬롯 배치 재적용 (스킬트리 복원 후 호출 — 랭크 검증 통과 필요) */
	void RestoreBoundSkills(const TArray<FName>& SkillIds);

	static constexpr int32 MaxSkillSlot = 4;

	/** E 상호작용: 웨이포인트 이동 → 주변 드랍 습득 → 인벤토리 첫 장비 장착. NPC 대화 등으로 확장 예정 */
	void Interact();

	/** 리스폰: 위치 이동 + 상태/자원 복구 */
	void Revive(const FVector& Location);

	virtual void HandleDeath(AActor* Killer) override;

	/** 현재 난이도의 저항 페널티(악몽 -40/지옥 -100)를 무한 GE로 적용. 난이도 변경 시 재호출 */
	void ApplyDifficultyResistPenalty();

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

	virtual void PossessedBy(AController* NewController) override;

private:
	/** 웨이포인트 순환 이동. 이동할 곳이 없으면 false (호출자는 습득/장착으로 계속) */
	bool TravelViaWaypoint(const AROHWaypoint* FromWaypoint);

	/** 지역 매니저 조회 (매 틱 월드 순회 방지 — 캐시 무효 시 재탐색) */
	AROHZoneManager* GetZoneManager();

	/** 난이도 저항 페널티 GE 핸들 (재적용 시 제거용) */
	FActiveGameplayEffectHandle DifficultyPenaltyHandle;

	TWeakObjectPtr<AROHZoneManager> CachedZoneManager;
};
