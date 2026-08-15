#pragma once

#include "CoreMinimal.h"
#include "Character/ROHMonsterCharacter.h"
#include "ROHBossCharacter.generated.h"

/**
 * 보스 베이스 + 액트1 중간 보스 발타르 (docs/06 §3).
 * 패턴: 평타(근접) + 내려찍기(예고 후 광역·넉백, 주기 발동) + 50% 이하 광폭화.
 * 드랍은 TC_Boss (Picks 3), 처치 시 대량 경험치.
 */
UCLASS()
class ROH_API AROHBossCharacter : public AROHMonsterCharacter
{
	GENERATED_BODY()

public:
	AROHBossCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void HandleDamageTaken(float Damage, AActor* InstigatorActor) override;

protected:
	/** 빙의 시 SlamAbility 부여 (평타는 부모가 부여) */
	virtual void PossessedBy(AController* NewController) override;

	/** 화면 보스 HP 표시용 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Boss")
	FText BossName;

	/** 내려찍기 어빌리티 (주기적으로 발동) */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Boss")
	TSubclassOf<UROHGameplayAbility> SlamAbility;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Boss")
	float SlamInterval = 8.f;

	/** 대상이 이 거리 안일 때만 내려찍기 시도 */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Boss")
	float SlamTriggerRange = 500.f;

	/** 이 비율 이하 생명력에서 1회 광폭화 */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Boss")
	float EnrageHealthRatio = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Boss")
	float EnrageDamageMultiplier = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Boss")
	float EnrageMoveSpeedBonus = 120.f;

	/** 보스 HP를 화면에 표시할 플레이어와의 최대 거리 */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Boss")
	float HealthBarVisibleRange = 2500.f;

private:
	float NextSlamTime = 0.f;
	bool bEnraged = false;
};
