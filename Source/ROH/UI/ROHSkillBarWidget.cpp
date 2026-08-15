#include "UI/ROHSkillBarWidget.h"
#include "Character/ROHPlayerCharacter.h"
#include "Abilities/ROHGameplayAbility.h"
#include "Abilities/ROHGameplayEffects.h" // UROHFrenzyEffect/UROHBattleShoutEffect (버프 잔여 조회)
#include "Progression/ROHSkillTreeComponent.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

namespace
{
	// 슬롯 키 안내 (인덱스 = ActivateAbilityBySlot 슬롯)
	const TCHAR* SkillBarKeyLabel(int32 SlotIndex)
	{
		switch (SlotIndex)
		{
		case 0:  return TEXT("[우클릭]");
		case 1:  return TEXT("[1]");
		case 2:  return TEXT("[2]");
		case 3:  return TEXT("[3]");
		case 4:  return TEXT("[4]");
		default: return TEXT("[?]");
		}
	}

	// 지정 GE 클래스의 최대 잔여 시간 (버프 표시용 — 없으면 0)
	float SkillBarEffectTimeRemaining(const UAbilitySystemComponent& ASC, TSubclassOf<UGameplayEffect> EffectClass)
	{
		FGameplayEffectQuery Query;
		Query.EffectDefinition = EffectClass;
		float Best = 0.f;
		for (const float Remaining : ASC.GetActiveEffectsTimeRemaining(Query))
		{
			Best = FMath::Max(Best, Remaining);
		}
		return Best;
	}
}

UTextBlock* UROHSkillBarWidget::MakeText(UPanelWidget* Parent, const FString& InText, const FLinearColor& Color)
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

void UROHSkillBarWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree)
	{
		return;
	}

	// 상시 HUD — 하단 클릭 이동을 막지 않는다
	SetVisibility(ESlateVisibility::HitTestInvisible);

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Canvas;

	// 하단 중앙 반투명 패널 (좌상단 디버그 메시지/우상단 미니맵과 위치 분리)
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.6f));
	Background->SetPadding(FMargin(12.f, 6.f));
	if (UCanvasPanelSlot* BackgroundSlot = Canvas->AddChildToCanvas(Background))
	{
		BackgroundSlot->SetAnchors(FAnchors(0.5f, 1.f));
		BackgroundSlot->SetAlignment(FVector2D(0.5f, 1.f));
		BackgroundSlot->SetPosition(FVector2D(0.f, -12.f));
		BackgroundSlot->SetAutoSize(true);
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Background->SetContent(Column);

	// 버프 잔여 1줄 (내용은 NativeTick이 채운다)
	BuffText = MakeText(Column, FString(), FLinearColor(0.5f, 1.f, 0.5f));

	UHorizontalBox* SlotRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	Column->AddChild(SlotRow);

	SlotNameTexts.Reset();
	SlotCooldownTexts.Reset();
	for (int32 SlotIndex = 0; SlotIndex <= AROHPlayerCharacter::MaxSkillSlot; ++SlotIndex)
	{
		UVerticalBox* SlotBox = WidgetTree->ConstructWidget<UVerticalBox>();
		SlotRow->AddChild(SlotBox);
		if (UHorizontalBoxSlot* RowSlot = Cast<UHorizontalBoxSlot>(SlotBox->Slot))
		{
			RowSlot->SetPadding(FMargin(10.f, 0.f));
		}

		MakeText(SlotBox, SkillBarKeyLabel(SlotIndex), FLinearColor(0.8f, 0.8f, 0.5f));
		SlotNameTexts.Add(MakeText(SlotBox, TEXT("-"), FLinearColor::White));
		SlotCooldownTexts.Add(MakeText(SlotBox, FString(), FLinearColor(0.6f, 0.6f, 0.6f)));
	}
}

void UROHSkillBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	AROHPlayerCharacter* Player = Cast<AROHPlayerCharacter>(GetOwningPlayerPawn());
	UAbilitySystemComponent* ASC = Player ? Player->GetAbilitySystemComponent() : nullptr;
	const UROHSkillTreeComponent* SkillTree = Player ? Player->GetSkillTree() : nullptr;
	if (!Player || !ASC || !SkillTree)
	{
		return;
	}

	const TArray<FROHSkillDef>& Defs = UROHSkillTreeComponent::GetSkillDefs(SkillTree->GetPlayerClass());

	for (int32 SlotIndex = 0; SlotIndex < SlotNameTexts.Num() && SlotIndex < SlotCooldownTexts.Num(); ++SlotIndex)
	{
		UTextBlock* NameText = SlotNameTexts[SlotIndex];
		UTextBlock* CooldownText = SlotCooldownTexts[SlotIndex];
		if (!NameText || !CooldownText)
		{
			continue;
		}

		const TSubclassOf<UROHGameplayAbility> AbilityClass = Player->GetSlotAbilityClass(SlotIndex);
		if (!AbilityClass)
		{
			NameText->SetText(FText::FromString(TEXT("-")));
			NameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.45f, 0.45f)));
			CooldownText->SetText(FText::GetEmpty());
			continue;
		}

		// 이름: 스킬 레지스트리 역조회 (기본 공격 어빌리티는 트리 밖)
		FString SkillName = TEXT("기본 공격");
		for (const FROHSkillDef& Def : Defs)
		{
			if (Def.AbilityClass == AbilityClass)
			{
				SkillName = Def.DisplayName.ToString();
				break;
			}
		}
		NameText->SetText(FText::FromString(SkillName));

		// 쿨다운 잔여: 어빌리티 CDO의 쿨다운 태그로 활성 쿨다운 GE 잔여 조회
		const UROHGameplayAbility* AbilityCDO = AbilityClass->GetDefaultObject<UROHGameplayAbility>();
		float CooldownRemaining = 0.f;
		if (const FGameplayTagContainer* CooldownTagSet = AbilityCDO ? AbilityCDO->GetCooldownTags() : nullptr)
		{
			if (!CooldownTagSet->IsEmpty())
			{
				for (const float Remaining : ASC->GetActiveEffectsTimeRemaining(
					FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(*CooldownTagSet)))
				{
					CooldownRemaining = FMath::Max(CooldownRemaining, Remaining);
				}
			}
		}
		CooldownText->SetText(CooldownRemaining > 0.05f
			? FText::FromString(FString::Printf(TEXT("%.1f"), CooldownRemaining))
			: FText::GetEmpty());

		// 색: 쿨다운 중 회색 > 리소스 부족 빨강 > 평시 흰색
		const bool bInsufficient = AbilityCDO && AbilityCDO->GetCostAttribute().IsValid()
			&& AbilityCDO->GetCostAmount() > 0.f
			&& ASC->GetNumericAttribute(AbilityCDO->GetCostAttribute()) < AbilityCDO->GetCostAmount();
		if (CooldownRemaining > 0.05f)
		{
			NameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.55f, 0.55f)));
		}
		else if (bInsufficient)
		{
			NameText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.35f, 0.35f)));
		}
		else
		{
			NameText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		}
	}

	// 버프 잔여 1줄 (광란/전투의 함성 — Duration GE 클래스로 필터)
	if (BuffText)
	{
		TArray<FString> BuffParts;
		const float ShoutRemaining = SkillBarEffectTimeRemaining(*ASC, UROHBattleShoutEffect::StaticClass());
		if (ShoutRemaining > 0.05f)
		{
			BuffParts.Add(FString::Printf(TEXT("전투의 함성 %.1f초"), ShoutRemaining));
		}
		const float FrenzyRemaining = SkillBarEffectTimeRemaining(*ASC, UROHFrenzyEffect::StaticClass());
		if (FrenzyRemaining > 0.05f)
		{
			BuffParts.Add(FString::Printf(TEXT("광란 %.1f초"), FrenzyRemaining));
		}
		BuffText->SetText(FText::FromString(FString::Join(BuffParts, TEXT(" | "))));
	}
}
