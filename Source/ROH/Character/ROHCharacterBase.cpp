#include "Character/ROHCharacterBase.h"
#include "Abilities/ROHAbilitySystemComponent.h"
#include "Character/ROHAttributeSet.h"

AROHCharacterBase::AROHCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = CreateDefaultSubobject<UROHAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UROHAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* AROHCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AROHCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// AI 등 컨트롤러 빙의 전에 스폰되는 경우 대비
	if (AbilitySystemComponent && !AbilitySystemComponent->AbilityActorInfo->OwnerActor.IsValid())
	{
		InitAbilityActorInfo();
	}
}

void AROHCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitAbilityActorInfo();
}

void AROHCharacterBase::InitAbilityActorInfo()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}
