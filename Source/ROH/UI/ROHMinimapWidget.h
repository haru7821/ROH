#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ROHMinimapWidget.generated.h"

/**
 * 미니맵 (b31, 우상단 200×200 — 애셋 없음): NativePaint에서 FSlateDrawElement로 점 직접 그리기.
 * 반경 4000 범위를 플레이어 중심 고정(북쪽=X+ 위)으로 축척.
 * 점 색: 플레이어 흰(중앙) / 몬스터 빨강 / 정예 주황 / 보스 자주 / NPC 초록 / 웨이포인트 파랑 / 보관함 노랑.
 * 액터 수집은 0.25초 캐시(TWeakObjectPtr — 수명 안전), Paint는 캐시만 소비.
 * HitTestInvisible — 우상단 클릭 이동을 막지 않는다.
 */
UCLASS()
class ROH_API UROHMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	/** 미니맵 표식 (코드 전용 — 0.25s 수집 캐시) */
	struct FROHMinimapMark
	{
		TWeakObjectPtr<const AActor> Actor;
		FLinearColor Color = FLinearColor::White;
		/** 캐릭터 표식: 그리기 직전 IsAlive 재검증 (사망 몬스터 즉시 제외) */
		bool bCheckAlive = false;
	};

	void RebuildMarkCache();

	TArray<FROHMinimapMark> CachedMarks;
	float CacheTimer = 0.f;

	/** 표시 반경 (월드 단위) */
	static constexpr float WorldRadius = 4000.f;
	static constexpr float CacheInterval = 0.25f;
};
