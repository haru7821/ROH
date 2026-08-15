#include "Items/ROHInventoryComponent.h"
#include "Items/ROHItemDatabase.h"
#include "Character/ROHAttributeSet.h"
#include "Character/ROHCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Engine/Engine.h" // GEngine 화면 메시지 (룬워드 완성 알림)
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "ROH.h"

UROHInventoryComponent::UROHInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UAbilitySystemComponent* UROHInventoryComponent::GetOwnerASC() const
{
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		return ASI->GetAbilitySystemComponent();
	}
	return nullptr;
}

UROHItemDatabase* UROHInventoryComponent::GetDatabase() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const UGameInstance* GameInstance = World->GetGameInstance())
		{
			return GameInstance->GetSubsystem<UROHItemDatabase>();
		}
	}
	return nullptr;
}

bool UROHInventoryComponent::AddItem(const FROHItemInstance& Item)
{
	if (!Item.IsValid() || IsFull())
	{
		return false;
	}
	Items.Add(Item);
	return true;
}

bool UROHInventoryComponent::RemoveItemAt(int32 ItemIndex)
{
	if (!Items.IsValidIndex(ItemIndex))
	{
		return false;
	}
	Items.RemoveAt(ItemIndex);
	return true;
}

bool UROHInventoryComponent::EquipItemByIndex(int32 ItemIndex)
{
	if (!Items.IsValidIndex(ItemIndex))
	{
		return false;
	}

	UROHItemDatabase* Database = GetDatabase();
	const FROHItemBaseDef* Base = Database ? Database->FindBase(Items[ItemIndex].BaseId) : nullptr;
	if (!Base || Base->Kind != EROHItemKind::Equipment || Base->Slot == EROHEquipSlot::None)
	{
		return false;
	}

	// 미감정 장착 불가 (docs/12) — 셀바 또는 ROHIdentify로 감정
	if (Items[ItemIndex].bUnidentified)
	{
		return false;
	}

	const FROHItemInstance ItemToEquip = Items[ItemIndex];
	Items.RemoveAt(ItemIndex);

	// 기존 장비는 인벤토리로 복귀
	if (const FROHItemInstance* Existing = Equipped.Find(Base->Slot))
	{
		FROHItemInstance Removed = *Existing;
		RemoveEquipEffect(Base->Slot);
		Equipped.Remove(Base->Slot);
		Items.Add(Removed);
	}

	Equipped.Add(Base->Slot, ItemToEquip);
	ApplyEquipEffect(Base->Slot, ItemToEquip);
	RefreshSetBonuses(); // 장착 조합 변경 (M5 2차 세트)
	return true;
}

bool UROHInventoryComponent::EquipFirstEquippable()
{
	const UROHItemDatabase* Database = GetDatabase();
	if (!Database)
	{
		return false;
	}
	// 비어 있는 슬롯에 들어갈 장비만 대상 (기존 장비와의 무한 맞교환 방지 —
	// 교체는 콘솔 ROHEquip / 추후 인벤토리 UI에서)
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		const FROHItemBaseDef* Base = Database->FindBase(Items[i].BaseId);
		if (Base && Base->Kind == EROHItemKind::Equipment && Base->Slot != EROHEquipSlot::None
			&& !Equipped.Contains(Base->Slot) && !Items[i].bUnidentified) // 미감정 스킵 (docs/12)
		{
			return EquipItemByIndex(i);
		}
	}
	return false;
}

bool UROHInventoryComponent::UnequipSlot(EROHEquipSlot Slot)
{
	const FROHItemInstance* Existing = Equipped.Find(Slot);
	if (!Existing || IsFull())
	{
		return false;
	}
	FROHItemInstance Removed = *Existing;
	RemoveEquipEffect(Slot);
	Equipped.Remove(Slot);
	Items.Add(Removed);
	RefreshSetBonuses(); // 장착 조합 변경 (M5 2차 세트)
	return true;
}

