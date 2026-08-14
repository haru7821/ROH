#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "ROHCharacterBase.generated.h"

class UROHAbilitySystemComponent;
class UROHAttributeSet;

/**
 * 플레이어/몬스터 공통 베이스.
 * 싱글플레이 전제로 ASC를 캐릭터에 직접 부착한다 (Action RPG 샘플 방식).
 * 멀티 확장 시 플레이어만 PlayerState로 옮기면 되도록 InitAbilityActorInfo를 분리해 둔다.
 */
UCLASS(Abstract)
class ROH_API AROHCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AROHCharacterBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UROHAttributeSet* GetAttributeSet() const { return AttributeSet; }

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	void InitAbilityActorInfo();

	UPROPERTY(VisibleAnywhere, Category = "ROH|Abilities")
	TObjectPtr<UROHAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UROHAttributeSet> AttributeSet;
};
