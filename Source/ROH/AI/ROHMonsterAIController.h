#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ROHMonsterAIController.generated.h"

class AROHMonsterCharacter;
class AROHCharacterBase;

/**
 * M1용 경량 몬스터 AI (틱 기반 상태 판단).
 * - 어그로 범위 내 플레이어 추적 → 사거리 진입 시 공격 어빌리티 발동
 * - 원거리형(PreferredRange > 0)은 거리를 유지하며 사격
 * 패턴형 보스 AI는 M4에서 Behavior Tree로 별도 구현 (docs/04).
 */
UCLASS()
class ROH_API AROHMonsterAIController : public AAIController
{
	GENERATED_BODY()

public:
	AROHMonsterAIController();

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	AROHCharacterBase* FindPlayerTarget() const;
	void TryAttack(AROHMonsterCharacter* Monster, AROHCharacterBase* Target);

	bool bAggroed = false;
	float LastAttackTime = -1000.f;
	float LastRepathTime = -1000.f;
};
