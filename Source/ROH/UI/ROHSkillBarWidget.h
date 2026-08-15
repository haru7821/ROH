#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ROHSkillBarWidget.generated.h"

class UTextBlock;
class UPanelWidget;

/**
 * 스킬바 HUD (b31, 하단 상시 표시 — 순수 C++ UMG, 애셋 없음):
 * [우클릭] 기본공격 + [1]~[4] 배치 스킬. 슬롯마다 스킬 한글명 / 쿨다운 잔여초,
 * 리소스(마나/분노) 부족 시 이름 색 변화. 상단 1줄에 버프(광란/전투의 함성) 잔여초.
 * 항목 수 고정 5라 매 틱 텍스트 갱신 허용 (창 dirty 폴링과 달리 리빌드 없음 — SetText만).
 * HitTestInvisible — 하단 클릭 이동을 막지 않는다.
 */
UCLASS()
class ROH_API UROHSkillBarWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UTextBlock* MakeText(UPanelWidget* Parent, const FString& InText, const FLinearColor& Color);

	/** 버프 잔여 표시줄 (없으면 빈 문자열) */
	UPROPERTY()
	TObjectPtr<UTextBlock> BuffText;

	/** 슬롯별 스킬명 (인덱스 0=기본공격, 1~4=스킬 슬롯) */
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> SlotNameTexts;

	/** 슬롯별 쿨다운 잔여초 */
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> SlotCooldownTexts;
};
