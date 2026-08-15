#pragma once

#include "CoreMinimal.h"
#include "UI/ROHUiWindow.h"
#include "ROHParagonWindow.generated.h"

class UTextBlock;

/**
 * 정복자 창 (P 키, UI 2차): 정복자 레벨/XP 진행/미사용 포인트 + 4분류
 * (공격/방어/정밀/룬 조율) "현재 투자/100 (포인트당 효과)" 행과 [+1] 투자 버튼.
 * 투자 성공 시 UROHAccountSubsystem이 즉시 저장하고 플레이어 GE를 재적용한다
 * (치트 ROHParagonUp과 동일 경로). 캐릭터 레벨 50 미만이면 잠금 안내만 표시.
 */
UCLASS()
class ROH_API UROHParagonWindow : public UROHUiWindow
{
	GENERATED_BODY()

public:
	virtual void OnAction(FName InActionId, int32 InActionIndex) override;
	virtual void RefreshContents() override;

protected:
	virtual FText GetWindowTitle() const override
	{
		return NSLOCTEXT("ROH", "ParagonWindowTitle", "정복자 (P)");
	}

	/** 정복자 XP/포인트 + 레벨업(잠금 해제) 자동 반영 (b31 dirty 폴링) */
	virtual int32 ComputeContentSerial() const override;

private:
	/** 투자 실패 사유 표시줄 (RefreshContents마다 재생성) */
	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;
};