void UROHInventoryComponent::ApplyEquipEffect(EROHEquipSlot Slot, const FROHItemInstance& Item)
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	UROHItemDatabase* Database = GetDatabase();
	const FROHItemBaseDef* Base = Database ? Database->FindBase(Item.BaseId) : nullptr;
	if (!ASC || !Base)
	{
		return;
	}

	// 런타임 GE 구성: 베이스 성능 + 접사 전부 Additive 모디파이어로
	// (이름은 자동 유니크 — 고정 이름은 동명 객체를 in-place 교체해 활성 GE를 파괴할 위험)
	UGameplayEffect* EquipEffect = NewObject<UGameplayEffect>(GetTransientPackage());
	EquipEffect->DurationPolicy = EGameplayEffectDurationType::Infinite;

	auto AddModifier = [EquipEffect](const FGameplayAttribute& Attribute, float Value)
	{
		if (!Attribute.IsValid() || FMath::IsNearlyZero(Value))
		{
			return;
		}
		FGameplayModifierInfo Modifier;
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Value));
		EquipEffect->Modifiers.Add(Modifier);
	};

	if (Base->DamageMax > 0.f)
	{
		AddModifier(UROHAttributeSet::GetAttackPowerAttribute(), (Base->DamageMin + Base->DamageMax) * 0.5f);
	}
	if (Base->Armor > 0.f)
	{
		AddModifier(UROHAttributeSet::GetDefenseAttribute(), Base->Armor);
	}
	for (const FROHAffixRoll& Affix : Item.Affixes)
	{
		AddModifier(Affix.Attribute, Affix.Value);
	}

	// 소켓 룬/룬워드 (M5): 세이브엔 ID만 저장 — 보너스는 항상 DB에서 해석 (로드 갱신 불필요)
	float TotalRunePower = 0.f;
	for (const FName& RuneId : Item.SocketedRunes)
	{
		if (const FROHRuneDef* Rune = Database->FindRune(RuneId))
		{
			AddModifier(Rune->BonusAttribute, Rune->BonusValue);
			TotalRunePower += Rune->RunePower;
		}
	}
	if (!Item.RunewordId.IsNone())
	{
		if (const FROHRunewordDef* Runeword = Database->FindRuneword(Item.RunewordId))
		{
			for (const FROHRunewordBonus& Bonus : Runeword->Bonuses)
			{
				AddModifier(Bonus.Attribute, Bonus.Value);
			}
			TotalRunePower += Runeword->RunePower;
		}
	}
	// 세트 피스 자체 옵션 (M5 2차) — 조합 보너스는 RefreshSetBonuses의 별도 GE가 담당
	if (!Item.SetPieceId.IsNone())
	{
		if (const FROHSetPieceDef* Piece = Database->FindSetPiece(Item.SetPieceId))
		{
			for (const FROHRunewordBonus& Bonus : Piece->Bonuses)
			{
				AddModifier(Bonus.Attribute, Bonus.Value);
			}
		}
	}

	// 유니크/고대 (M5 2차): 고정 옵션 — 고대는 옵션 ×1.5 + 룬 위력 +3 (DB 실시간 해석)
	if (!Item.UniqueId.IsNone())
	{
		if (const FROHUniqueDef* Unique = Database->FindUnique(Item.UniqueId))
		{
			const bool bAncient = (Item.Quality == EROHItemQuality::Ancient);
			const float BonusMult = bAncient ? UROHItemDatabase::AncientBonusMult : 1.f;
			for (const FROHRunewordBonus& Bonus : Unique->Bonuses)
			{
				AddModifier(Bonus.Attribute, Bonus.Value * BonusMult);
			}
			TotalRunePower += Unique->RunePower + (bAncient ? UROHItemDatabase::AncientRunePowerBonus : 0.f);
		}
	}

	// 룬 위력 → ×(1 + RunePower/100) 최종 피해 배율 합산원 (docs/10 §5.2)
	AddModifier(UROHAttributeSet::GetRunePowerAttribute(), TotalRunePower);

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);
	const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectToSelf(EquipEffect, 1.f, Context);
	EquipEffectHandles.Add(Slot, Handle);
}

void UROHInventoryComponent::RemoveEquipEffect(EROHEquipSlot Slot)
{
	if (FActiveGameplayEffectHandle* Handle = EquipEffectHandles.Find(Slot))
	{
		if (UAbilitySystemComponent* ASC = GetOwnerASC())
		{
			ASC->RemoveActiveGameplayEffect(*Handle);
		}
		EquipEffectHandles.Remove(Slot);
	}
}

