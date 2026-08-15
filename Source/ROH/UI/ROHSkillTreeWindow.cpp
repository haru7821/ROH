#include "UI/ROHSkillTreeWindow.h"
#include "Character/ROHPlayerCharacter.h"
#include "Progression/ROHProgressionComponent.h"
#include "Progression/ROHSkillTreeComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

int32 UROHSkillTreeWindow::ComputeContentSerial() const
{
	const AROHPlayerCharacter* Player = GetPlayerCharacter();
	const UROHSkillTreeComponent* SkillTree = Player ? Player->GetSkillTree() : nullptr;
	const UROHProgressionComponent* Progression = Player ? Player->GetProgression() : nullptr;
	return (SkillTree ? SkillTree->GetChangeSerial() : 0) + (Progression ? Progression->GetChangeSerial() : 0);
}

void UROHSkillTreeWindow::RefreshContents()
{
	if (!ContentBox)
	{
		return;
	}
	ContentBox->ClearChildren();
	StatusText = nullptr;

	AROHPlayerCharacter* Player = GetPlayerCharacter();
	UROHSkillTreeComponent* SkillTree = Player ? Player->GetSkillTree() : nullptr;
	const UROHProgressionComponent* Progression = Player ? Player->GetProgression() : nullptr;
	if (!SkillTree || !Progression)
	{
		MakeText(ContentBox, TEXT("스킬트리를 찾을 수 없습니다"), FLinearColor(0.6f, 0.6f, 0.6f));
		return;
	}

	MakeText(ContentBox, FString::Printf(TEXT("스킬 포인트: %d | 스탯 포인트: %d"),
		Progression->GetSkillPoints(), Progression->GetStatPoints()), FLinearColor::White);
	StatusText = MakeText(ContentBox, FString(), FLinearColor(1.f, 0.6f, 0.3f));

	const TArray<FROHSkillDef>& Defs = UROHSkillTreeComponent::GetSkillDefs(SkillTree->GetPlayerClass());
	const TArray<FName> BoundSlots = Player->ExportBoundSkills(); // 인덱스 0~3 = 슬롯 1~4
	const int32 PlayerLevel = Progression->GetLevel();

	FString LastTreeName;
	for (int32 DefIndex = 0; DefIndex < Defs.Num(); ++DefIndex)
	{
		const FROHSkillDef& Def = Defs[DefIndex];

		// 계열 섹션 헤더 (레지스트리가 계열순으로 나열됨)
		const FString TreeNameString = Def.TreeName.ToString();
		if (TreeNameString != LastTreeName)
		{
			LastTreeName = TreeNameString;
			MakeText(ContentBox, FString::Printf(TEXT("=== %s ==="), *TreeNameString), FLinearColor(1.f, 0.9f, 0.6f));
		}

		const int32 Rank = SkillTree->GetRank(Def.SkillId);
		const bool bLocked = (PlayerLevel < Def.RequiredLevel)
			|| (!Def.PrereqSkillId.IsNone() && SkillTree->GetRank(Def.PrereqSkillId) <= 0);

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		ContentBox->AddChild(Row);

		// [+] 투자 (미해금/만렙이면 비활성 — 사유는 InvestPoint 검증과 동일 규칙)
		MakeActionButton(Row, TEXT("[+]"), TEXT("Invest"), DefIndex,
			FLinearColor(0.4f, 1.f, 0.4f), !bLocked && Rank < Def.MaxPoints);

		const FString RowLabel = FString::Printf(TEXT(" %s %d/%d [%s] (Lv%d+)"),
			*Def.DisplayName.ToString(), Rank, Def.MaxPoints,
			Def.Kind == EROHSkillKind::Active ? TEXT("액티브") : TEXT("패시브"), Def.RequiredLevel);
		const FLinearColor RowColor = bLocked ? FLinearColor(0.45f, 0.45f, 0.45f)
			: (Rank > 0 ? FLinearColor::White : FLinearColor(0.75f, 0.75f, 0.75f));
		MakeText(Row, RowLabel, RowColor);

		// 액티브: 슬롯 배치 미니 버튼 [1]~[4] (배치된 슬롯 = 초록 강조)
		if (Def.Kind == EROHSkillKind::Active)
		{
			for (int32 SlotNumber = 1; SlotNumber <= AROHPlayerCharacter::MaxSkillSlot; ++SlotNumber)
			{
				const bool bBoundHere = BoundSlots.IsValidIndex(SlotNumber - 1) && BoundSlots[SlotNumber - 1] == Def.SkillId;
				MakeActionButton(Row, FString::Printf(TEXT("[%d]"), SlotNumber),
					FName(*FString::Printf(TEXT("Bind%d"), SlotNumber)), DefIndex,
					bBoundHere ? FLinearColor(0.3f, 1.f, 0.3f) : FLinearColor(0.6f, 0.6f, 0.6f),
					Rank > 0);
			}
		}
	}
}

void UROHSkillTreeWindow::OnAction(FName InActionId, int32 InActionIndex)
{
	AROHPlayerCharacter* Player = GetPlayerCharacter();
	UROHSkillTreeComponent* SkillTree = Player ? Player->GetSkillTree() : nullptr;
	const EROHPlayerClass PlayerClass = SkillTree ? SkillTree->GetPlayerClass() : EROHPlayerClass::Warrior;
	const TArray<FROHSkillDef>& Defs = UROHSkillTreeComponent::GetSkillDefs(PlayerClass);

	auto ShowStatus = [this](const FString& Message)
	{
		if (StatusText)
		{
			StatusText->SetText(FText::FromString(Message));
		}
	};

	if (InActionId == TEXT("Invest") && SkillTree && Defs.IsValidIndex(InActionIndex))
	{
		FString Error;
		if (SkillTree->InvestPoint(Defs[InActionIndex].SkillId, Error))
		{
			RefreshNow();
		}
		else
		{
			ShowStatus(Error);
		}
		return;
	}

	// Bind1~Bind4: 액티브 스킬 슬롯 배치
	const FString ActionString = InActionId.ToString();
	if (ActionString.StartsWith(TEXT("Bind")) && Player && Defs.IsValidIndex(InActionIndex))
	{
		const int32 SlotNumber = FCString::Atoi(*ActionString.RightChop(4));
		FString Error;
		if (Player->BindSkillToSlot(SlotNumber, Defs[InActionIndex].SkillId, Error))
		{
			RefreshNow();
		}
		else
		{
			ShowStatus(Error);
		}
		return;
	}

	Super::OnAction(InActionId, InActionIndex);
}
