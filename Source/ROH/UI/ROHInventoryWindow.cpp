#include "UI/ROHInventoryWindow.h"
#include "Character/ROHPlayerCharacter.h"
#include "Items/ROHInventoryComponent.h"
#include "Items/ROHItemDatabase.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Engine/GameInstance.h" // GetSubsystem<T>() 템플릿 인스턴스화에 완전한 타입 필요

namespace
{
	const TCHAR* EquipSlotLabel(EROHEquipSlot Slot)
	{
		switch (Slot)
		{
		case EROHEquipSlot::Weapon: return TEXT("무기");
		case EROHEquipSlot::Shield: return TEXT("방패");
		case EROHEquipSlot::Helm:   return TEXT("투구");
		case EROHEquipSlot::Chest:  return TEXT("흉갑");
		case EROHEquipSlot::Boots:  return TEXT("장화");
		default:                    return TEXT("?");
		}
	}

	// 소켓 요약: " [카르,-]" (콘솔 ROHDumpInventory와 동일 규칙)
	FString SocketSummary(const UROHItemDatabase& Database, const FROHItemInstance& Item)
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

	FLinearColor QualityLinearColor(EROHItemQuality Quality)
	{
		return FLinearColor::FromSRGBColor(UROHItemDatabase::GetQualityColor(Quality));
	}
}

void UROHInventoryWindow::RefreshContents()
{
	if (!ContentBox)
	{
		return;
	}
	ContentBox->ClearChildren();

	AROHPlayerCharacter* Player = GetPlayerCharacter();
	UROHInventoryComponent* Inventory = Player ? Player->GetInventory() : nullptr;
	UROHItemDatabase* Database = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UROHItemDatabase>() : nullptr;
	if (!Inventory || !Database)
	{
		MakeText(ContentBox, TEXT("인벤토리를 찾을 수 없습니다"), FLinearColor(0.6f, 0.6f, 0.6f));
		return;
	}

	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>();
	ContentBox->AddChild(Columns);

	// --- 좌: 장비창 (클릭 = 해제) ---
	UVerticalBox* EquipColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	Columns->AddChild(EquipColumn);
	MakeText(EquipColumn, TEXT("--- 장비창 (클릭=해제) ---"), FLinearColor(1.f, 0.9f, 0.6f));

	static const EROHEquipSlot SlotOrder[] = {
		EROHEquipSlot::Weapon, EROHEquipSlot::Shield, EROHEquipSlot::Helm, EROHEquipSlot::Chest, EROHEquipSlot::Boots };
	const TMap<EROHEquipSlot, FROHItemInstance>& Equipped = Inventory->GetEquipped();
	// 주의: 지역변수명 Slot 금지 — UWidget 상속 멤버 Slot 섀도잉(=에러)
	for (const EROHEquipSlot EquipSlot : SlotOrder)
	{
		if (const FROHItemInstance* Item = Equipped.Find(EquipSlot))
		{
			const FString Label = FString::Printf(TEXT("%s: %s%s"), EquipSlotLabel(EquipSlot),
				*Database->GetItemDisplayName(*Item).ToString(), *SocketSummary(*Database, *Item));
			MakeActionButton(EquipColumn, Label, TEXT("Unequip"), static_cast<int32>(EquipSlot),
				QualityLinearColor(Item->Quality), true);
		}
		else
		{
			MakeActionButton(EquipColumn, FString::Printf(TEXT("%s: -"), EquipSlotLabel(EquipSlot)),
				TEXT("Unequip"), static_cast<int32>(EquipSlot), FLinearColor(0.45f, 0.45f, 0.45f), false);
		}
	}

	// 세트 장착 집계 (M5 2차)
	{
		TMap<FName, int32> SetCounts;
		for (const auto& Pair : Equipped)
		{
			if (Pair.Value.SetPieceId.IsNone())
			{
				continue;
			}
			const FROHSetDef* OwningSet = nullptr;
			if (Database->FindSetPiece(Pair.Value.SetPieceId, &OwningSet) && OwningSet)
			{
				++SetCounts.FindOrAdd(OwningSet->SetId);
			}
		}
		for (const auto& Pair : SetCounts)
		{
			if (const FROHSetDef* Set = Database->FindSet(Pair.Key))
			{
				MakeText(EquipColumn, FString::Printf(TEXT("세트: %s %d/%d"),
					*Set->DisplayName.ToString(), Pair.Value, Set->Pieces.Num()),
					FLinearColor(0.3f, 0.85f, 0.3f));
			}
		}
	}

	// --- 우: 인벤토리 (클릭 = 장착/물약 사용) ---
	UVerticalBox* ItemColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	Columns->AddChild(ItemColumn);
	MakeText(ItemColumn, TEXT("--- 인벤토리 (클릭=장착/사용) ---"), FLinearColor(1.f, 0.9f, 0.6f));

	const TArray<FROHItemInstance>& Items = Inventory->GetItems();
	for (int32 ItemIndex = 0; ItemIndex < Items.Num(); ++ItemIndex)
	{
		const FROHItemBaseDef* Base = Database->FindBase(Items[ItemIndex].BaseId);
		const bool bClickable = Base
			&& ((Base->Kind == EROHItemKind::Equipment && Base->Slot != EROHEquipSlot::None)
				|| Base->Kind == EROHItemKind::Potion);
		const FString Label = FString::Printf(TEXT("%d: %s%s"), ItemIndex,
			*Database->GetItemDisplayName(Items[ItemIndex]).ToString(),
			*SocketSummary(*Database, Items[ItemIndex]));
		// 룬/재료는 표시만 (콘솔 ROHSocket/ROHTransmute — UI 2차)
		MakeActionButton(ItemColumn, Label, TEXT("Item"), ItemIndex,
			QualityLinearColor(Items[ItemIndex].Quality), bClickable);
	}
	if (Items.Num() == 0)
	{
		MakeText(ItemColumn, TEXT("(비어 있음)"), FLinearColor(0.45f, 0.45f, 0.45f));
	}

	MakeText(ContentBox, FString::Printf(TEXT("골드: %d"), Inventory->GetGold()), FLinearColor(1.f, 0.85f, 0.2f));
}

void UROHInventoryWindow::OnAction(FName InActionId, int32 InActionIndex)
{
	AROHPlayerCharacter* Player = GetPlayerCharacter();
	UROHInventoryComponent* Inventory = Player ? Player->GetInventory() : nullptr;
	UROHItemDatabase* Database = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UROHItemDatabase>() : nullptr;

	if (InActionId == TEXT("Unequip") && Inventory)
	{
		Inventory->UnequipSlot(static_cast<EROHEquipSlot>(InActionIndex));
		RefreshContents();
		return;
	}
	if (InActionId == TEXT("Item") && Inventory && Database)
	{
		const TArray<FROHItemInstance>& Items = Inventory->GetItems();
		const FROHItemBaseDef* Base = Items.IsValidIndex(InActionIndex)
			? Database->FindBase(Items[InActionIndex].BaseId) : nullptr;
		if (Base && Base->Kind == EROHItemKind::Equipment)
		{
			Inventory->EquipItemByIndex(InActionIndex);
		}
		else if (Base && Base->Kind == EROHItemKind::Potion)
		{
			Inventory->UsePotionAt(InActionIndex);
		}
		RefreshContents();
		return;
	}
	Super::OnAction(InActionId, InActionIndex);
}
