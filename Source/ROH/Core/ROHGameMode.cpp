#include "Core/ROHGameMode.h"
#include "Core/ROHPlayerController.h"
#include "Character/ROHPlayerCharacter.h"

AROHGameMode::AROHGameMode()
{
	DefaultPawnClass = AROHPlayerCharacter::StaticClass();
	PlayerControllerClass = AROHPlayerController::StaticClass();
}
