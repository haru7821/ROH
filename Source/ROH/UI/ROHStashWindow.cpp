#include "UI/ROHStashWindow.h"
#include "Character/ROHPlayerCharacter.h"
#include "Items/ROHInventoryComponent.h"
#include "Items/ROHItemDatabase.h"
#include "Save/ROHAccountSubsystem.h"
#include "Save/ROHSaveSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/GameInstance.h" // GetSubsystem<T>() 템플릿 인스턴스화에 완전한 타입 필요

namespace
{
	// InventoryWindow와 로컬 중복 (익명 네임스페이스 헬퍼 공용화는 UI 2차로 미룸 — 사양 승인)
	FString StashSocketSummary(const UROHItemDatabase& Database, const FROHItemInstance& Item)
	{
		if (Item.MaxSockets <= 0)
		{
			return FString();
		}
		TArray<FString> Parts;
		for (int32 SocketIndex = 0; SocketIndex < Item.MaxSockets; ++SocketIndex)
		{
			if (Item.SocketedRunes.IsValidIndex(SocketIndex))
			{
				const FROHRuneDef* Rune = Database.FindRune(Item.SocketedRunes[SocketIndex]);
				Parts.Add(Rune ? Rune->DisplayName.ToString() : Item.SocketedRunes[SocketIndex].ToString());
			}
			else
			{
				Parts.Add(TEXT("-"));
			}
		}
		return FString::Printf(TEXT(" [%s]"), *FString::Join(Parts, TEXT(",")));
	}

	FLinearColor StashQualityColor(EROHItemQuality Quality)
	{
		return FLinearColor::FromSRGBColor(UROHItemDatabase::GetQualityColor(Quality));
	}
}

void UROHStashWindow::RefreshContents()
{
	if (!ContentBox)
	{
		return;
	}
	ContentBox->ClearChildren();
	StatusText = nullptr;

	AROHPlayerCharacter* Player = GetPlayerCharacter();
	UROHInventoryComponent* Inventory = Player ? Player->GetInventory() : nullptr;
	UROHItemDatabase* Database = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UROHItemDatabase>() : nullptr;
	UROHAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UROHAccountSubsystem>() : nullptr;
	if (!Inventory || !Database || !Account)
	{
		MakeText(ContentBox, TEXT("보관함을 찾을 수 없습니다"), FLinearColor(0.6f, 0.6f, 0.6f));
		return;
	}

	StatusText = MakeText(ContentBox, FString(), FLinearColor(1.f, 0.6f, 0.3f));
	MakeText(ContentBox, FString::Printf(TEXT("보관함 %d/%d (계정 공유 — 입출금 즉시 저장)"),
		Account->GetStashItems().Num(), UROHAccountSubsystem::StashCapacity), FLinearColor(1.f, 0.85f, 0.2f));

	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>();
	ContentBox->AddChild(Columns);

	// --- 좌: 스태시 (클릭 = 인출) ---
	UVerticalBox* StashColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	Columns->AddChild(StashColumn);
	MakeText(StashColumn, TEXT("--- 보관함 (클릭=인출) ---"), FLinearColor(1.f, 0.9f, 0.6f));
	const TArray<FROHItemInstance>& StashItems = Account->GetStashItems();
	for (int32 StashIndex = 0; StashIndex < StashItems.Num(); ++StashIndex)
	{
		UROHActionButton* Button = MakeActionButton(StashColumn, FString::Printf(TEXT("%s%s"),
			*Database->GetItemDisplayName(StashItems[StashIndex]).ToString(),
			*StashSocketSummary(*Database, StashItems[StashIndex])),
			TEXT("Withdraw"), StashIndex, StashQualityColor(StashItems[StashIndex].Quality));
		Button->SetToolTipText(Database->GetItemTooltip(StashItems[StashIndex])); // 호버 상세 (UI 2차)
	}
	if (StashItems.Num() == 0)
	{
		MakeText(StashColumn, TEXT("(비어 있음)"), FLinearColor(0.45f, 0.45f, 0.45f));
	}

	// --- 우: 인벤토리 (클릭 = 보관, 미감정 허용) ---
	UVerticalBox* ItemColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	Columns->AddChild(ItemColumn);
	MakeText(ItemColumn, TEXT("--- 인벤토리 (클릭=보관) ---"), FLinearColor(1.f, 0.9f, 0.6f));
	const TArray<FROHItemInstance>& Items = Inventory->GetItems();
	for (int32 ItemIndex = 0; ItemIndex < Items.Num(); ++ItemIndex)
	{
		UROHActionButton* Button = MakeActionButton(ItemColumn, FString::Printf(TEXT("%s%s"),
			*Database->GetItemDisplayName(Items[ItemIndex]).ToString(),
			*StashSocketSummary(*Database, Items[ItemIndex])),
			TEXT("Deposit"), ItemIndex, StashQualityColor(Items[ItemIndex].Quality));
		Button->SetToolTipText(Database->GetItemTooltip(Items[ItemIndex])); // 호버 상세 (UI 2차)
	}
	if (Items.Num() == 0)
	{
		MakeText(ItemColumn, TEXT("(비어 있음)"), FLinearColor(0.45f, 0.45f, 0.45f));
	}
}

