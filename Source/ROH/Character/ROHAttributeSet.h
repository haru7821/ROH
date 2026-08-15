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
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
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

	/** 아이템/패시브 고정 생명력 재생 (초당 — docs/10 §2.3, VIT×0.05 항은 Tick 공식이 담당) */
	UPROPERTY(BlueprintReadOnly, Category = "Vital")
	FGameplayAttributeData HealthRegen;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, HealthRegen)

	/** 아이템/패시브 고정 마나 재생 (초당 — docs/10 §2.3, INT×0.1 항은 Tick 공식이 담당) */
	UPROPERTY(BlueprintReadOnly, Category = "Vital")
	FGameplayAttributeData ManaRegen;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, ManaRegen)

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

	/** 치명타 확률 고정 보너스 %. 기본 5% + DEX/50 항은 파이프라인 공식이 담당 (docs/10 §3.2) */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData CritChance;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, CritChance)

	/** 치명타 피해 총 % (150 = 1.5배 — docs/10 §3.2) */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData CritDamage;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, CritDamage)

	/** 룬 위력 %: 최종 피해 곱연산 RuneMultiplier = 1 + RunePower/100 (docs/10 §5.2) */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData RunePower;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, RunePower)

	/** 공격 속도 보너스 % — 아이템/패시브 고정치. DEX/100 항은 쿨다운 공식이 담당 (docs/10 §3.4) */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData AttackSpeedPct;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, AttackSpeedPct)

	/** 시전 속도 보너스 % — 아이템/패시브 고정치. INT/200 항은 쿨다운 공식이 담당 (docs/10 §3.4) */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData CastSpeedPct;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, CastSpeedPct)

	/** 물리 피해 감소 FlatPDR% 역할 (docs/10 §4.3). 원소 저항과 별개 축, 캡은 파이프라인에서 90% */
	UPROPERTY(BlueprintReadOnly, Category = "Resistance")
	FGameplayAttributeData PhysicalResistance;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, PhysicalResistance)

	// 원소 저항 5종: 퍼센트 감쇄 (docs/10 §4.2 — 캡 75, 악몽/지옥 페널티로 음수 가능)
	UPROPERTY(BlueprintReadOnly, Category = "Resistance")
	FGameplayAttributeData FireResistance;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, FireResistance)

	UPROPERTY(BlueprintReadOnly, Category = "Resistance")
	FGameplayAttributeData ColdResistance;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, ColdResistance)

	UPROPERTY(BlueprintReadOnly, Category = "Resistance")
	FGameplayAttributeData LightningResistance;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, LightningResistance)

	UPROPERTY(BlueprintReadOnly, Category = "Resistance")
	FGameplayAttributeData PoisonResistance;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, PoisonResistance)

	UPROPERTY(BlueprintReadOnly, Category = "Resistance")
	FGameplayAttributeData ShadowResistance;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, ShadowResistance)

	// --- 이동 ---
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, MoveSpeed)

	// --- 메타 (저장/표시되지 않는 계산용) ---
	UPROPERTY(BlueprintReadOnly, Category = "Meta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UROHAttributeSet, IncomingDamage)
};
