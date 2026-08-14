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
	void OnInteract();
	void ActivateSlot(int32 SlotIndex);

	/**
	 * 코드로 입력을 생성한다 (키 배치의 단일 소스):
	 * 좌클릭 이동 / 우클릭 기본공격 / 1·2·3 스킬 / E 상호작용.
	 * BP/애셋에 지정된 키가 있어도 코드 정의가 우선한다.
	 * (플레이어 키 커스터마이즈 기능 도입 시 이 정책 재검토)
	 */
	void BuildRuntimeInput();

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Input")
	TObjectPtr<UInputAction> SetDestinationAction;

	/** 기본 공격 (권장: 마우스 오른쪽 버튼) */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Input")
	TObjectPtr<UInputAction> BasicAttackAction;

	/** 스킬 1~3 (권장: 1/2/3) */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Input")
	TObjectPtr<UInputAction> Skill1Action;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Input")
	TObjectPtr<UInputAction> Skill2Action;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Input")
	TObjectPtr<UInputAction> Skill3Action;

	/** 상호작용: 주변 드랍 습득 → 없으면 장비 장착 (권장: E) */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Input")
	TObjectPtr<UInputAction> InteractAction;

	/** 이 시간(초) 이하로 누르면 클릭 이동, 넘으면 홀드 이동으로 판정 */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Input")
	float ShortPressThreshold = 0.3f;

private:
	FVector CachedDestination = FVector::ZeroVector;
	float FollowTime = 0.f;
	bool bRuntimeInputBuilt = false;
};
