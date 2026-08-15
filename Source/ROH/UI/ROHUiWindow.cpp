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
	RefreshContents();
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
