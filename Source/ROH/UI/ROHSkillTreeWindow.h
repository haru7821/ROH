#pragma once

#include "CoreMinimal.h"
#include "UI/ROHUiWindow.h"
#include "ROHSkillTreeWindow.generated.h"

class UTextBlock;

/**
 * 스킬트리 창 (K 키). 계열별 섹션 + 스킬 행:
 * [+] 투자, 액티브는 [1][2][3][4] 슬롯 배치 미니 버튼 (배치된 슬롯 강조).
 * 실패 사유는 상단 상태줄 하나에 표시 (행마다 늘어놓지 않음).
 */
UCLASS()
class ROH_API UROHSkillTreeWindow : public UROHUiWindow
{
	GENERATED_BODY()

public:
	virtual void OnAction(FName InActionId, int32 InActionIndex) override;
	virtual void RefreshContents() override;

protected:
	virtual FText GetWindowTitle() const override
	{
		return NSLOCTEXT("ROH", "SkillTreeWindowTitle", "스킬트리 (K)");
	}

private:
	/** 투자/배치 실패 사유 표시줄 (RefreshContents마다 재생성) */
	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;
};
