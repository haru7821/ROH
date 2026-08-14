#include "Character/ROHCharacterBase.h"
#include "Abilities/ROHAbilitySystemComponent.h"
#include "Abilities/ROHGameplayAbility.h"
#include "Character/ROHAttributeSet.h"
#include "ROHGameplayTags.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

AROHCharacterBase::AROHCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = CreateDefaultSubobject<UROHAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UROHAttributeSet>(TEXT("AttributeSet"));

	// 그레이박스: 엔진 기본 캡슐 메시로 몸통 표시 (아트는 M6)
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(GetCapsuleComponent());
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CapsuleMesh(TEXT("/Engine/BasicShapes/Capsule.Capsule"));
	if (CapsuleMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CapsuleMesh.Object);
		VisualMesh->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
		VisualMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.7f));
	}
}

UAbilitySystemComponent* AROHCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

bool AROHCharacterBase::IsAlive() const
{
	return AbilitySystemComponent && !AbilitySystemComponent->HasMatchingGameplayTag(ROHGameplayTags::State_Dead);
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
	if (!AbilitySystemComponent)
	{
		return;
	}
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	InitializeAttributes();
	GrantDefaultAbilities();
}

void AROHCharacterBase::InitializeAttributes()
{
	if (!AttributeSet)
	{
		return;
	}

	AttributeSet->InitStrength(BaseStrength);
	AttributeSet->InitDexterity(BaseDexterity);
	AttributeSet->InitVitality(BaseVitality);
	AttributeSet->InitEnergy(BaseEnergy);
	AttributeSet->InitCharacterLevel(BaseLevel);

	// 파생 공식 (docs/02 §1.3) — M3에서 레벨업/장비 반영 시 GameplayEffect로 이관
	const float MaxHealth = BaseMaxHealth + BaseVitality * 4.f;
	AttributeSet->InitMaxHealth(MaxHealth);
	AttributeSet->InitHealth(MaxHealth);

	const float MaxMana = 20.f + BaseEnergy * 2.f;
	AttributeSet->InitMaxMana(MaxMana);
	AttributeSet->InitMana(MaxMana);

	AttributeSet->InitRage(0.f);
	AttributeSet->InitAttackRating(BaseDexterity * 5.f);
	AttributeSet->InitDefense(BaseDexterity * 2.f);
	AttributeSet->InitMoveSpeed(BaseMoveSpeed);

	GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed;
}

void AROHCharacterBase::GrantDefaultAbilities()
{
	if (bAbilitiesGranted)
	{
		return;
	}
	bAbilitiesGranted = true;

	for (const TSubclassOf<UROHGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		GrantAbility(AbilityClass);
	}
}

FGameplayAbilitySpecHandle AROHCharacterBase::GrantAbility(TSubclassOf<UROHGameplayAbility> AbilityClass)
{
	if (!AbilitySystemComponent || !AbilityClass)
	{
		return FGameplayAbilitySpecHandle();
	}
	return AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
}

void AROHCharacterBase::HandleDamageTaken(float Damage, AActor* InstigatorActor)
{
	// M1: 피격 연출 훅 (추후 히트 플래시/사운드/데미지 텍스트)
}

void AROHCharacterBase::HandleDeath(AActor* Killer)
{
	if (!IsAlive())
	{
		return;
	}

	AbilitySystemComponent->AddLooseGameplayTag(ROHGameplayTags::State_Dead);
	AbilitySystemComponent->CancelAllAbilities();

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 그레이박스 사망 연출: 몸통을 눕힌다
	if (VisualMesh)
	{
		VisualMesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	}

	OnDeath.Broadcast(this);
}
