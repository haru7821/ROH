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

protected:
	/** 애셋 카탈로그 키 (b32 — docs/13) */
	virtual FName GetCatalogMeshKey() const override { return TEXT("SM_Player_Warrior"); }
};

/** 원소술사: 마나 자원, 원거리 마법 (docs/02 §1.2) */
UCLASS()
class ROH_API AROHElementalistCharacter : public AROHPlayerCharacter
{
	GENERATED_BODY()

public:
	AROHElementalistCharacter();

protected:
	/** 애셋 카탈로그 키 (b32 — docs/13) */
	virtual FName GetCatalogMeshKey() const override { return TEXT("SM_Player_Elementalist"); }
};
