#pragma once

#include "CoreMinimal.h"
#include "Character/ROHPlayerCharacter.h"
#include "ROHPlayerClasses.generated.h"

/** 전사: 분노 자원, 근접 물리 (docs/02 §1.1) */
UCLASS()
class ROH_API AROHWarriorCharacter : public AROHPlayerCharacter
{
	GENERATED_BODY()

public:
	AROHWarriorCharacter();
};

/** 원소술사: 마나 자원, 원거리 마법 (docs/02 §1.2) */
UCLASS()
class ROH_API AROHElementalistCharacter : public AROHPlayerCharacter
{
	GENERATED_BODY()

public:
	AROHElementalistCharacter();
};
