#include "Core/ROHPlayerController.h"
#include "Core/ROHCheatManager.h"
#include "Character/ROHPlayerCharacter.h"
#include "UI/ROHUiWindow.h"
#include "UI/ROHWaypointWindow.h"
#include "UI/ROHInventoryWindow.h"
#include "UI/ROHSkillTreeWindow.h"
#include "UI/ROHVendorWindow.h"
#include "UI/ROHStashWindow.h"
#include "UI/ROHParagonWindow.h"
#include "UI/ROHSkillBarWidget.h"
#include "UI/ROHMinimapWidget.h"
#include "Blueprint/UserWidget.h" // CreateWidget
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
		// TODO(UI 도입 시): 전체 삭제 대신 이 컨트롤러가 아는 컨텍스트만 교체할 것
		//                   (UI/CommonUI가 추가하는 IMC까지 지워버릴 수 있음)
		Subsystem->ClearAllMappings();
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	// 상시 HUD (b31): 하단 스킬바 + 우상단 미니맵 — 창(z=10)보다 낮은 z, HitTestInvisible
	if (!SkillBarWidget)
	{
		SkillBarWidget = CreateWidget<UROHSkillBarWidget>(this, UROHSkillBarWidget::StaticClass());
		if (SkillBarWidget)
		{
			SkillBarWidget->AddToViewport(1);
		}
	}
	if (!MinimapWidget)
	{
		MinimapWidget = CreateWidget<UROHMinimapWidget>(this, UROHMinimapWidget::StaticClass());
		if (MinimapWidget)
		{
			MinimapWidget->AddToViewport(1);
			// 우상단 고정 200×200 (뷰포트 슬롯이 위젯 지오메트리를 결정 — 위젯 내부는 배경+점 그리기만)
			MinimapWidget->SetAnchorsInViewport(FAnchors(1.f, 0.f));
			MinimapWidget->SetAlignmentInViewport(FVector2D(1.f, 0.f));
			MinimapWidget->SetPositionInViewport(FVector2D(-16.f, 16.f), false);
			MinimapWidget->SetDesiredSizeInViewport(FVector2D(200.f, 200.f));
		}
	}
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
	Skill4Action = MakeAction(TEXT("IA_Skill4_Runtime"), EKeys::Four);
	InteractAction = MakeAction(TEXT("IA_Interact_Runtime"), EKeys::E);
	InventoryAction = MakeAction(TEXT("IA_Inventory_Runtime"), EKeys::I);
	SkillTreeAction = MakeAction(TEXT("IA_SkillTree_Runtime"), EKeys::K);
	ParagonAction = MakeAction(TEXT("IA_Paragon_Runtime"), EKeys::P);
	DefaultMappingContext = RuntimeIMC;

	UE_LOG(LogROH, Log, TEXT("코드 정의 입력 적용 (좌클릭 이동 / 우클릭 공격 / 1·2·3·4 스킬 / E 상호작용 / I 인벤토리 / K 스킬트리 / P 정복자)"));
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
		if (Skill4Action)
		{
			EIC->BindAction(Skill4Action, ETriggerEvent::Started, this, &AROHPlayerController::OnSkill4);
		}
		if (InteractAction)
		{
			EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &AROHPlayerController::OnInteract);
		}
		if (InventoryAction)
		{
			EIC->BindAction(InventoryAction, ETriggerEvent::Started, this, &AROHPlayerController::OnToggleInventory);
		}
		if (SkillTreeAction)
		{
			EIC->BindAction(SkillTreeAction, ETriggerEvent::Started, this, &AROHPlayerController::OnToggleSkillTree);
		}
		if (ParagonAction)
		{
			EIC->BindAction(ParagonAction, ETriggerEvent::Started, this, &AROHPlayerController::OnToggleParagon);
		}
	}
}

