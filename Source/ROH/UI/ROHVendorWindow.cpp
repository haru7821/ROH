#include "UI/ROHVendorWindow.h"
#include "World/ROHTownNpc.h"
#include "Character/ROHPlayerCharacter.h"
#include "Items/ROHInventoryComponent.h"
#include "Items/ROHItemDatabase.h"
#include "Progression/ROHProgressionComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/GameInstance.h" // GetSubsystem<T>() 템플릿 인스턴스화에 완전한 타입 필요

namespace
{
	// 로사 고정 재고 — 구매 버튼과 처리부가 같은 배열을 봐야 한다 (중복 정의 금지)
	static const FName GeneralStock[] = { TEXT("ShortSword"), TEXT("Buckler"), TEXT("Cap"), TEXT("LeatherArmor"), TEXT("LeatherBoots") };

	FLinearColor VendorQualityColor(EROHItemQuality Quality)
	{
		return FLinearColor::FromSRGBColor(UROHItemDatabase::GetQualityColor(Quality));
	}

	// 구매 목록(베이스만 있는 재고) 툴팁용 미리보기 인스턴스 — 접사/소켓 굴림 없음 (베이스 성능/가치만)
	FROHItemInstance MakeVendorPreviewItem(FName BaseId)
	{
		FROHItemInstance Preview;
		Preview.BaseId = BaseId;
		return Preview;
	}
}

void UROHVendorWindow::SetNpc(AROHTownNpc* InNpc)
{
	Npc = InNpc;
}

void UROHVendorWindow::ShowStatus(const FString& Message)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Message));
	}
}

bool UROHVendorWindow::TryPurchase(const FROHItemInstance& Item, int32 Price, FString& OutStatus)
{
	AROHPlayerCharacter* Player = GetPlayerCharacter();
	UROHInventoryComponent* Inventory = Player ? Player->GetInventory() : nullptr;
	const UROHItemDatabase* Database = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UROHItemDatabase>() : nullptr;
	if (!Inventory || !Database || !Item.IsValid())
	{
		OutStatus = TEXT("구매 실패");
		return false;
	}
	// 공간 → 골드 순 (SpendGold 후 지급 실패로 골드만 사라지는 일 방지)
	if (Inventory->IsFull())
	{
		OutStatus = TEXT("인벤토리가 가득 찼습니다");
		return false;
	}
	if (!Inventory->SpendGold(Price))
	{
		OutStatus = FString::Printf(TEXT("골드 부족 (%d골드 필요, 보유 %d)"), Price, Inventory->GetGold());
		return false;
	}
	Inventory->AddItem(Item);
	OutStatus = FString::Printf(TEXT("구매: %s (-%d골드)"), *Database->GetItemDisplayName(Item).ToString(), Price);
	return true;
}

