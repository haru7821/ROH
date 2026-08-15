#pragma once

#include "CoreMinimal.h"
#include "UI/ROHUiWindow.h"
#include "ROHStashWindow.generated.h"

class UTextBlock;

/**
 * 계정 보관함 창 (M5 최종): 좌 = 스태시(클릭=인출), 우 = 인벤토리(클릭=보관).
 * 내용물은 UROHAccountSubsystem 소유 — 입출금마다 즉시 계정 저장.
 * 미감정 보관 허용 (감정은 셀바에서만).
 */
UCLASS()
class ROH_API UROHStashWindow : public UROHUiWindow
{
	GENERATED_BODY()

public:
	virtual void OnAction(FName InActionId, int32 InActionIndex) override;
	virtual void RefreshContents() override;

protected:
	virtual FText GetWindowTitle() const override
	{
		return NSLOCTEXT("ROH", "StashWindowTitle", "계정 보관함");
	}

private:
	/** 가득 참 등 실패 사유 표시줄 (RefreshContents마다 재생성) */
	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;
};
