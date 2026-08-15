#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ROHWaypoint.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 * 웨이포인트 (docs/04 M4): 지역 중심에 서는 기둥.
 * 접촉으로 활성화(캠페인 서브시스템에 기록·세이브 연동)하고,
 * 근처에서 E(상호작용)로 지역 선택 창을 연다 (UI 1차 — 활성화 지역 클릭 이동).
 * 활성화 상태의 소유자는 서브시스템 — 이 액터는 표시/입력 접점만 담당.
 */
UCLASS()
class ROH_API AROHWaypoint : public AActor
{
	GENERATED_BODY()

public:
	AROHWaypoint();

	virtual void Tick(float DeltaSeconds) override;

	/** 지역 매니저가 지연 스폰 중(FinishSpawning 전) 주입 */
	void SetZoneInfo(int32 InZoneIndex, const FText& InZoneName);

	int32 GetZoneIndex() const { return ZoneIndex; }
	FText GetZoneName() const { return ZoneName; }

	/** 캠페인 서브시스템 질의 (상태는 저쪽이 소유) */
	bool IsActivated() const;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** 그레이박스 기둥 (엔진 기본 셰이프 — BeginPlay에서 로드) */
	UPROPERTY(VisibleAnywhere, Category = "ROH|Waypoint")
	TObjectPtr<UStaticMeshComponent> PillarMesh;

	/** 접촉 활성화 범위 */
	UPROPERTY(VisibleAnywhere, Category = "ROH|Waypoint")
	TObjectPtr<USphereComponent> ActivationSphere;

private:
	int32 ZoneIndex = 0;
	FText ZoneName;
};