void UROHInventoryComponent::RefreshSetBonuses()
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	UROHItemDatabase* Database = GetDatabase();

	// 기존 세트 보너스 전부 제거 후 현재 조합 기준으로 재구성
	for (const auto& HandlePair : SetBonusHandles)
	{
		if (ASC)
		{
			ASC->RemoveActiveGameplayEffect(HandlePair.Value);
		}
	}
	SetBonusHandles.Reset();
	if (!ASC || !Database)
	{
		return;
	}

	// 세트별 장착 피스 수 집계 (슬롯당 장비 1개라 동일 피스 중복 장착은 구조상 불가)
	TMap<FName, int32> CountsBySet;
	for (const auto& EquipPair : Equipped)
	{
		if (EquipPair.Value.SetPieceId.IsNone())
		{
			continue;
		}
		const FROHSetDef* OwningSet = nullptr;
		if (Database->FindSetPiece(EquipPair.Value.SetPieceId, &OwningSet) && OwningSet)
		{
			++CountsBySet.FindOrAdd(OwningSet->SetId);
		}
	}

	for (const auto& CountPair : CountsBySet)
	{
		const FROHSetDef* Set = Database->FindSet(CountPair.Key);
		if (!Set)
		{
			continue;
		}

		UGameplayEffect* SetEffect = NewObject<UGameplayEffect>(GetTransientPackage());
		SetEffect->DurationPolicy = EGameplayEffectDurationType::Infinite;
		auto AddModifier = [SetEffect](const FGameplayAttribute& Attribute, float Value)
		{
			if (!Attribute.IsValid() || FMath::IsNearlyZero(Value))
			{
				return;
			}
			FGameplayModifierInfo Modifier;
			Modifier.Attribute = Attribute;
			Modifier.ModifierOp = EGameplayModOp::Additive;
			Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Value));
			SetEffect->Modifiers.Add(Modifier);
		};

		// 도달한 임계 보너스 누적 (2피스 도달 시 2, 3피스 도달 시 2+3, …)
		for (const auto& ThresholdPair : Set->CountBonuses)
		{
			if (CountPair.Value >= ThresholdPair.Key)
			{
				for (const FROHRunewordBonus& Bonus : ThresholdPair.Value)
				{
					AddModifier(Bonus.Attribute, Bonus.Value);
				}
			}
		}
		// 풀세트 룬 위력 (docs/10 §5.2 합산원)
		if (CountPair.Value >= Set->Pieces.Num())
		{
			AddModifier(UROHAttributeSet::GetRunePowerAttribute(), Set->FullSetRunePower);
		}
		if (SetEffect->Modifiers.Num() == 0)
		{
			continue;
		}

		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(this);
		SetBonusHandles.Add(CountPair.Key, ASC->ApplyGameplayEffectToSelf(SetEffect, 1.f, Context));
	}
}

bool UROHInventoryComponent::UseFirstPotion()
{
	const UROHItemDatabase* Database = GetDatabase();
	if (!Database)
	{
		return false;
	}
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		const FROHItemBaseDef* Base = Database->FindBase(Items[i].BaseId);
		if (Base && Base->Kind == EROHItemKind::Potion && Base->PotionHealAmount > 0.f)
		{
			return UsePotionAt(i);
		}
	}
	return false;
}

bool UROHInventoryComponent::UsePotionAt(int32 ItemIndex)
{
	UROHItemDatabase* Database = GetDatabase();
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!Database || !ASC || !Items.IsValidIndex(ItemIndex))
	{
		return false;
	}

	// 죽은 상태에서는 사용 불가
	if (const AROHCharacterBase* OwnerCharacter = Cast<AROHCharacterBase>(GetOwner()))
	{
		if (!OwnerCharacter->IsAlive())
		{
			return false;
		}
	}

	const FROHItemBaseDef* Base = Database->FindBase(Items[ItemIndex].BaseId);
	if (!Base || Base->Kind != EROHItemKind::Potion || Base->PotionHealAmount <= 0.f)
	{
		return false;
	}
	ASC->ApplyModToAttribute(UROHAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, Base->PotionHealAmount);
	Items.RemoveAt(ItemIndex);
	return true;
}

