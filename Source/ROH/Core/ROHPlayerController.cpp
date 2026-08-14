#include "Core/ROHPlayerController.h"
#include "Core/ROHCheatManager.h"
#include "Character/ROHPlayerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "GameFramework/Pawn.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "ROH.h"

AROHPlayerController::AROHPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	CheatClass = UROHCheatManager::StaticClass();
}

void AROHPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BuildRuntimeInput();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		// 다른 경로(BP 등)로 추가된 매핑 제거 후 코드 정의 매핑만 적용
		Subsystem->ClearAllMappings();
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}

void AROHPlayerController::BuildRuntimeInput()
{
	// 코드가 키 배치의 단일 소스 — BP/애셋 지정값은 덮어쓴다
	if (bRuntimeInputBuilt)
	{
		return;
	}
	bRuntimeInputBuilt = true;

	UInputMappingContext* RuntimeIMC = NewObject<UInputMappingContext>(this, TEXT("IMC_RuntimeDefault"));

	auto MakeAction = [this, RuntimeIMC](const TCHAR* Name, const FKey& Key) -> UInputAction*
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = EInputActionValueType::Boolean;
		RuntimeIMC->MapKey(Action, Key);
		return Action;
	};

	SetDestinationAction = MakeAction(TEXT("IA_SetDestination_Runtime"), EKeys::LeftMouseButton);
	BasicAttackAction = MakeAction(TEXT("IA_BasicAttack_Runtime"), EKeys::RightMouseButton);
	Skill1Action = MakeAction(TEXT("IA_Skill1_Runtime"), EKeys::One);
	Skill2Action = MakeAction(TEXT("IA_Skill2_Runtime"), EKeys::Two);
	Skill3Action = MakeAction(TEXT("IA_Skill3_Runtime"), EKeys::Three);
	InteractAction = MakeAction(TEXT("IA_Interact_Runtime"), EKeys::E);
	DefaultMappingContext = RuntimeIMC;

	UE_LOG(LogROH, Log, TEXT("코드 정의 입력 적용 (좌클릭 이동 / 우클릭 공격 / 1·2·3 스킬 / E 상호작용)"));
}

void AROHPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	BuildRuntimeInput();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (SetDestinationAction)
		{
			EIC->BindAction(SetDestinationAction, ETriggerEvent::Started, this, &AROHPlayerController::OnSetDestinationStarted);
			EIC->BindAction(SetDestinationAction, ETriggerEvent::Triggered, this, &AROHPlayerController::OnSetDestinationTriggered);
			EIC->BindAction(SetDestinationAction, ETriggerEvent::Completed, this, &AROHPlayerController::OnSetDestinationReleased);
			EIC->BindAction(SetDestinationAction, ETriggerEvent::Canceled, this, &AROHPlayerController::OnSetDestinationReleased);
		}
		if (BasicAttackAction)
		{
			EIC->BindAction(BasicAttackAction, ETriggerEvent::Started, this, &AROHPlayerController::OnBasicAttack);
		}
		if (Skill1Action)
		{
			EIC->BindAction(Skill1Action, ETriggerEvent::Started, this, &AROHPlayerController::OnSkill1);
		}
		if (Skill2Action)
		{
			EIC->BindAction(Skill2Action, ETriggerEvent::Started, this, &AROHPlayerController::OnSkill2);
		}
		if (Skill3Action)
		{
			EIC->BindAction(Skill3Action, ETriggerEvent::Started, this, &AROHPlayerController::OnSkill3);
		}
		if (InteractAction)
		{
			EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &AROHPlayerController::OnInteract);
		}
	}
}

void AROHPlayerController::OnInteract()
{
	if (AROHPlayerCharacter* PlayerCharacter = Cast<AROHPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->Interact();
	}
}

void AROHPlayerController::OnBasicAttack()
{
	ActivateSlot(0);
}

void AROHPlayerController::OnSkill1()
{
	ActivateSlot(1);
}

void AROHPlayerController::OnSkill2()
{
	ActivateSlot(2);
}

void AROHPlayerController::OnSkill3()
{
	ActivateSlot(3);
}

void AROHPlayerController::ActivateSlot(int32 SlotIndex)
{
	if (AROHPlayerCharacter* PlayerCharacter = Cast<AROHPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->ActivateAbilityBySlot(SlotIndex);
	}
}

void AROHPlayerController::OnSetDestinationStarted()
{
	StopMovement();
	FollowTime = 0.f;
}

void AROHPlayerController::OnSetDestinationTriggered()
{
	FollowTime += GetWorld()->GetDeltaSeconds();

	FHitResult Hit;
	if (GetHitResultUnderCursor(ECC_Visibility, true, Hit))
	{
		CachedDestination = Hit.Location;
	}

	// 홀드 중에는 커서 방향으로 직접 이동
	if (APawn* ControlledPawn = GetPawn())
	{
		const FVector WorldDirection = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal2D();
		ControlledPawn->AddMovementInput(WorldDirection, 1.f, false);
	}
}

void AROHPlayerController::OnSetDestinationReleased()
{
	// 짧은 클릭이면 내비게이션으로 목적지까지 이동
	if (FollowTime <= ShortPressThreshold)
	{
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, CachedDestination);
	}
	FollowTime = 0.f;
}
