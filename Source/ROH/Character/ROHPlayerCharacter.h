#pragma once

#include "CoreMinimal.h"
#include "Character/ROHCharacterBase.h"
#include "ROHPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;

/**
 * 쿼터뷰 플레이어 캐릭터. 카메라는 고정 각도 스프링암.
 */
UCLASS()
class ROH_API AROHPlayerCharacter : public AROHCharacterBase
{
	GENERATED_BODY()

public:
	AROHPlayerCharacter();

protected:
	UPROPERTY(VisibleAnywhere, Category = "ROH|Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "ROH|Camera")
	TObjectPtr<UCameraComponent> Camera;
};