bool UROHInventoryComponent::SocketRune(int32 ItemIndex, int32 RuneItemIndex, FString& OutError)
{
	UROHItemDatabase* Database = GetDatabase();
	if (!Database)
	{
		OutError = TEXT("아이템 데이터베이스가 없습니다.");
		return false;
	}
	if (ItemIndex == RuneItemIndex || !Items.IsValidIndex(ItemIndex) || !Items.IsValidIndex(RuneItemIndex))
	{
		OutError = TEXT("잘못된 인덱스입니다. ROHDumpInventory로 확인하세요.");
		return false;
	}

	const FROHRuneDef* Rune = Database->FindRuneByBaseId(Items[RuneItemIndex].BaseId);
	if (!Rune)
	{
		OutError = TEXT("두 번째 인덱스는 룬이어야 합니다.");
		return false;
	}

	FROHItemInstance& Target = Items[ItemIndex];
	const FROHItemBaseDef* TargetBase = Database->FindBase(Target.BaseId);
	if (!TargetBase || TargetBase->Kind != EROHItemKind::Equipment)
	{
		OutError = TEXT("첫 번째 인덱스는 장비여야 합니다. (장착 중이면 해제 후 소켓하세요)");
		return false;
	}
	if (Target.bUnidentified)
	{
		OutError = TEXT("미감정 아이템입니다 — 먼저 감정하세요 (셀바/ROHIdentify)");
		return false;
	}
	if (Target.SocketedRunes.Num() >= Target.MaxSockets)
	{
		OutError = Target.MaxSockets == 0 ? TEXT("소켓이 없는 장비입니다.") : TEXT("빈 소켓이 없습니다.");
		return false;
	}

	Target.SocketedRunes.Add(Rune->RuneId);

	// 룬워드 완성 검사 — 완성 시 등급 승격 (표시 색/명칭 자동 전환)
	if (const FROHRunewordDef* Runeword = Database->MatchRuneword(Target))
	{
		Target.RunewordId = Runeword->RunewordId;
		Target.Quality = EROHItemQuality::Runeword;
		UE_LOG(LogROH, Log, TEXT("룬워드 완성: %s (%s)"), *Runeword->DisplayName.ToString(), *Target.BaseId.ToString());
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Green,
				FString::Printf(TEXT("룬워드 완성: %s!"), *Runeword->DisplayName.ToString()));
		}
	}

	// 룬 소모는 마지막에 — RemoveAt이 Target 참조를 무효화할 수 있으므로 수정 완료 후 제거
	Items.RemoveAt(RuneItemIndex);
	return true;
}

bool UROHInventoryComponent::SalvageUnique(int32 ItemIndex, FString& OutMessage)
{
	UROHItemDatabase* Database = GetDatabase();
	if (!Database || !Items.IsValidIndex(ItemIndex))
	{
		OutMessage = TEXT("잘못된 인덱스입니다. ROHDumpInventory로 확인하세요.");
		return false;
	}
	if (Items[ItemIndex].Quality == EROHItemQuality::Ancient)
	{
		OutMessage = TEXT("고대 아이템은 분해할 수 없습니다.");
		return false;
	}
	if (Items[ItemIndex].Quality != EROHItemQuality::Unique)
	{
		OutMessage = TEXT("유니크만 분해할 수 있습니다.");
		return false;
	}
	if (Items[ItemIndex].bUnidentified)
	{
		OutMessage = TEXT("미감정 아이템입니다 — 먼저 감정하세요 (셀바/ROHIdentify)");
		return false;
	}

	// 조각 굴림을 먼저 확정하고 공간을 검사 — 파괴 후 조각이 소실되는 일 방지
	// (분해는 시드 재현이 불필요한 소비성 굴림)
	const int32 ShardCount = FMath::RandRange(2, 4);
	if (Capacity - (Items.Num() - 1) < ShardCount)
	{
		OutMessage = FString::Printf(TEXT("인벤토리 공간이 부족합니다 (조각 %d개 필요)"), ShardCount);
		return false;
	}

	const FString SalvagedName = Database->GetItemDisplayName(Items[ItemIndex]).ToString();
	Items.RemoveAt(ItemIndex);
	int32 Granted = 0;
	for (int32 i = 0; i < ShardCount; ++i)
	{
		if (AddItem(Database->GenerateItem(TEXT("SaintRelic"), 1, EROHItemQuality::Normal)))
		{
			++Granted;
		}
	}
	OutMessage = FString::Printf(TEXT("분해: %s → 성유물 조각 ×%d"), *SalvagedName, Granted);
	UE_LOG(LogROH, Log, TEXT("%s"), *OutMessage);
	return true;
}

