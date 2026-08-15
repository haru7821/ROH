#pragma once

#include "CoreMinimal.h"
#include "Abilities/ROHGameplayAbility.h"
#include "ROHAbility_Charge.generated.h"

/**
 * 돌진 (b30, 무기술 상위기): 커서 방향 직선 돌진 — 경로 회랑과 도착점의 적에게 물리 피해.
 * 피해는 돌진 시작 시점의 경로 기준으로 판정하고 즉시 적용한다 (그레이박스 단순화 —
 * LaunchCharacter 이동은 연출, 판정은 결정적). 분노 소모, 공격 속도 스케일.
 */
UCLASS()
class ROH_API UROHAbility_Charge : public UROHGameplayAbility
{
	GENERATED_BODY()

public:
	UROHAbility_Charge();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float BaseDamage = 26.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float MaxChargeDistance = 800.f;

	/** 경로 판정 회랑 반폭 (돌진 선분에서 이 거리 이내의 적 타격) */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float PathHalfWidth = 140.f;

	/** 도착점 추가 판정 반경 (경로 회랑과 합집합) */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float ArrivalRadius = 200.f;

	/** 돌진 소요 시간(초) — LaunchCharacter 수평 속도 산출용 */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	float DashTime = 0.25f;
};