void UROHVendorWindow::RefreshContents()
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
	if (!Npc.IsValid() || !Inventory || !Database)
	{
		MakeText(ContentBox, TEXT("상인을 찾을 수 없습니다"), FLinearColor(0.6f, 0.6f, 0.6f));
		return;
	}

	// 타이틀은 SetNpc가 프레임 구성(NativeOnInitialized) 이후라 여기서 갱신
	if (TitleText)
	{
		TitleText->SetText(FText::FromString(FString::Printf(TEXT("%s — %s"),
			*Npc->GetDisplayName().ToString(), *Npc->GetRoleLabel().ToString())));
	}

	MakeText(ContentBox, FString::Printf(TEXT("\"%s\""), *Npc->GetGreeting().ToString()), FLinearColor(0.7f, 0.7f, 0.75f));
	StatusText = MakeText(ContentBox, FString(), FLinearColor(1.f, 0.6f, 0.3f));
	MakeText(ContentBox, FString::Printf(TEXT("보유 골드: %d"), Inventory->GetGold()), FLinearColor(1.f, 0.85f, 0.2f));

	const TArray<FROHItemInstance>& Items = Inventory->GetItems();
	const FLinearColor HeaderColor(1.f, 0.9f, 0.6f);

	switch (Npc->GetNpcRole())
	{
	case EROHNpcRole::General:
	{
		// 구매: 일반 등급 기본 장비 5종 고정 재고 (가격 = GoldValue)
		MakeText(ContentBox, TEXT("--- 구매 (일반 장비) ---"), HeaderColor);
		for (int32 StockIndex = 0; StockIndex < UE_ARRAY_COUNT(GeneralStock); ++StockIndex)
		{
			if (const FROHItemBaseDef* Base = Database->FindBase(GeneralStock[StockIndex]))
			{
				UROHActionButton* Button = MakeActionButton(ContentBox, FString::Printf(TEXT("%s — %d골드"),
					*Base->DisplayName.ToString(), Base->GoldValue), TEXT("BuyBase"), StockIndex, FLinearColor::White);
				Button->SetToolTipText(Database->GetItemTooltip(MakeVendorPreviewItem(Base->BaseId))); // 호버 상세 (UI 2차)
			}
		}
		// 판매: 전 품목 (미감정 포함 — docs/12), 판매가 = GoldValue/2
		MakeText(ContentBox, TEXT("--- 판매 (가격 = 가치의 절반) ---"), HeaderColor);
		for (int32 ItemIndex = 0; ItemIndex < Items.Num(); ++ItemIndex)
		{
			const FROHItemBaseDef* Base = Database->FindBase(Items[ItemIndex].BaseId);
			const int32 SellPrice = Base ? Base->GoldValue / 2 : 0;
			UROHActionButton* Button = MakeActionButton(ContentBox, FString::Printf(TEXT("%s — %d골드"),
				*Database->GetItemDisplayName(Items[ItemIndex]).ToString(), SellPrice),
				TEXT("Sell"), ItemIndex, VendorQualityColor(Items[ItemIndex].Quality));
			Button->SetToolTipText(Database->GetItemTooltip(Items[ItemIndex]));
		}
		break;
	}

	case EROHNpcRole::Blacksmith:
	{
		// 재고는 창을 열 때 1회 생성 (닫았다 열면 갱신 — docs/12 "방문마다 갱신")
		if (!bSmithStockRolled)
		{
			bSmithStockRolled = true;
			static const FName SmithBases[] = { TEXT("ShortSword"), TEXT("BattleAxe"), TEXT("Buckler"), TEXT("RoundShield"),
				TEXT("Cap"), TEXT("FullHelm"), TEXT("LeatherArmor"), TEXT("ChainMail") };
			const int32 SmithIlvl = Player->GetProgression() ? Player->GetProgression()->GetLevel() : 1;
			for (int32 StockIndex = 0; StockIndex < 6; ++StockIndex)
			{
				// 상점 생성품은 감정 완료 상태 (docs/12 — 미감정은 드랍 전용)
				const FName SmithBase = SmithBases[FMath::RandRange(0, UE_ARRAY_COUNT(SmithBases) - 1)];
				SmithStock.Add(Database->GenerateItem(SmithBase, SmithIlvl, EROHItemQuality::Magic));
			}
		}
		MakeText(ContentBox, TEXT("--- 구매 (마법 장비, 방문마다 갱신) ---"), HeaderColor);
		for (int32 StockIndex = 0; StockIndex < SmithStock.Num(); ++StockIndex)
		{
			const FROHItemBaseDef* Base = Database->FindBase(SmithStock[StockIndex].BaseId);
			const int32 Price = Base ? Base->GoldValue * 3 : 0;
			UROHActionButton* Button = MakeActionButton(ContentBox, FString::Printf(TEXT("%s — %d골드"),
				*Database->GetItemDisplayName(SmithStock[StockIndex]).ToString(), Price),
				TEXT("BuySmith"), StockIndex, VendorQualityColor(SmithStock[StockIndex].Quality));
			Button->SetToolTipText(Database->GetItemTooltip(SmithStock[StockIndex])); // 호버 상세 (UI 2차)
		}
		if (SmithStock.Num() == 0)
		{
			MakeText(ContentBox, TEXT("(재고 소진 — 다시 방문하세요)"), FLinearColor(0.45f, 0.45f, 0.45f));
		}
		break;
	}

	case EROHNpcRole::Jeweler:
	{
		// 구매: 하급 룬 티어 1~4 (가격 = 티어 × 100)
		MakeText(ContentBox, TEXT("--- 구매 (하급 룬) ---"), HeaderColor);
		for (int32 Tier = 1; Tier <= 4; ++Tier)
		{
			if (const FROHRuneDef* Rune = Database->FindRuneByTier(Tier))
			{
				UROHActionButton* Button = MakeActionButton(ContentBox, FString::Printf(TEXT("%s 룬 (T%d) — %d골드"),
					*Rune->DisplayName.ToString(), Tier, Tier * 100), TEXT("BuyRune"), Tier, FLinearColor::White);
				Button->SetToolTipText(Database->GetItemTooltip(
					MakeVendorPreviewItem(UROHItemDatabase::GetRuneBaseId(Rune->RuneId)))); // 호버 상세 (UI 2차)
			}
		}
		// 판매: 룬/재료(보석·조각)만
		MakeText(ContentBox, TEXT("--- 판매 (룬/보석/조각) ---"), HeaderColor);
		for (int32 ItemIndex = 0; ItemIndex < Items.Num(); ++ItemIndex)
		{
			const FROHItemBaseDef* Base = Database->FindBase(Items[ItemIndex].BaseId);
			if (!Base || (Base->Kind != EROHItemKind::Rune && Base->Kind != EROHItemKind::Material))
			{
				continue;
			}
			UROHActionButton* Button = MakeActionButton(ContentBox, FString::Printf(TEXT("%s — %d골드"),
				*Database->GetItemDisplayName(Items[ItemIndex]).ToString(), Base->GoldValue / 2),
				TEXT("Sell"), ItemIndex, FLinearColor::White);
			Button->SetToolTipText(Database->GetItemTooltip(Items[ItemIndex]));
		}
		break;
	}

	case EROHNpcRole::PotionVendor:
	{
		MakeText(ContentBox, TEXT("--- 구매 (치유물약) ---"), HeaderColor);
		if (const FROHItemBaseDef* Potion = Database->FindBase(TEXT("HealthPotion")))
		{
			const FText PotionTooltip = Database->GetItemTooltip(MakeVendorPreviewItem(Potion->BaseId));
			UROHActionButton* SingleButton = MakeActionButton(ContentBox,
				FString::Printf(TEXT("치유물약 1개 — %d골드"), Potion->GoldValue),
				TEXT("BuyPotion"), 1, FLinearColor(0.4f, 1.f, 0.4f));
			SingleButton->SetToolTipText(PotionTooltip); // 호버 상세 (UI 2차)
			UROHActionButton* BundleButton = MakeActionButton(ContentBox,
				FString::Printf(TEXT("치유물약 5개 — %d골드"), Potion->GoldValue * 5),
				TEXT("BuyPotion"), 5, FLinearColor(0.4f, 1.f, 0.4f));
			BundleButton->SetToolTipText(PotionTooltip);
		}
		break;
	}

	case EROHNpcRole::Gambler:
	{
		int32 GemCount = 0;
		for (const FROHItemInstance& Item : Items)
		{
			if (Item.BaseId == TEXT("FateGem"))
			{
				++GemCount;
			}
		}
		MakeText(ContentBox, FString::Printf(TEXT("운명의 보석: %d개 (악몽/지옥 몬스터가 떨어뜨립니다)"), GemCount),
			FLinearColor(1.f, 0.85f, 0.2f));
		MakeActionButton(ContentBox, FString::Printf(TEXT("[도박한다 (보석 %d개)]"), UROHInventoryComponent::GambleGemCost),
			TEXT("Gamble"), -1, FLinearColor(1.f, 0.85f, 0.2f), GemCount >= UROHInventoryComponent::GambleGemCost);
		MakeText(ContentBox, TEXT("50% 레어 / 30% 유니크 / 15% 세트 / 5% 고대무기"), FLinearColor(0.6f, 0.6f, 0.6f));
		break;
	}

	case EROHNpcRole::Identifier:
	{
		const int32 UnidentifiedCount = Inventory->CountUnidentified();
		MakeText(ContentBox, FString::Printf(TEXT("미감정 아이템: %d개"), UnidentifiedCount), HeaderColor);
		MakeActionButton(ContentBox, FString::Printf(TEXT("[전부 감정 (%d개 × %d골드)]"), UnidentifiedCount, IdentifyCost),
			TEXT("IdentifyAll"), -1, FLinearColor(0.7f, 0.4f, 1.f), UnidentifiedCount > 0);
		for (int32 ItemIndex = 0; ItemIndex < Items.Num(); ++ItemIndex)
		{
			if (!Items[ItemIndex].bUnidentified)
			{
				continue;
			}
			UROHActionButton* Button = MakeActionButton(ContentBox, FString::Printf(TEXT("%s — 감정 %d골드"),
				*Database->GetItemDisplayName(Items[ItemIndex]).ToString(), IdentifyCost),
				TEXT("Identify"), ItemIndex, VendorQualityColor(Items[ItemIndex].Quality));
			Button->SetToolTipText(Database->GetItemTooltip(Items[ItemIndex])); // 미감정 — 옵션 숨김 툴팁
		}
		break;
	}

	default:
		break;
	}
}

