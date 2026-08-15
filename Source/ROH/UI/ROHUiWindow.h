#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "ROHUiWindow.generated.h"

class UPanelWidget;
class UTextBlock;
class UVerticalBox;
class AROHPlayerCharacter;
class UROHUiWindow;

/**
 * 행 버튼 (그레이박스 UI 공용): 클릭 → 소유 창의 OnAction(ActionId, ActionIndex).
 * AddDynamic 대상이라 핸들러는 UFUNCTION 필수.
 */
UCLASS()
class ROH_API UROHActionButton : public UButton
{
	GENERATED_BODY()

public:
	/** 소유 창/액션 지정 + 클릭 델리게이트 바인딩 (WidgetTree 생성 직후 1회) */
	void InitAction(UROHUiWindow* InOwnerWindow, FName InActionId, int32 InActionIndex);

protected:
	UFUNCTION()
	void HandleClick();

private:
	TWeakObjectPtr<UROHUiWindow> OwnerWindow;
	FName ActionId;
	int32 ActionIndex = -1;
	bool bBound = false;
};

/**
 * 순수 C++ UMG 창 베이스 (그레이박스 — 에디터 애셋 금지, WidgetTree 수동 구성).
 * 프레임(반투명 배경/타이틀/스크롤 내용/[닫기])은 NativeOnInitialized에서 구성한다 —
 * NativeConstruct 시점엔 TakeWidget이 이미 RootWidget으로 슬레이트를 만든 뒤라 너무 늦다.
 * 파생 창은 RefreshContents에서 ContentBox를 전체 재구성한다 (간단 우선, 툴팁/드래그는 2차).
 */
UCLASS(Abstract)
class ROH_API UROHUiWindow : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 행 버튼 클릭 처리. 베이스는 "Close"만 처리 — 파생은 자기 액션 처리 후 Super 호출 */
	virtual void OnAction(FName InActionId, int32 InActionIndex);

	/** 내용 전체 재구성 (열기/동작 후마다 호출) */
	virtual void RefreshContents() {}

	/**
	 * 내용 재구성 + Serial 캐시 동기화 (UI 3차 — b31).
	 * 수동 갱신 경로(NativeConstruct/파생 OnAction)는 RefreshContents 대신 이걸 호출한다 —
	 * 캐시가 함께 갱신되어 직후 틱의 중복 리빌드를 막는다.
	 */
	void RefreshNow();

	/** 소유 컨트롤러에 닫기 요청 */
	void RequestClose();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/**
	 * 창이 관심 갖는 데이터의 버전 합 (b31 dirty 폴링): 파생이 인벤토리/프로그레션 등
	 * 관련 Serial(단조 증가)의 합을 반환한다. 0 반환(기본) = 자동 갱신 없음.
	 * NativeTick은 int 비교만 수행 — Serial 불변이면 절대 리빌드하지 않는다.
	 */
	virtual int32 ComputeContentSerial() const { return 0; }

	/** 파생 창의 타이틀 문자열 */
	virtual FText GetWindowTitle() const { return FText::GetEmpty(); }

	AROHPlayerCharacter* GetPlayerCharacter() const;

	// --- 구성 헬퍼 ---
	UTextBlock* MakeText(UPanelWidget* Parent, const FString& InText, const FLinearColor& Color);
	UROHActionButton* MakeActionButton(UPanelWidget* Parent, const FString& Label, FName InActionId, int32 InActionIndex,
		const FLinearColor& LabelColor, bool bEnabled = true);

	/** 파생 창 내용 컨테이너 (스크롤 내부) */
	UPROPERTY()
	TObjectPtr<UVerticalBox> ContentBox;

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleText;

private:
	bool bFrameBuilt = false;

	/** 마지막 재구성 시점의 Serial 캐시 (b31) */
	int32 CachedContentSerial = 0;
};
