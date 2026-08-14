#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ROHGameInstance.generated.h"

/**
 * 세션 수명 데이터의 거점. M3에서 세이브/로드, M5에서 계정 공유 데이터(스태시/정복자)가 붙는다.
 */
UCLASS()
class ROH_API UROHGameInstance : public UGameInstance
{
	GENERATED_BODY()
};
