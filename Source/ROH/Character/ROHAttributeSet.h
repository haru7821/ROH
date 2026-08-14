#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "ROHAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 공통 스탯 세트 (docs/02 §1.3)
 * - 1차 스탯: 힘/민첩/활력/에너지 (레벨업 수동 분배)
 * - 자원: 생명력/마나 (활력/에너지에서 파생, 파생 공식은 M1에서 GameplayEffect로 연결)
 * 모든 스탯 보정(장비/스킬/정복자/버프)은 GameplayEffect로만 가한다.
 */
UCLASS()
class ROH_API UROHAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UROHAttributeSet();

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

	// --- 자원 ---
	UPROPERTY(BlueprintReadOnly, Category = "Vital")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Vital")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category = "Vital")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, Mana)

	UPROPERTY(BlueprintReadOnly, Category = "Vital")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, MaxMana)

	// --- 1차 스탯 ---
	UPROPERTY(BlueprintReadOnly, Category = "Primary")
	FGameplayAttributeData Strength;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, Strength)

	UPROPERTY(BlueprintReadOnly, Category = "Primary")
	FGameplayAttributeData Dexterity;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, Dexterity)

	UPROPERTY(BlueprintReadOnly, Category = "Primary")
	FGameplayAttributeData Vitality;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, Vitality)

	UPROPERTY(BlueprintReadOnly, Category = "Primary")
	FGameplayAttributeData Energy;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, Energy)

	// --- 이동 ---
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, MoveSpeed)
};
