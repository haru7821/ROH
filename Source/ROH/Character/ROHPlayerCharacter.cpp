#include "Character/ROHPlayerCharacter.h"
#include "Character/ROHAttributeSet.h"
#include "Items/ROHInventoryComponent.h"
#include "Loot/ROHItemPickup.h"
#include "Abilities/ROHAbilitySystemComponent.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Abilities/Warrior/ROHAbility_BasicAttack.h"
#include "Abilities/Warrior/ROHAbility_Bash.h"
#include "Abilities/Warrior/ROHAbility_Whirlwind.h"
#include "Abilities/Warrior/ROHAbility_LeapAttack.h"
#include "Core/ROHGameMode.h"
#include "ROHGameplayTags.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

AROHPlayerCharacter::AROHPlayerCharacter()
{
	TeamId = 0;

	// 전사 프리셋 (docs/02 §1.1) — M3에서 클래스 선택으로 분리
	BaseStrength = 30.f;
	BaseDexterity = 20.f;
	BaseVitality = 25.f;
	BaseEnergy = 10.f;
	BaseMaxHealth = 50.f; // 최종 생명력 = 50 + 활력 25×4 = 150

	DefaultAbilities.Add(UROHAbility_BasicAttack::StaticClass());
	DefaultAbilities.Add(UROHAbility_Bash::StaticClass());
	DefaultAbilities.Add(UROHAbility_Whirlwind::StaticClass());
	DefaultAbilities.Add(UROHAbility_LeapAttack::StaticClass());

	Inventory = CreateDefaultSubobject<UROHInventoryComponent>(TEXT("Inventory"));

	// 이동 방향으로 캐릭터 회전 (쿼터뷰 표준)
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, 640.f, 0.f);
	Movement->bConstrainToPlane = true;
	Movement->bSnapToPlaneAtStart = true;
	Movement->MaxWalkSpeed = 600.f;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->SetUsingAbsoluteRotation(true);
	SpringArm->SetRelativeRotation(FRotator(-55.f, 0.f, 0.f));
	SpringArm->TargetArmLength = 1400.f;
	SpringArm->bDoCollisionTest = false;
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = false;
	SpringArm->bInheritRoll = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
}

void AROHPlayerCharacter::ActivateAbilityBySlot(int32 SlotIndex)
{
	if (!AbilitySystemComponent || !DefaultAbilities.IsValidIndex(SlotIndex) || !DefaultAbilities[SlotIndex])
	{
		return;
	}
	AbilitySystemComponent->TryActivateAbilityByClass(DefaultAbilities[SlotIndex]);
}

void AROHPlayerCharacter::Interact()
{
	auto ShowMessage = [](const FString& Text)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(2, 3.f, FColor::Cyan, Text);
		}
	};

	// 1) 근처 지면 드랍 습득 (인벤토리 가득 등으로 바닥에 남은 것)
	AROHItemPickup* Nearest = nullptr;
	float BestDistSq = FMath::Square(250.f);
	for (TActorIterator<AROHItemPickup> It(GetWorld()); It; ++It)
	{
		const float DistSq = FVector::DistSquared(GetActorLocation(), It->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Nearest = *It;
		}
	}
	if (Nearest && Nearest->TryGive(this))
	{
		ShowMessage(TEXT("아이템 습득"));
		return;
	}

	// 2) 인벤토리의 첫 장비 장착
	if (Inventory && Inventory->EquipFirstEquippable())
	{
		ShowMessage(TEXT("장비 장착 완료 (콘솔 ROHDumpAttrs로 스탯 확인)"));
		return;
	}

	ShowMessage(TEXT("상호작용 대상 없음 (장착할 장비/주울 아이템 없음)"));
}

void AROHPlayerCharacter::HandleDeath(AActor* Killer)
{
	if (!IsAlive())
	{
		return;
	}
	Super::HandleDeath(Killer);

	if (AROHGameMode* GameMode = GetWorld()->GetAuthGameMode<AROHGameMode>())
	{
		GameMode->SchedulePlayerRespawn(this);
	}
}

void AROHPlayerCharacter::Revive(const FVector& Location)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(ROHGameplayTags::State_Dead);
	}
	if (AttributeSet)
	{
		AttributeSet->SetHealth(AttributeSet->GetMaxHealth());
		AttributeSet->SetMana(AttributeSet->GetMaxMana());
		AttributeSet->SetRage(0.f);
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	if (VisualMesh)
	{
		VisualMesh->SetRelativeRotation(FRotator::ZeroRotator);
	}
	SetActorLocation(Location);
}