bool UROHInventoryComponent::ForgeAncient(int32 ItemIndex, FString& OutMessage)
{
	UROHItemDatabase* Database = GetDatabase();
	if (!Database || !Items.IsValidIndex(ItemIndex))
	{
		OutMessage = TEXT("잘못된 인덱스입니다. ROHDumpInventory로 확인하세요.");
		return false;
	}
	FROHItemInstance& Target = Items[ItemIndex];
	if (Target.Quality == EROHItemQuality::Ancient)
	{
		OutMessage = TEXT("이미 고대 아이템입니다.");
		return false;
	}
	if (Target.Quality != EROHItemQuality::Unique)
	{
		OutMessage = TEXT("유니크만 고대로 벼릴 수 있습니다. (장착 중이면 해제 후 시도)");
		return false;
	}
	if (Target.bUnidentified)
	{
		OutMessage = TEXT("미감정 아이템입니다 — 먼저 감정하세요 (셀바/ROHIdentify)");
		return false;
	}

	// 성유물 조각 수집
	TArray<int32> RelicIndices;
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (i != ItemIndex && Items[i].BaseId == TEXT("SaintRelic"))
		{
			RelicIndices.Add(i);
		}
	}
	if (RelicIndices.Num() < AncientForgeCost)
	{
		OutMessage = FString::Printf(TEXT("성유물 조각이 부족합니다 (%d/%d — ROHSalvage로 유니크를 분해하세요)"),
			RelicIndices.Num(), AncientForgeCost);
		return false;
	}

	// 승격 먼저 → 표시명 확보 → 조각 소모 (RemoveAt 이후엔 Target 참조/인덱스가 무효)
	Target.Quality = EROHItemQuality::Ancient;
	OutMessage = FString::Printf(TEXT("고대 강림: %s!"), *Database->GetItemDisplayName(Target).ToString());
	for (int32 Consumed = 0; Consumed < AncientForgeCost; ++Consumed)
	{
		Items.RemoveAt(RelicIndices[RelicIndices.Num() - 1 - Consumed]); // 뒤 인덱스부터
	}
	UE_LOG(LogROH, Log, TEXT("%s"), *OutMessage);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor(220, 60, 60), OutMessage);
	}
	return true;
}

bool UROHInventoryComponent::GambleWithGems(FString& OutMessage, bool& bOutAncientJackpot)
{
	bOutAncientJackpot = false;
	UROHItemDatabase* Database = GetDatabase();
	if (!Database)
	{
		OutMessage = TEXT("아이템 데이터베이스가 없습니다.");
		return false;
	}

	// 보석 수집 — 소모 전 검증 (실패 경로에서 아이템 소실 금지, SalvageUnique와 동일 원칙)
	TArray<int32> GemIndices;
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (Items[i].BaseId == TEXT("FateGem"))
		{
			GemIndices.Add(i);
		}
	}
	if (GemIndices.Num() < GambleGemCost)
	{
		OutMessage = FString::Printf(TEXT("운명의 보석이 부족합니다 (%d/%d — 악몽/지옥 몬스터가 떨어뜨립니다)"),
			GemIndices.Num(), GambleGemCost);
		return false;
	}
	// 공간: 보석 3개를 빼고 1개를 넣으므로 항상 여유 — 별도 용량 검증 불필요

	// 결과 굴림 (비시드 — 소비성): 5% 고대 잭팟 / 30% 유니크 / 15% 세트 / 50% 레어
	const FName WeaponBase = FMath::RandBool() ? FName(TEXT("ShortSword")) : FName(TEXT("BattleAxe"));
	const int32 GambleIlvl = 12; // 상급 유니크(성 라콤/아벨로 등) 후보 포함
	const float ResultRoll = FMath::FRand();
	const bool bJackpotRoll = ResultRoll < 0.05f;

	FROHItemInstance Result;
	if (bJackpotRoll)
	{
		// 잭팟: 영웅(성인)이 사용하던 고대무기 — 유니크 무기 생성 후 Ancient 승격 (ForgeAncient와 동일 의미)
		Result = Database->GenerateItem(WeaponBase, GambleIlvl, EROHItemQuality::Unique);
		if (Result.UniqueId.IsNone())
		{
			// 방어: 후보 부재로 레어 강등된 경우 1회 재시도 (현 DB엔 무기 유니크가 항상 있어 실제 미발생)
			Result = Database->GenerateItem(WeaponBase, GambleIlvl, EROHItemQuality::Unique);
		}
		if (!Result.UniqueId.IsNone())
		{
			Result.Quality = EROHItemQuality::Ancient;
			bOutAncientJackpot = true;
		}
	}
	else if (ResultRoll < 0.35f)
	{
		Result = Database->GenerateItem(WeaponBase, GambleIlvl, EROHItemQuality::Unique);
	}
	else if (ResultRoll < 0.50f)
	{
		// 세트 피스 (무기 우선 — 현 DB엔 ilvl 12 무기 피스가 항상 있어 강등 미발생, 규칙은 자연 적용)
		Result = Database->GenerateItem(WeaponBase, GambleIlvl, EROHItemQuality::Set);
	}
	else
	{
		Result = Database->GenerateItem(WeaponBase, GambleIlvl, EROHItemQuality::Rare);
	}
	if (!Result.IsValid())
	{
		OutMessage = TEXT("도박 실패: 아이템 생성 오류 (보석은 소모되지 않음)");
		return false;
	}

	// 성공 확정 후 소모 — 뒤 인덱스부터
	for (int32 Consumed = 0; Consumed < GambleGemCost; ++Consumed)
	{
		Items.RemoveAt(GemIndices[GemIndices.Num() - 1 - Consumed]);
	}
	AddItem(Result); // 3개 빠진 자리라 항상 성공

	const FString ResultName = Database->GetItemDisplayName(Result).ToString();
	if (bOutAncientJackpot)
	{
		OutMessage = FString::Printf(TEXT("★ 고대무기 강림: %s ★"), *ResultName);
	}
	else if (bJackpotRoll)
	{
		OutMessage = FString::Printf(TEXT("도박 결과: %s (고대 강림 실패 — 레어로 대체)"), *ResultName);
	}
	else
	{
		OutMessage = FString::Printf(TEXT("도박 결과: %s"), *ResultName);
	}
	UE_LOG(LogROH, Log, TEXT("%s"), *OutMessage);
	return true;
}

