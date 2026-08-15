#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TimerHandle.h" // FTimerHandle (마을 침입 정리 타이머, b34)
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

	/** Location이 bTown 존 반경 안인지 (안전지대 판정 — 스포너 제거/침입 정리 공용) */
	bool IsInsideTownZone(const FVector& Location) const;

	/**
	 * 마을 안전지대 강제 1 (b34): 맵에 배치된 잔재 스포너가 마을에서 무한 스폰하는 문제 대응.
	 * 존 배치 직후 월드의 모든 스포너를 검사해 마을 반경 안이면 제거한다
	 * (자신이 방금 스폰한 전투 지역 스포너는 마을 밖이라 무영향).
	 */
	void RemoveSpawnersInTownZones();

	/**
	 * 마을 안전지대 강제 2 (b34, 백스톱): 이미 스폰됐거나 플레이어를 쫓아 들어온 적대 몬스터를
	 * 저빈도(1초) 타이머로 제거한다. 플레이어/NPC/웨이포인트/상자는 대상 아님(TeamId로 구분),
	 * 치트 소환 몬스터는 면제(테스트 보호).
	 */
	void CleanupTownIntruders();

	/** 마을 침입 정리 타이머 (마을 존이 있을 때만 설정 — 매니저 소유라 수명 안전) */
	FTimerHandle TownCleanupTimerHandle;

	/** 마을 NPC 6인 배치 (docs/12 로스터 — 마을 중심 반경 600, 60도 간격 호) */
	void SpawnTownNpcs(const FVector& TownCenter);

	TArray<FROHZoneDef> Zones;
};
