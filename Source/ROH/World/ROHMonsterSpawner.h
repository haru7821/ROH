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
	int32 MaxAlive = 6;

	UPROPERTY(EditAnywhere, Category = "ROH|Spawner")
	float SpawnRadius = 800.f;

	UPROPERTY(EditAnywhere, Category = "ROH|Spawner")
	float RespawnDelay = 5.f;

private:
	int32 AliveCount = 0;
	int32 NextClassIndex = 0;
};
