#include "Items/ROHInventoryComponent.h"
#include "Items/ROHItemDatabase.h"
#include "Character/ROHAttributeSet.h"
#include "Character/ROHCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
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
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		const FROHItemBaseDef* Base = Database->FindBase(Items[i].BaseId);
		if (Base && Base->Kind == EROHItemKind::Equipment && Base->Slot != EROHEquipSlot::None)
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
