#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "ROHCheatManager.generated.h"

/**
 * 밸런스 반복 테스트용 치트 콘솔 (docs/03 §5).
 * PIE 콘솔(`)에서 사용. 예:
 *   ROHSetAttr Strength 50
 *   ROHDumpAttrs
 */
UCLASS()
class ROH_API UROHCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	/** 플레이어 어트리뷰트 베이스 값을 설정한다. 예: ROHSetAttr Health 999 */
	UFUNCTION(Exec)
	void ROHSetAttr(FName AttributeName, float Value);

	/** 플레이어의 모든 어트리뷰트를 로그/화면에 출력한다. */
	UFUNCTION(Exec)
	void ROHDumpAttrs();
};