void AROHPlayerController::ToggleUiWindow(EROHUiWindowKind Kind)
{
	// 같은 창 재입력 = 닫기, 다른 창이 열려 있으면 교체
	if (CurrentWindow && CurrentWindowKind == Kind)
	{
		CloseUiWindow();
		return;
	}
	CloseUiWindow();

	TSubclassOf<UROHUiWindow> WindowClass;
	switch (Kind)
	{
	case EROHUiWindowKind::Waypoint:  WindowClass = UROHWaypointWindow::StaticClass(); break;
	case EROHUiWindowKind::Inventory: WindowClass = UROHInventoryWindow::StaticClass(); break;
	case EROHUiWindowKind::SkillTree: WindowClass = UROHSkillTreeWindow::StaticClass(); break;
	case EROHUiWindowKind::Stash:     WindowClass = UROHStashWindow::StaticClass(); break;
	case EROHUiWindowKind::Paragon:   WindowClass = UROHParagonWindow::StaticClass(); break;
	default: break; // Vendor는 OpenVendorWindow 전용 (NPC 참조 필요)
	}
	if (!WindowClass)
	{
		return;
	}

	CurrentWindow = CreateWidget<UROHUiWindow>(this, WindowClass);
	if (CurrentWindow)
	{
		CurrentWindow->AddToViewport(10); // 디버그 메시지 위
		CurrentWindowKind = Kind;
	}
}

void AROHPlayerController::OpenVendorWindow(AROHTownNpc* Npc)
{
	if (!Npc)
	{
		return;
	}
	// 벤더는 토글 없이 항상 새로 연다 (다른 NPC로 갈아탈 때 재고/역할 갱신)
	CloseUiWindow();

	UROHVendorWindow* Window = CreateWidget<UROHVendorWindow>(this, UROHVendorWindow::StaticClass());
	if (Window)
	{
		Window->SetNpc(Npc); // AddToViewport(NativeConstruct → RefreshContents) 전에 주입
		Window->AddToViewport(10);
		CurrentWindow = Window;
		CurrentWindowKind = EROHUiWindowKind::Vendor;
	}
}

void AROHPlayerController::CloseUiWindow()
{
	if (CurrentWindow)
	{
		CurrentWindow->RemoveFromParent();
		CurrentWindow = nullptr;
	}
	CurrentWindowKind = EROHUiWindowKind::None;
}

void AROHPlayerController::OnToggleInventory()
{
	ToggleUiWindow(EROHUiWindowKind::Inventory);
}

void AROHPlayerController::OnToggleSkillTree()
{
	ToggleUiWindow(EROHUiWindowKind::SkillTree);
}

void AROHPlayerController::OnToggleParagon()
{
	ToggleUiWindow(EROHUiWindowKind::Paragon);
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
	// UI 창이 열려 있으면 게임 입력 차단 (UI 2차 — 창 조작 중 오발 방지)
	if (IsUiWindowOpen())
	{
		return;
	}
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

void AROHPlayerController::OnSkill4()
{
	ActivateSlot(4);
}

void AROHPlayerController::ActivateSlot(int32 SlotIndex)
{
	// UI 창이 열려 있으면 스킬 1~4 포함 전 슬롯 차단 (UI 2차)
	if (IsUiWindowOpen())
	{
		return;
	}
	if (AROHPlayerCharacter* PlayerCharacter = Cast<AROHPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->ActivateAbilityBySlot(SlotIndex);
	}
}

void AROHPlayerController::OnSetDestinationStarted()
{
	// UI 창이 열려 있으면 좌클릭 이동 차단 (UI 2차 — 창 배경 클릭이 이동으로 새는 오발 방지)
	if (IsUiWindowOpen())
	{
		// 누른 채 창을 닫고 떼면 Released가 스테일 CachedDestination으로 클릭 이동을
		// 발동시킨다 — 홀드 판정치를 넘겨 이 클릭이 이동으로 이어지지 않게 한다
		FollowTime = ShortPressThreshold + 1.f;
		return;
	}
	StopMovement();
	FollowTime = 0.f;
}

void AROHPlayerController::OnSetDestinationTriggered()
{
	if (IsUiWindowOpen())
	{
		return;
	}
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
	if (IsUiWindowOpen())
	{
		return;
	}
	// 짧은 클릭이면 내비게이션으로 목적지까지 이동
	if (FollowTime <= ShortPressThreshold)
	{
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, CachedDestination);
	}
	FollowTime = 0.f;
}
