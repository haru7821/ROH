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
 * 공통 스탯 세트 (docs/02 §1.3, 피해 파이프라인은 docs/03 §3.1)
 * - 1차 스탯: 힘/민첩/활력/에너지
 * - 자원: 생명력/마나(원소술사)/분노(전사)
 * - 전투: 명중(AttackRating)/방어(Defense)/저항 4종
 * - IncomingDamage는 메타 어트리뷰트: 피해 GE가 여기에 쓰면
 *   PostGameplayEffectExecute에서 Health로 반영 후 0으로 리셋된다.
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

	UPROPERTY(BlueprintReadOnly, Category = "Vital")
	FGameplayAttributeData Rage;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, Rage)

	UPROPERTY(BlueprintReadOnly, Category = "Vital")
	FGameplayAttributeData MaxRage;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, MaxRage)

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

	// --- 전투 ---
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData CharacterLevel;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, CharacterLevel)

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData AttackRating;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, AttackRating)

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData Defense;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, Defense)

	/** 무기 등 장비에서 오는 평 공격력 가산치 (스킬 피해 공식에 합산) */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData AttackPower;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, AttackPower)

	/** 매직 파인드 %: 드랍 등급 판정 보너스 (체감 곡선 적용) */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData MagicFind;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, MagicFind)

	// 저항: 퍼센트(0~75 캡, 악몽/지옥 페널티로 음수 가능)
	UPROPERTY(BlueprintReadOnly, Category = "Resistance")
	FGameplayAttributeData PhysicalResistance;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, PhysicalResistance)

	UPROPERTY(BlueprintReadOnly, Category = "Resistance")
	FGameplayAttributeData FireResistance;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, FireResistance)

	UPROPERTY(BlueprintReadOnly, Category = "Resistance")
	FGameplayAttributeData ColdResistance;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, ColdResistance)

	UPROPERTY(BlueprintReadOnly, Category = "Resistance")
	FGameplayAttributeData LightningResistance;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, LightningResistance)

	// --- 이동 ---
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, MoveSpeed)

	// --- 메타 (저장/표시되지 않는 계산용) ---
	UPROPERTY(BlueprintReadOnly, Category = "Meta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, IncomingDamage)
};
