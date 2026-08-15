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
			&& !Equipped.Contains(Base->Slot))
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

bool UROHInventoryComponent::UseFirstPotion()
{
	UROHItemDatabase* Database = GetDatabase();
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!Database || !ASC)
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

	for (int32 i = 0; i < Items.Num(); ++i)
	{
		const FROHItemBaseDef* Base = Database->FindBase(Items[i].BaseId);
		if (Base && Base->Kind == EROHItemKind::Potion && Base->PotionHealAmount > 0.f)
		{
			ASC->ApplyModToAttribute(UROHAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, Base->PotionHealAmount);
			Items.RemoveAt(i);
			return true;
		}
	}
	return false;
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
