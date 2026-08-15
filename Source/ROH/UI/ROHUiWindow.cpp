#include "UI/ROHUiWindow.h"
#include "Core/ROHPlayerController.h"
#include "Character/ROHPlayerCharacter.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

void UROHActionButton::InitAction(UROHUiWindow* InOwnerWindow, FName InActionId, int32 InActionIndex)
{
	OwnerWindow = InOwnerWindow;
	ActionId = InActionId;
	ActionIndex = InActionIndex;
	if (!bBound)
	{
		bBound = true;
		OnClicked.AddDynamic(this, &UROHActionButton::HandleClick);
	}
}

void UROHActionButton::HandleClick()
{
	if (OwnerWindow.IsValid())
	{
		OwnerWindow->OnAction(ActionId, ActionIndex);
	}
}

void UROHUiWindow::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 프레임은 1회만 구성 — RootWidget은 TakeWidget(뷰포트 추가) 이전에 준비돼야 한다
	if (bFrameBuilt || !WidgetTree)
	{
		return;
	}
	bFrameBuilt = true;

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Canvas;

	// 중앙 정렬 반투명 패널
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.92f));
	Background->SetPadding(FMargin(16.f));
	if (UCanvasPanelSlot* BackgroundSlot = Canvas->AddChildToCanvas(Background))
	{
		BackgroundSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		BackgroundSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		BackgroundSlot->SetAutoSize(true);
	}

	UVerticalBox* Frame = WidgetTree->ConstructWidget<UVerticalBox>();
	Background->SetContent(Frame);

	TitleText = MakeText(Frame, GetWindowTitle().ToString(), FLinearColor(1.f, 0.9f, 0.6f));

	// 내용 스크롤 영역 (창 크기 고정 — 목록이 길어도 프레임 유지)
	USizeBox* ScrollBounds = WidgetTree->ConstructWidget<USizeBox>();
	ScrollBounds->SetWidthOverride(560.f);
	ScrollBounds->SetHeightOverride(480.f);
	Frame->AddChild(ScrollBounds);

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	ScrollBounds->SetContent(Scroll);

	ContentBox = WidgetTree->ConstructWidget<UVerticalBox>();
	Scroll->AddChild(ContentBox);

	MakeActionButton(Frame, TEXT("[닫기]"), TEXT("Close"), -1, FLinearColor(0.8f, 0.8f, 0.8f));
}

void UROHUiWindow::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshNow();
}

void UROHUiWindow::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// dirty 폴링 (b31): 창 열림 중 외부 변이(E 습득/골드/레벨업 등) 반영.
	// 비용은 int 비교뿐 — Serial 불변이면 절대 리빌드하지 않는다 (호버 중 불필요 리빌드 금지)
	if (ComputeContentSerial() != CachedContentSerial)
	{
		RefreshNow();
	}
}

void UROHUiWindow::RefreshNow()
{
	RefreshContents();
	// 재구성 결과 기준으로 동기화 (재구성 자체는 데이터를 변이하지 않지만, 액션 경로에서
	// 변이 → RefreshNow 순서로 호출되므로 여기서 읽어야 직후 틱 중복 리빌드가 없다)
	CachedContentSerial = ComputeContentSerial();
}

void UROHUiWindow::OnAction(FName InActionId, int32 InActionIndex)
{
	if (InActionId == TEXT("Close"))
	{
		RequestClose();
	}
}

void UROHUiWindow::RequestClose()
{
	if (AROHPlayerController* PC = Cast<AROHPlayerController>(GetOwningPlayer()))
	{
		PC->CloseUiWindow();
	}
	else
	{
		RemoveFromParent();
	}
}

AROHPlayerCharacter* UROHUiWindow::GetPlayerCharacter() const
{
	return Cast<AROHPlayerCharacter>(GetOwningPlayerPawn());
}

UTextBlock* UROHUiWindow::MakeText(UPanelWidget* Parent, const FString& InText, const FLinearColor& Color)
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(FText::FromString(InText));
	Text->SetColorAndOpacity(FSlateColor(Color));
	if (Parent)
	{
		Parent->AddChild(Text);
	}
	return Text;
}

UROHActionButton* UROHUiWindow::MakeActionButton(UPanelWidget* Parent, const FString& Label, FName InActionId, int32 InActionIndex,
	const FLinearColor& LabelColor, bool bEnabled)
{
	UROHActionButton* Button = WidgetTree->ConstructWidget<UROHActionButton>();
	Button->InitAction(this, InActionId, InActionIndex);
	Button->SetIsEnabled(bEnabled);
	MakeText(Button, Label, LabelColor);
	if (Parent)
	{
		Parent->AddChild(Button);
	}
	return Button;
}
