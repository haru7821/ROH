#include "Core/ROHPlayerController.h"
#include "Core/ROHCheatManager.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "GameFramework/Pawn.h"
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

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
		else
		{
			UE_LOG(LogROH, Warning,
				TEXT("DefaultMappingContext가 비어 있습니다. BP_ROHPlayerController에서 IMC를 지정하세요. (docs/05)"));
		}
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}

void AROHPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (SetDestinationAction)
		{
			EIC->BindAction(SetDestinationAction, ETriggerEvent::Started, this, &AROHPlayerController::OnSetDestinationStarted);
			EIC->BindAction(SetDestinationAction, ETriggerEvent::Triggered, this, &AROHPlayerController::OnSetDestinationTriggered);
			EIC->BindAction(SetDestinationAction, ETriggerEvent::Completed, this, &AROHPlayerController::OnSetDestinationReleased);
			EIC->BindAction(SetDestinationAction, ETriggerEvent::Canceled, this, &AROHPlayerController::OnSetDestinationReleased);
		}
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
