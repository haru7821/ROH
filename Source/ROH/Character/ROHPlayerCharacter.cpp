#include "Character/ROHPlayerCharacter.h"
#include "Character/ROHAttributeSet.h"
#include "Items/ROHInventoryComponent.h"
#include "Loot/ROHItemPickup.h"
#include "Abilities/ROHAbilitySystemComponent.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Progression/ROHProgressionComponent.h"
#include "Progression/ROHSkillTreeComponent.h"
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
	// 스탯/어빌리티 프리셋은 서브클래스(전사/원소술사)가 정의 — ROHPlayerClasses 참고

	Inventory = CreateDefaultSubobject<UROHInventoryComponent>(TEXT("Inventory"));
	Progression = CreateDefaultSubobject<UROHProgressionComponent>(TEXT("Progression"));
	SkillTree = CreateDefaultSubobject<UROHSkillTreeComponent>(TEXT("SkillTree"));

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

void AROHPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 성장 상태 표시 (좌상단 두 번째 줄)
	if (GEngine && IsPlayerControlled() && Progression)
	{
		const FString XPText = Progression->GetLevel() >= UROHProgressionComponent::MaxLevel
			? TEXT("MAX")
			: FString::Printf(TEXT("%d/%d"), Progression->GetXP(), UROHProgressionComponent::XPForNextLevel(Progression->GetLevel()));
		GEngine->AddOnScreenDebugMessage(5, 0.5f, FColor::White,
			FString::Printf(TEXT("Lv %d | XP %s | 스탯P %d | 스킬P %d | 골드 %d"),
				Progression->GetLevel(), *XPText,
				Progression->GetStatPoints(), Progression->GetSkillPoints(),
				Inventory ? Inventory->GetGold() : 0));
	}
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
