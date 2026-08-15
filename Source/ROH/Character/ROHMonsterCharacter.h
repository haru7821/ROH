#pragma once

#include "CoreMinimal.h"
#include "Character/ROHCharacterBase.h"
#include "Combat/ROHCombatStatics.h" // EROHDamageType / FROHDamageParams
#include "ROHMonsterCharacter.generated.h"

class UROHGameplayAbility;

/** 몬스터 등급 (docs/04 M4 챔피언/유니크 변형). 스폰 후 PromoteToRank로 승급 */
UENUM(BlueprintType)
enum class EROHMonsterRank : uint8
{
	Normal,
	Champion,
	Unique
};

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

	virtual void Tick(float DeltaSeconds) override;

	/**
	 * 정예 승급. 스포너가 SpawnActor 이후 호출할 것 —
	 * 빙의 시 난이도 스케일링이 끝난 값 위에 배율이 곱연산으로 합성된다.
	 * 이미 승급했거나 보스면 무시.
	 */
	void PromoteToRank(EROHMonsterRank NewRank);
	EROHMonsterRank GetRank() const { return Rank; }

	float GetAttackDamage() const { return AttackDamage; }
	EROHDamageType GetAttackDamageType() const { return AttackDamageType; }

	/**
	 * 기본 공격용 피해 파라미터: AttackDamage를 AttackDamageType에 맞는 유형 칸에 배분.
	 * 물리만 명중 굴림 대상, 원소 공격은 주문 취급 (docs/10 §5.3)
	 */
	FROHDamageParams MakeAttackDamageParams() const;

	float GetAttackRange() const { return AttackRange; }
	float GetAggroRange() const { return AggroRange; }
	float GetAttackInterval() const { return AttackInterval; }
	/** 원거리형이 유지하려는 거리. 0이면 근접형 */
	float GetPreferredRange() const { return PreferredRange; }
	TSubclassOf<UROHGameplayAbility> GetAttackAbility() const { return AttackAbility; }

protected:
	virtual void HandleDeath(AActor* Killer) override;

	/** 빙의 시 AttackAbility를 ASC에 부여 (TryActivateAbilityByClass는 부여된 어빌리티만 발동 가능) */
	virtual void PossessedBy(AController* NewController) override;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Monster")
	float AttackDamage = 10.f;

	/** 기본 공격 피해 유형 (docs/10 §5.3 — 6종) */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Monster")
	EROHDamageType AttackDamageType = EROHDamageType::Physical;

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

	/** 난이도 스케일링은 스폰(빙의) 시 1회만 */
	bool bDifficultyScaled = false;

	/** 정예 등급 (승급은 1회만) */
	EROHMonsterRank Rank = EROHMonsterRank::Normal;
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

/** 덩치: 느리고 단단한 근접 탱커 */
UCLASS()
class ROH_API AROHMonster_Brute : public AROHMonsterCharacter
{
	GENERATED_BODY()

public:
	AROHMonster_Brute();
};

/** 주술사: 사수보다 유리 몸이지만 공격이 아픈 원거리 술사 */
UCLASS()
class ROH_API AROHMonster_Hexer : public AROHMonsterCharacter
{
	GENERATED_BODY()

public:
	AROHMonster_Hexer();
};

/** 추적자: 매우 빠르게 파고드는 측면 기습형 */
UCLASS()
class ROH_API AROHMonster_Stalker : public AROHMonsterCharacter
{
	GENERATED_BODY()

public:
	AROHMonster_Stalker();
};