void UROHStashWindow::OnAction(FName InActionId, int32 InActionIndex)
{
	AROHPlayerCharacter* Player = GetPlayerCharacter();
	UROHInventoryComponent* Inventory = Player ? Player->GetInventory() : nullptr;
	UROHAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UROHAccountSubsystem>() : nullptr;
	if (!Inventory || !Account)
	{
		Super::OnAction(InActionId, InActionIndex);
		return;
	}

	FString Status;
	bool bMoved = false; // 성공한 입출금 후 캐릭터도 즉시 저장 (계정/캐릭터 슬롯 동기화)

	if (InActionId == TEXT("Withdraw"))
	{
		if (Inventory->IsFull())
		{
			Status = TEXT("인벤토리가 가득 찼습니다");
		}
		else
		{
			FROHItemInstance Withdrawn;
			if (Account->StashWithdrawAt(InActionIndex, Withdrawn)) // 제거 + 즉시 계정 저장
			{
				Inventory->AddItem(Withdrawn);
				bMoved = true;
			}
		}
	}
	else if (InActionId == TEXT("Deposit"))
	{
		const TArray<FROHItemInstance>& Items = Inventory->GetItems();
		if (!Items.IsValidIndex(InActionIndex))
		{
			Status = TEXT("잘못된 인덱스");
		}
		else if (Account->IsStashFull())
		{
			Status = FString::Printf(TEXT("보관함이 가득 찼습니다 (%d칸)"), UROHAccountSubsystem::StashCapacity);
		}
		else
		{
			// 보관 성공 확정 후 인벤토리에서 제거 — 실패 경로에서 아이템 소실 금지
			const FROHItemInstance Deposited = Items[InActionIndex];
			if (Account->StashDeposit(Deposited)) // 추가 + 즉시 계정 저장
			{
				Inventory->RemoveItemAt(InActionIndex);
				bMoved = true;
			}
		}
	}
	else
	{
		Super::OnAction(InActionId, InActionIndex);
		return;
	}

	// 계정 슬롯만 저장하고 인벤토리 변화가 메모리에 남으면 재기동 시 소실(인출)/복제(보관)가
	// 생긴다 — 입출금 성공 시 캐릭터 슬롯도 같이 스냅샷해 두 슬롯을 항상 동기화한다
	if (bMoved && Player && GetGameInstance())
	{
		if (UROHSaveSubsystem* SaveSystem = GetGameInstance()->GetSubsystem<UROHSaveSubsystem>())
		{
			SaveSystem->SaveCharacter(Player);
		}
	}

	RefreshContents();
	if (!Status.IsEmpty() && StatusText)
	{
		StatusText->SetText(FText::FromString(Status));
	}
}
