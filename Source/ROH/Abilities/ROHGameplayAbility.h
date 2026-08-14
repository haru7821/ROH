#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ROHGameplayAbility.generated.h"

/**
 * 모든 스킬의 베이스. M1에서 자원 비용/쿨다운/시너지 계수 조회를 여기에 얹는다.
 */
UCLASS()
class ROH_API UROHGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UROHGameplayAbility();
};
