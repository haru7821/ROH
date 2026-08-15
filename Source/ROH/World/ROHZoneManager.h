#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ROHZoneManager.generated.h"

class AROHMonsterCharacter;

/**
 * 지역 정의. 지역 배치는 코드가 단일 소스 (키 배치와 동일 원칙 — CLAUDE.md).
 * CenterOffset은 매니저 기준 상대 좌표 (+X 방향 6000uu 간격 배치).
 */
struct FROHZoneDef
{
	FText Name;
	FVector CenterOffset = FVector::ZeroVector;
	float Radius = 3000.f;
	bool bTown = false;

	// 비마을 지역용 스포너 구성
	TArray<TSubclassOf<AROHMonsterCharacter>> SpawnClasses;
	int32 MaxAlive = 0;
	int32 MonsterLevelBonus = 0;

	// 지역 보스 (null = 없음). 퀘스트가 BossMaxQuestStage 이하일 때만 스폰 (재클리어 방지)
	TSubclassOf<AROHMonsterCharacter> BossClass;
	int32 BossMaxQuestStage = -1;
};

/**
 * 지역 매니저 (docs/04 M4 "지역 3~4개 + 웨이포인트").
 * 맵에 없으면 게임모드가 플레이어 스타트 위치에 자동 배치한다.
 * BeginPlay에서 지역별 웨이포인트/스포너/보스를 구성하고,
 * 매 틱 지역 경계를 디버그 원으로 표시한다 (그레이박스).
 */
UCLASS()
class ROH_API AROHZoneManager : public AActor
{
	GENERATED_BODY()

public:
	AROHZoneManager();

	virtual void Tick(float DeltaSeconds) override;

	/** Location을 반경 안에 포함하는 가장 가까운 지역 인덱스. 없으면 -1 */
	int32 GetZoneIndexAt(const FVector& Location) const;
	FText GetZoneName(int32 ZoneIndex) const;
	bool IsTown(int32 ZoneIndex) const;
	FVector GetWaypointLocation(int32 ZoneIndex) const;
	int32 GetZoneCount() const { return Zones.Num(); }

protected:
	virtual void BeginPlay() override;

private:
	void BuildZoneTable();
	FVector GetZoneCenter(int32 ZoneIndex) const;

	TArray<FROHZoneDef> Zones;
};
