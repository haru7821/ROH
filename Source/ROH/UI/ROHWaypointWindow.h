#pragma once

#include "CoreMinimal.h"
#include "UI/ROHUiWindow.h"
#include "ROHWaypointWindow.generated.h"

/**
 * 웨이포인트 지역 선택 창 (UI 1차 최우선 — 사용자 결정: E 순환 이동 대체).
 * 활성화된 지역 행 클릭 = 해당 웨이포인트로 이동 후 창 닫기, 미발견/현재 지역은 비활성.
 */
UCLASS()
class ROH_API UROHWaypointWindow : public UROHUiWindow
{
	GENERATED_BODY()

public:
	virtual void OnAction(FName InActionId, int32 InActionIndex) override;
	virtual void RefreshContents() override;

protected:
	virtual FText GetWindowTitle() const override
	{
		return NSLOCTEXT("ROH", "WaypointWindowTitle", "웨이포인트 — 이동할 지역 선택");
	}
};