bool UROHInventoryComponent::IdentifyItemAt(int32 ItemIndex)
{
	if (!Items.IsValidIndex(ItemIndex) || !Items[ItemIndex].bUnidentified)
	{
		return false;
	}
	Items[ItemIndex].bUnidentified = false;
	return true;
}

int32 UROHInventoryComponent::IdentifyAll()
{
	int32 Identified = 0;
	for (FROHItemInstance& Item : Items)
	{
		if (Item.bUnidentified)
		{
			Item.bUnidentified = false;
			++Identified;
		}
	}
	return Identified;
}

int32 UROHInventoryComponent::CountUnidentified() const
{
	int32 Count = 0;
	for (const FROHItemInstance& Item : Items)
	{
		if (Item.bUnidentified)
		{
			++Count;
		}
	}
	return Count;
}

void UROHInventoryComponent::ExportState(TArray<FROHItemInstance>& OutItems, TMap<EROHEquipSlot, FROHItemInstance>& OutEquipped, int32& OutGold) const
{
	OutItems = Items;
	OutEquipped = Equipped;
	OutGold = Gold;
}

void UROHInventoryComponent::RestoreState(const TArray<FROHItemInstance>& InItems, const TMap<EROHEquipSlot, FROHItemInstance>& InEquipped, int32 InGold)
{
	// 기존 장착 효과 제거 후 초기화
	TArray<EROHEquipSlot> Slots;
	Equipped.GetKeys(Slots);
	for (EROHEquipSlot Slot : Slots)
	{
		RemoveEquipEffect(Slot);
	}
	Equipped.Reset();
	Items = InItems;
	Gold = FMath::Max(0, InGold);

	// 장착 복원: 인벤토리에 넣었다가 정식 경로로 장착 (GE 재적용 보장)
	for (const auto& Pair : InEquipped)
	{
		Items.Add(Pair.Value);
		if (!EquipItemByIndex(Items.Num() - 1))
		{
			// 장착 불가(데이터 변경 등) 아이템이 용량 초과로 남는 것 방지
			UE_LOG(LogROH, Warning, TEXT("세이브 장착 복원 실패: %s — 아이템 제거"), *Pair.Value.BaseId.ToString());
			Items.Pop();
		}
	}
	// 복원 장비가 없어도(전부 해제 상태 세이브) 잔존 세트 보너스가 남지 않게 최종 재계산
	RefreshSetBonuses();
}

void UROHInventoryComponent::AddGold(int32 Amount)
{
	Gold = FMath::Max(0, Gold + Amount);
}

bool UROHInventoryComponent::SpendGold(int32 Amount)
{
	if (Amount < 0 || Gold < Amount)
	{
		return false;
	}
	Gold -= Amount;
	return true;
}
