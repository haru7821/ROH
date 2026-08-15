#include "Character/ROHBossCharacter.h"
#include "Character/ROHAttributeSet.h"
#include "Abilities/Monster/ROHAbility_BossSlam.h"
#include "Abilities/Monster/ROHAbility_BossBarrage.h"
#include "Abilities/Monster/ROHAbility_MonsterAttack.h"
#include "Abilities/ROHAbilitySystemComponent.h" // TObjectPtr<UROHAbilitySystemComponent> 멤버 호출에 완전한 타입 필요
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

AROHBossCharacter::AROHBossCharacter()
{
	BossName = FText::FromString(TEXT("발타르"));
	SpecialAbility = UROHAbility_BossSlam::StaticClass();

	// 덩치: 일반 몬스터보다 크게 (그레이박스 메시는 캡슐 크기에 맞춰 스케일됨)
	GetCapsuleComponent()->SetCapsuleSize(60.f, 130.f);

	// 스탯: 액트1 중간 보스 (docs/02 §1.3 파생 공식 기준)
	BaseMaxHealth = 1400.f;
	BaseDexterity = 30.f;   // AR 150 / Def 60
	BaseLevel = 8.f;        // 명중 공식의 레벨 항
	BaseMoveSpeed = 380.f;
	AttackDamage = 25.f;
	AttackRange = 250.f;
	AggroRange = 2500.f;
	AttackInterval = 2.f;
	XPValue = 500;
	TreasureClassId = TEXT("TC_Boss");
	CorpseLifetime = 10.f;
}

void AROHBossCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (SpecialAbility && AbilitySystemComponent && !AbilitySystemComponent->FindAbilitySpecFromClass(SpecialAbility))
	{
		GrantAbility(SpecialAbility);
	}
}

void AROHBossCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsAlive())
	{
		return;
	}

	const APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!Player)
	{
		return;
	}
	const float Distance = FVector::Dist2D(GetActorLocation(), Player->GetActorLocation());

	// 보스 HP 표시 (근처에 있을 때만)
	if (GEngine && AttributeSet && Distance < HealthBarVisibleRange)
	{
		const float Health = AttributeSet->GetHealth();
		const float MaxHealth = FMath::Max(AttributeSet->GetMaxHealth(), 1.f);
		GEngine->AddOnScreenDebugMessage(7, 0.5f, bEnraged ? FColor::Red : FColor::Orange,
			FString::Printf(TEXT("보스 %s | 생명 %.0f/%.0f (%.0f%%)%s"),
				*BossName.ToString(), Health, MaxHealth, Health / MaxHealth * 100.f,
				bEnraged ? TEXT(" [광폭화]") : TEXT("")));
	}

	// 내려찍기: 주기 도래 + 사거리 내일 때 발동
	const float Now = GetWorld()->GetTimeSeconds();
	if (SpecialAbility && AbilitySystemComponent && Now >= NextSpecialTime && Distance < SpecialTriggerRange)
	{
		if (AbilitySystemComponent->TryActivateAbilityByClass(SpecialAbility))
		{
			NextSpecialTime = Now + SpecialInterval;
		}
	}
}

void AROHBossCharacter::HandleDamageTaken(float Damage, AActor* InstigatorActor)
{
	Super::HandleDamageTaken(Damage, InstigatorActor);

	// 1회 광폭화: 피해/이속 증가 (막타로 죽는 순간에는 발동하지 않음 — Health 0 제외)
	if (!bEnraged && AttributeSet && AttributeSet->GetHealth() > 0.f
		&& AttributeSet->GetHealth() <= AttributeSet->GetMaxHealth() * EnrageHealthRatio)
	{
		bEnraged = true;
		AttackDamage *= EnrageDamageMultiplier;
		if (AbilitySystemComponent)
		{
			AbilitySystemComponent->ApplyModToAttribute(
				UROHAttributeSet::GetMoveSpeedAttribute(), EGameplayModOp::Additive, EnrageMoveSpeedBonus);
		}
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red,
				FString::Printf(TEXT("%s이(가) 광폭화했다!"), *BossName.ToString()));
		}
	}
}

AROHBossMorgath::AROHBossMorgath()
{
	BossName = FText::FromString(TEXT("모르가스"));
	SpecialAbility = UROHAbility_BossBarrage::StaticClass();
	SpecialInterval = 7.f;
	SpecialTriggerRange = 1400.f; // 원거리형: 먼 거리에서도 패턴 사용

	// 스탯: 액트1 최종 보스 — 발타르보다 강함 (docs/04 M4)
	GetCapsuleComponent()->SetCapsuleSize(55.f, 120.f);
	BaseMaxHealth = 2000.f;
	BaseDexterity = 40.f;
	BaseLevel = 12.f;
	BaseMoveSpeed = 340.f;
	AttackDamage = 20.f;
	AttackRange = 900.f;
	PreferredRange = 650.f; // 거리 유지 (사수형 AI 카이팅 재사용)
	AttackInterval = 1.8f;
	AttackAbility = UROHAbility_MonsterRanged::StaticClass();
	AttackDamageType = EROHDamageType::Fire; // 기본 원거리도 화염탄 (3연발 패턴과 속성 통일)
	XPValue = 1200;
	EnrageHealthRatio = 0.35f;
	EnrageMoveSpeedBonus = 150.f;
}
