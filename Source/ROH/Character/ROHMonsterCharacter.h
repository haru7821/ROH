#pragma once

#include "CoreMinimal.h"
#include "Character/ROHCharacterBase.h"
#include "ROHMonsterCharacter.generated.h"

class UROHGameplayAbility;

/**
 * 몬스터 베이스. 스탯은 EditDefaults 프로퍼티로 정의하고 스폰 시 어트리뷰트에 반영한다.
 * M2에서 DT_Monsters 데이터테이블 주도로 이관 예정 (docs/03).
 */
UCLASS()
class ROH_API AROHMonsterCharacter : public AROHCharacterBase
{
	GENERATED_BODY()

public:
	AROHMonsterCharacter();

	float GetAttackDamage() const { return AttackDamage; }
	float GetAttackRange() const { return AttackRange; }
	float GetAggroRange() const { return AggroRange; }
	float GetAttackInterval() const { return AttackInterval; }
	/** 원거리형이 유지하려는 거리. 0이면 근접형 */
	float GetPreferredRange() const { return PreferredRange; }
	TSubclassOf<UROHGameplayAbility> GetAttackAbility() const { return AttackAbility; }

protected:
	virtual void HandleDeath(AActor* Killer) override;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Monster")
	float AttackDamage = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Monster")
	float AttackRange = 180.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Monster")
	float AggroRange = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Monster")
	float AttackInterval = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Monster")
	float PreferredRange = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Monster")
	TSubclassOf<UROHGameplayAbility> AttackAbility;

	/** 사망 후 시체 유지 시간(초) */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Monster")
	float CorpseLifetime = 4.f;

	/** 사망 시 굴릴 트레저 클래스 (docs/02 §3.4) */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Monster")
	FName TreasureClassId = TEXT("TC_Default");

	/** 처치 시 지급 경험치 */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Monster")
	int32 XPValue = 15;

private:
	void DropLoot(AActor* Killer);
};

/** 졸개: 평균적인 근접 몬스터 */
UCLASS()
class ROH_API AROHMonster_Grunt : public AROHMonsterCharacter
{
	GENERATED_BODY()

public:
	AROHMonster_Grunt();
};

/** 사수: 거리를 유지하며 투사체를 쏘는 원거리 몬스터 */
UCLASS()
class ROH_API AROHMonster_Archer : public AROHMonsterCharacter
{
	GENERATED_BODY()

public:
	AROHMonster_Archer();
};

/** 돌격병: 빠르고 아프지만 물렁한 몬스터 */
UCLASS()
class ROH_API AROHMonster_Charger : public AROHMonsterCharacter
{
	GENERATED_BODY()

public:
	AROHMonster_Charger();
};