void UROHVendorWindow::OnAction(FName InActionId, int32 InActionIndex)
{
	AROHPlayerCharacter* Player = GetPlayerCharacter();
	UROHInventoryComponent* Inventory = Player ? Player->GetInventory() : nullptr;
	UROHItemDatabase* Database = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UROHItemDatabase>() : nullptr;
	if (!Inventory || !Database)
	{
		Super::OnAction(InActionId, InActionIndex);
		return;
	}

	FString Status;

	if (InActionId == TEXT("BuyBase"))
	{
		if (InActionIndex >= 0 && InActionIndex < UE_ARRAY_COUNT(GeneralStock))
		{
			const FROHItemBaseDef* Base = Database->FindBase(GeneralStock[InActionIndex]);
			if (Base)
			{
				TryPurchase(Database->GenerateItem(Base->BaseId, 1, EROHItemQuality::Normal), Base->GoldValue, Status);
			}
		}
	}
	else if (InActionId == TEXT("BuySmith"))
	{
		if (SmithStock.IsValidIndex(InActionIndex))
		{
			const FROHItemBaseDef* Base = Database->FindBase(SmithStock[InActionIndex].BaseId);
			if (Base && TryPurchase(SmithStock[InActionIndex], Base->GoldValue * 3, Status))
			{
				SmithStock.RemoveAt(InActionIndex); // 재고 1개씩 소진
			}
		}
	}
	else if (InActionId == TEXT("BuyRune"))
	{
		if (const FROHRuneDef* Rune = Database->FindRuneByTier(InActionIndex))
		{
			TryPurchase(Database->GenerateItem(UROHItemDatabase::GetRuneBaseId(Rune->RuneId), Rune->Tier, EROHItemQuality::Normal),
				InActionIndex * 100, Status);
		}
	}
	else if (InActionId == TEXT("BuyPotion"))
	{
		const FROHItemBaseDef* Potion = Database->FindBase(TEXT("HealthPotion"));
		int32 Bought = 0;
		for (int32 i = 0; Potion && i < InActionIndex; ++i)
		{
			FString SingleStatus;
			if (!TryPurchase(Database->GenerateItem(TEXT("HealthPotion"), 1, EROHItemQuality::Normal), Potion->GoldValue, SingleStatus))
			{
				Status = SingleStatus; // 마지막 실패 사유 (골드 부족/가득참)
				break;
			}
			++Bought;
		}
		if (Bought > 0)
		{
			Status = FString::Printf(TEXT("구매: 치유물약 ×%d%s"), Bought, Status.IsEmpty() ? TEXT("") : TEXT(" (일부 실패)"));
		}
	}
	else if (InActionId == TEXT("Sell"))
	{
		const TArray<FROHItemInstance>& Items = Inventory->GetItems();
		const FROHItemBaseDef* Base = Items.IsValidIndex(InActionIndex) ? Database->FindBase(Items[InActionIndex].BaseId) : nullptr;
		if (Base)
		{
			const FString SoldName = Database->GetItemDisplayName(Items[InActionIndex]).ToString();
			const int32 SellPrice = Base->GoldValue / 2;
			Inventory->RemoveItemAt(InActionIndex);
			Inventory->AddGold(SellPrice);
			Status = FString::Printf(TEXT("판매: %s (+%d골드)"), *SoldName, SellPrice);
		}
	}
	else if (InActionId == TEXT("Gamble"))
	{
		FString GambleMessage;
		bool bJackpot = false;
		Inventory->GambleWithGems(GambleMessage, bJackpot);
		if (bJackpot && Player && Inventory->GetItems().Num() > 0)
		{
			// 잭팟: 연출 후 창 닫기 (docs/12 카론 — 결과는 방금 추가된 마지막 아이템)
			Player->PlayAncientCelebration(Database->GetItemDisplayName(Inventory->GetItems().Last()).ToString());
			RequestClose();
			return;
		}
		Status = GambleMessage;
	}
	else if (InActionId == TEXT("Identify"))
	{
		if (Inventory->GetItems().IsValidIndex(InActionIndex) && Inventory->GetItems()[InActionIndex].bUnidentified)
		{
			if (Inventory->SpendGold(IdentifyCost))
			{
				Inventory->IdentifyItemAt(InActionIndex);
				Status = FString::Printf(TEXT("감정: %s"), *Database->GetItemDisplayName(Inventory->GetItems()[InActionIndex]).ToString());
			}
			else
			{
				Status = FString::Printf(TEXT("골드 부족 (%d골드 필요)"), IdentifyCost);
			}
		}
	}
	else if (InActionId == TEXT("IdentifyAll"))
	{
		const int32 UnidentifiedCount = Inventory->CountUnidentified();
		const int32 TotalCost = UnidentifiedCount * IdentifyCost;
		if (UnidentifiedCount <= 0)
		{
			Status = TEXT("미감정 아이템이 없습니다");
		}
		else if (Inventory->SpendGold(TotalCost))
		{
			Status = FString::Printf(TEXT("전부 감정: %d개 (-%d골드)"), Inventory->IdentifyAll(), TotalCost);
		}
		else
		{
			Status = FString::Printf(TEXT("골드 부족 (%d골드 필요)"), TotalCost);
		}
	}
	else
	{
		Super::OnAction(InActionId, InActionIndex);
		return;
	}

	RefreshContents();
	ShowStatus(Status);
}
