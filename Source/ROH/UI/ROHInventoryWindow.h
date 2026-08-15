#pragma once

#include "CoreMinimal.h"
#include "UI/ROHUiWindow.h"
#include "ROHInventoryWindow.generated.h"

class UTextBlock;

/**
 * 인벤토리/장비 창 (I 키). 좌: 장비창(클릭=해제) + 세트 집계, 우: 인벤토리(클릭=장착/물약 사용).
 * 룬/재료는 표시만 (소켓/합성은 콘솔 — UI 2차에서 확장).
 */
UCLASS()
class ROH_API UROHInventoryWindow : public UROHUiWindow
{
	GENERATED_BODY()

public:
	virtual void OnAction(FName InActionId, int32 InActionIndex) override;
	virtual void RefreshContents() override;

protected:
	virtual FText GetWindowTitle() const override
	{
		return NSLOCTEXT("ROH", "InventoryWindowTitle", "인벤토리 (I)");
	}

private:
	/** 동작 실패 사유 표시줄 (미감정 장착 시도 등 — RefreshContents마다 재생성) */
	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;
};
