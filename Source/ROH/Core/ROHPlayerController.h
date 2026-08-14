#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ROHPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;

/**
 * 클릭 이동 컨트롤러 (디아블로식).
 * - 짧은 클릭: 클릭 지점으로 내비게이션 이동
 * - 꾹 누름: 커서 방향으로 실시간 이동
 * 입력 애셋(IMC/IA)은 BP 서브클래스(BP_ROHPlayerController)에서 지정한다. (docs/05 참고)
 */
UCLASS()
class ROH_API AROHPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AROHPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	void OnSetDestinationStarted();
	void OnSetDestinationTriggered();
	void OnSetDestinationReleased();

	void OnBasicAttack();
	void OnSkill1();
	void OnSkill2();
	void OnSkill3();
	void ActivateSlot(int32 SlotIndex);

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Input")
	TObjectPtr<UInputAction> SetDestinationAction;

	/** 기본 공격 (권장: 마우스 오른쪽 버튼) */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Input")
	TObjectPtr<UInputAction> BasicAttackAction;

	/** 스킬 1~3 (권장: Q/W/E) */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Input")
	TObjectPtr<UInputAction> Skill1Action;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Input")
	TObjectPtr<UInputAction> Skill2Action;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Input")
	TObjectPtr<UInputAction> Skill3Action;

	/** 이 시간(초) 이하로 누르면 클릭 이동, 넘으면 홀드 이동으로 판정 */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Input")
	float ShortPressThreshold = 0.3f;

private:
	FVector CachedDestination = FVector::ZeroVector;
	float FollowTime = 0.f;
};
