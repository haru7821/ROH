#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ROHMonsterSpawner.generated.h"

class AROHMonsterCharacter;
class AROHCharacterBase;

/**
 * 몬스터 스포너: 반경 내 무작위 지점에 몬스터를 유지 스폰한다.
 * 처치되면 RespawnDelay 후 재스폰 → 전투 반복 테스트용.
 * (오브젝트 풀링은 M2 물량 확충 시 도입)
 */
UCLASS()
class ROH_API AROHMonsterSpawner : public AActor
{
	GENERATED_BODY()

public:
	AROHMonsterSpawner();

	/**
	 * 지역 매니저용 설정 주입 (docs/04 M4 지역).
	 * 초기 스폰은 BeginPlay의 0.5초 지연 타이머로 시작되므로 SpawnActor 직후 호출이 안전하다
	 * — 이 가정이 깨지면(즉시 스폰으로 바뀌면) SpawnActorDeferred로 전환할 것.
	 */
	void ConfigureSpawner(const TArray<TSubclassOf<AROHMonsterCharacter>>& InClasses, int32 InMaxAlive, int32 InMonsterLevelBonus);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnMonsterDeath(AROHCharacterBase* DeadCharacter);

	void SpawnInitialBatch();
	void SpawnOne();

	/** 스폰할 몬스터 종류 (순환 스폰) */
	UPROPERTY(EditAnywhere, Category = "ROH|Spawner")
	TArray<TSubclassOf<AROHMonsterCharacter>> MonsterClasses;

	UPROPERTY(EditAnywhere, Category = "ROH|Spawner")
	int32 MaxAlive = 8;

	/** 스폰 몬스터 레벨 가산 (깊은 지역: 명중 공식 레벨 + 드랍 ilvl 상승) */
	UPROPERTY(EditAnywhere, Category = "ROH|Spawner")
	int32 MonsterLevelBonus = 0;

	UPROPERTY(EditAnywhere, Category = "ROH|Spawner")
	float SpawnRadius = 800.f;

	UPROPERTY(EditAnywhere, Category = "ROH|Spawner")
	float RespawnDelay = 5.f;

private:
	int32 AliveCount = 0;
	int32 NextClassIndex = 0;
};
