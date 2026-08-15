#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ROHStashChest.generated.h"

class UStaticMeshComponent;

/**
 * 계정 공유 보관함 상자 (M5 최종): 마을 웨이포인트 옆에 지역 매니저가 스폰.
 * 그레이박스 낮은 상자 + 금색 링. 접근만으로는 아무 일 없음 — E(Interact 체인)로 보관함 창.
 * 내용물의 소유자는 UROHAccountSubsystem (이 액터는 월드 접점일 뿐).
 */
UCLASS()
class ROH_API AROHStashChest : public AActor
{
	GENERATED_BODY()

public:
	AROHStashChest();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	/** 그레이박스 상자 (엔진 기본 셰이프 — BeginPlay에서 로드, 60×60×50) */
	UPROPERTY(VisibleAnywhere, Category = "ROH|Stash")
	TObjectPtr<UStaticMeshComponent> ChestMesh;
};
