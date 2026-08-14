#include "AI/ROHMonsterAIController.h"
#include "Character/ROHMonsterCharacter.h"
#include "Character/ROHCharacterBase.h"
#include "Abilities/ROHGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AROHMonsterAIController::AROHMonsterAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

AROHCharacterBase* AROHMonsterAIController::FindPlayerTarget() const
{
	AROHCharacterBase* Player = Cast<AROHCharacterBase>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	return (Player && Player->IsAlive()) ? Player : nullptr;
}

void AROHMonsterAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AROHMonsterCharacter* Monster = Cast<AROHMonsterCharacter>(GetPawn());
	if (!Monster || !Monster->IsAlive())
	{
		return;
	}

	AROHCharacterBase* Target = FindPlayerTarget();
	if (!Target)
	{
		// 플레이어 사망/부재 시 어그로 해제 (부활 지점까지 추격 방지)
		bAggroed = false;
		StopMovement();
		return;
	}

	const float Distance = FVector::Dist2D(Monster->GetActorLocation(), Target->GetActorLocation());

	// 어그로: 한 번 물면 놓지 않는다 (M1 단순화)
	if (!bAggroed)
	{
		if (Distance > Monster->GetAggroRange())
		{
			return;
		}
		bAggroed = true;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const bool bRanged = Monster->GetPreferredRange() > 0.f;
	const float DesiredRange = bRanged ? Monster->GetPreferredRange() : Monster->GetAttackRange() * 0.8f;

	if (bRanged && Distance < DesiredRange * 0.6f)
	{
		// 원거리형: 너무 가까우면 후퇴하며 거리 유지
		if (Now - LastRepathTime > 0.25f)
		{
			const FVector Away = (Monster->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
			MoveToLocation(Monster->GetActorLocation() + Away * 400.f, 50.f);
			LastRepathTime = Now;
		}
		if (Distance <= Monster->GetAttackRange())
		{
			TryAttack(Monster, Target);
		}
	}
	else if (Distance > DesiredRange)
	{
		// 이동 경로 갱신은 0.25초마다 (매 틱 재탐색 방지)
		if (Now - LastRepathTime > 0.25f)
		{
			MoveToActor(Target, DesiredRange * 0.7f);
			LastRepathTime = Now;
		}
	}
	else
	{
		StopMovement();
		if (Distance <= Monster->GetAttackRange())
		{
			TryAttack(Monster, Target);
		}
	}
}

void AROHMonsterAIController::TryAttack(AROHMonsterCharacter* Monster, AROHCharacterBase* Target)
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastAttackTime < Monster->GetAttackInterval())
	{
		return;
	}

	TSubclassOf<UROHGameplayAbility> AttackAbility = Monster->GetAttackAbility();
	UAbilitySystemComponent* ASC = Monster->GetAbilitySystemComponent();
	if (!AttackAbility || !ASC)
	{
		return;
	}

	// 대상을 바라보고 공격
	const FVector Direction = (Target->GetActorLocation() - Monster->GetActorLocation()).GetSafeNormal2D();
	if (!Direction.IsNearlyZero())
	{
		Monster->SetActorRotation(Direction.Rotation());
	}

	if (ASC->TryActivateAbilityByClass(AttackAbility))
	{
		LastAttackTime = Now;
	}
}
