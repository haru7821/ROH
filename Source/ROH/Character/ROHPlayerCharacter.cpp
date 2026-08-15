#include "Character/ROHPlayerCharacter.h"
#include "Character/ROHAttributeSet.h"
#include "Items/ROHInventoryComponent.h"
#include "Loot/ROHItemPickup.h"
#include "Abilities/ROHAbilitySystemComponent.h"
#include "Abilities/ROHGameplayAbility.h" // TSubclassOf<UROHGameplayAbility> 변환에 완전한 타입 필요 (유니티 빌드 의존 금지)
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Progression/ROHProgressionComponent.h"
#include "Progression/ROHSkillTreeComponent.h"
#include "Core/ROHGameMode.h"
#include "Campaign/ROHCampaignSubsystem.h"
#include "World/ROHWaypoint.h"
#include "World/ROHZoneManager.h"
#include "GameplayEffect.h"
#include "ROHGameplayTags.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h" // GetSubsystem<T>() 템플릿 인스턴스화에 완전한 타입 필요
#include "ROH.h"

AROHPlayerCharacter::AROHPlayerCharacter()
{
	TeamId = 0;
	// 스탯/어빌리티 프리셋은 서브클래스(전사/원소술사)가 정의 — ROHPlayerClasses 참고

	Inventory = CreateDefaultSubobject<UROHInventoryComponent>(TEXT("Inventory"));
	Progression = CreateDefaultSubobject<UROHProgressionComponent>(TEXT("Progression"));
	SkillTree = CreateDefaultSubobject<UROHSkillTreeComponent>(TEXT("SkillTree"));

	// 이동 방향으로 캐릭터 회전 (쿼터뷰 표준)
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, 640.f, 0.f);
	Movement->bConstrainToPlane = true;
	Movement->bSnapToPlaneAtStart = true;
	Movement->MaxWalkSpeed = 600.f;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->SetUsingAbsoluteRotation(true);
	SpringArm->SetRelativeRotation(FRotator(-55.f, 0.f, 0.f));
	SpringArm->TargetArmLength = 1400.f;
	SpringArm->bDoCollisionTest = false;
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = false;
	SpringArm->bInheritRoll = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
}

void AROHPlayerCharacter::ActivateAbilityBySlot(int32 SlotIndex)
{
	if (!AbilitySystemComponent || !DefaultAbilities.IsValidIndex(SlotIndex) || !DefaultAbilities[SlotIndex])
	{
		return;
	}
	AbilitySystemComponent->TryActivateAbilityByClass(DefaultAbilities[SlotIndex]);
}

bool AROHPlayerCharacter::BindSkillToSlot(int32 SlotIndex, FName SkillId, FString& OutError)
{
	if (SlotIndex < 1 || SlotIndex > MaxSkillSlot)
	{
		OutError = FString::Printf(TEXT("슬롯은 1~%d 입니다."), MaxSkillSlot);
		return false;
	}
	if (!SkillTree)
	{
		OutError = TEXT("스킬트리 컴포넌트가 없습니다.");
		return false;
	}

	const FROHSkillDef* Def = UROHSkillTreeComponent::FindSkillDef(SkillTree->GetPlayerClass(), SkillId);
	if (!Def)
	{
		OutError = TEXT("알 수 없는 스킬입니다. ROHSkillInfo로 목록을 확인하세요.");
		return false;
	}
	if (Def->Kind != EROHSkillKind::Active || !Def->AbilityClass)
	{
		OutError = TEXT("패시브 스킬은 슬롯에 배치할 수 없습니다.");
		return false;
	}
	if (SkillTree->GetRank(SkillId) < 1)
	{
		OutError = FString::Printf(TEXT("먼저 습득하세요: ROHSkillUp %s"), *SkillId.ToString());
		return false;
	}

	// 아직 부여되지 않은 어빌리티면 부여 (기본 슬롯 외 스킬)
	if (AbilitySystemComponent && !AbilitySystemComponent->FindAbilitySpecFromClass(Def->AbilityClass))
	{
		GrantAbility(Def->AbilityClass);
	}

	if (DefaultAbilities.Num() <= SlotIndex)
	{
		DefaultAbilities.SetNum(SlotIndex + 1);
	}
	DefaultAbilities[SlotIndex] = Def->AbilityClass;
	return true;
}

TArray<FName> AROHPlayerCharacter::ExportBoundSkills() const
{
	TArray<FName> Result;
	Result.Init(NAME_None, MaxSkillSlot);
	if (!SkillTree)
	{
		return Result;
	}
	for (int32 Slot = 1; Slot <= MaxSkillSlot; ++Slot)
	{
		if (!DefaultAbilities.IsValidIndex(Slot) || !DefaultAbilities[Slot])
		{
			continue;
		}
		for (const FROHSkillDef& Def : UROHSkillTreeComponent::GetSkillDefs(SkillTree->GetPlayerClass()))
		{
			// 습득한 스킬의 배치만 저장 (미습득 기본 배치는 로드 시 재바인딩 불가 — 경고 스팸 방지)
			if (Def.AbilityClass == DefaultAbilities[Slot] && SkillTree->GetRank(Def.SkillId) >= 1)
			{
				Result[Slot - 1] = Def.SkillId;
				break;
			}
		}
	}
	return Result;
}

void AROHPlayerCharacter::RestoreBoundSkills(const TArray<FName>& SkillIds)
{
	for (int32 i = 0; i < SkillIds.Num() && i < MaxSkillSlot; ++i)
	{
		if (SkillIds[i].IsNone())
		{
			continue;
		}
		FString Error;
		if (!BindSkillToSlot(i + 1, SkillIds[i], Error))
		{
			UE_LOG(LogROH, Warning, TEXT("세이브 슬롯 %d 배치 복원 실패 (%s): %s"),
				i + 1, *SkillIds[i].ToString(), *Error);
		}
	}
}

void AROHPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	ApplyDifficultyResistPenalty();
}

void AROHPlayerCharacter::ApplyDifficultyResistPenalty()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	if (DifficultyPenaltyHandle.IsValid())
	{
		AbilitySystemComponent->RemoveActiveGameplayEffect(DifficultyPenaltyHandle);
		DifficultyPenaltyHandle = FActiveGameplayEffectHandle();
	}

	const UROHCampaignSubsystem* Campaign = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UROHCampaignSubsystem>() : nullptr;
	if (!Campaign)
	{
		return;
	}
	const float Penalty = UROHCampaignSubsystem::GetDifficultyParams(Campaign->GetDifficulty()).PlayerResistPenalty;
	if (FMath::IsNearlyZero(Penalty))
	{
		return;
	}

	// 장비/패시브와 동일 패턴의 런타임 무한 GE (docs/04 M4: 난이도 저항 페널티)
	UGameplayEffect* PenaltyEffect = NewObject<UGameplayEffect>(GetTransientPackage());
	PenaltyEffect->DurationPolicy = EGameplayEffectDurationType::Infinite;
	// 원소 저항 5종만 감산 — PhysicalResistance는 PDR이라 페널티 대상 아님 (docs/10 §4.2~4.3)
	const FGameplayAttribute ResistAttributes[5] = {
		UROHAttributeSet::GetFireResistanceAttribute(),
		UROHAttributeSet::GetColdResistanceAttribute(),
		UROHAttributeSet::GetLightningResistanceAttribute(),
		UROHAttributeSet::GetPoisonResistanceAttribute(),
		UROHAttributeSet::GetShadowResistanceAttribute(),
	};
	for (const FGameplayAttribute& Attribute : ResistAttributes)
	{
		FGameplayModifierInfo Modifier;
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Penalty));
		PenaltyEffect->Modifiers.Add(Modifier);
	}
	DifficultyPenaltyHandle = AbilitySystemComponent->ApplyGameplayEffectToSelf(
		PenaltyEffect, 1.f, AbilitySystemComponent->MakeEffectContext());
}

void AROHPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 성장 상태 표시 (좌상단 두 번째 줄)
	if (GEngine && IsPlayerControlled() && Progression)
	{
		const FString XPText = Progression->GetLevel() >= UROHProgressionComponent::MaxLevel
			? TEXT("MAX")
			: FString::Printf(TEXT("%d/%d"), Progression->GetXP(), UROHProgressionComponent::XPForNextLevel(Progression->GetLevel()));
		GEngine->AddOnScreenDebugMessage(5, 0.5f, FColor::White,
			FString::Printf(TEXT("Lv %d | XP %s | 스탯P %d | 스킬P %d | 골드 %d"),
				Progression->GetLevel(), *XPText,
				Progression->GetStatPoints(), Progression->GetSkillPoints(),
				Inventory ? Inventory->GetGold() : 0));

		// 난이도/퀘스트 목표 표시 (세 번째 줄)
		if (const UROHCampaignSubsystem* Campaign = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UROHCampaignSubsystem>() : nullptr)
		{
			GEngine->AddOnScreenDebugMessage(8, 0.5f, FColor::Emerald, Campaign->GetObjectiveText());
		}

		// 현재 지역 표시 (네 번째 줄) — 지역 밖이면 표시하지 않음
		if (AROHZoneManager* ZoneManager = GetZoneManager())
		{
			const int32 ZoneIndex = ZoneManager->GetZoneIndexAt(GetActorLocation());
			if (ZoneIndex >= 0)
			{
				GEngine->AddOnScreenDebugMessage(9, 0.5f, FColor::Cyan,
					FString::Printf(TEXT("지역: %s%s"),
						*ZoneManager->GetZoneName(ZoneIndex).ToString(),
						ZoneManager->IsTown(ZoneIndex) ? TEXT(" (안전 지대)") : TEXT("")));
			}
		}
	}
}

AROHZoneManager* AROHPlayerCharacter::GetZoneManager()
{
	if (!CachedZoneManager.IsValid())
	{
		for (TActorIterator<AROHZoneManager> It(GetWorld()); It; ++It)
		{
			CachedZoneManager = *It;
			break;
		}
	}
	return CachedZoneManager.Get();
}

void AROHPlayerCharacter::Interact()
{
	auto ShowMessage = [](const FString& Text)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(2, 3.f, FColor::Cyan, Text);
		}
	};

	// 0) 근처 웨이포인트로 순환 이동 (docs/04 M4 지역)
	const AROHWaypoint* NearWaypoint = nullptr;
	float BestWaypointDistSq = FMath::Square(300.f);
	for (TActorIterator<AROHWaypoint> It(GetWorld()); It; ++It)
	{
		const float WaypointDistSq = FVector::DistSquared(GetActorLocation(), It->GetActorLocation());
		if (WaypointDistSq < BestWaypointDistSq)
		{
			BestWaypointDistSq = WaypointDistSq;
			NearWaypoint = *It;
		}
	}
	// 이동할 곳이 없으면 아래 습득/장착 분기로 계속 (웨이포인트 근처 드랍 습득 차단 방지)
	if (NearWaypoint && TravelViaWaypoint(NearWaypoint))
	{
		return;
	}

	// 1) 근처 지면 드랍 습득 (인벤토리 가득 등으로 바닥에 남은 것)
	AROHItemPickup* Nearest = nullptr;
	float BestDistSq = FMath::Square(250.f);
	for (TActorIterator<AROHItemPickup> It(GetWorld()); It; ++It)
	{
		const float DistSq = FVector::DistSquared(GetActorLocation(), It->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Nearest = *It;
		}
	}
	if (Nearest && Nearest->TryGive(this))
	{
		ShowMessage(TEXT("아이템 습득"));
		return;
	}

	// 2) 인벤토리의 첫 장비 장착
	if (Inventory && Inventory->EquipFirstEquippable())
	{
		ShowMessage(TEXT("장비 장착 완료 (콘솔 ROHDumpAttrs로 스탯 확인)"));
		return;
	}

	ShowMessage(TEXT("상호작용 대상 없음 (장착할 장비/주울 아이템 없음)"));
}

bool AROHPlayerCharacter::TravelViaWaypoint(const AROHWaypoint* FromWaypoint)
{
	auto ShowMessage = [](const FString& Text)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(2, 3.f, FColor::Cyan, Text);
		}
	};
	if (!FromWaypoint)
	{
		return false;
	}

	// 지역 순서 오름차순으로 활성화된 웨이포인트를 모은다 (현재 지역 제외)
	const int32 CurrentZone = FromWaypoint->GetZoneIndex();
	TArray<const AROHWaypoint*> Candidates;
	for (TActorIterator<AROHWaypoint> It(GetWorld()); It; ++It)
	{
		if (It->GetZoneIndex() != CurrentZone && It->IsActivated())
		{
			Candidates.Add(*It);
		}
	}
	if (Candidates.Num() == 0)
	{
		// 이동 불가 — 호출자가 습득/장착 분기로 계속하도록 false 반환
		return false;
	}
	Candidates.Sort([](const AROHWaypoint& A, const AROHWaypoint& B)
	{
		return A.GetZoneIndex() < B.GetZoneIndex();
	});

	// 순환: 현재 지역보다 큰 첫 번째, 없으면 가장 앞(랩어라운드)
	const AROHWaypoint* Target = Candidates[0];
	for (const AROHWaypoint* Candidate : Candidates)
	{
		if (Candidate->GetZoneIndex() > CurrentZone)
		{
			Target = Candidate;
			break;
		}
	}

	if (TeleportTo(Target->GetActorLocation() + FVector(0.f, 0.f, 100.f), GetActorRotation()))
	{
		ShowMessage(FString::Printf(TEXT("이동: %s"), *Target->GetZoneName().ToString()));
	}
	else
	{
		ShowMessage(TEXT("이동 실패 (목적지가 막혀 있음)"));
	}
	return true;
}

void AROHPlayerCharacter::HandleDeath(AActor* Killer)
{
	if (!IsAlive())
	{
		return;
	}
	Super::HandleDeath(Killer);

	if (AROHGameMode* GameMode = GetWorld()->GetAuthGameMode<AROHGameMode>())
	{
		GameMode->SchedulePlayerRespawn(this);
	}
}

void AROHPlayerCharacter::Revive(const FVector& Location)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(ROHGameplayTags::State_Dead);
	}
	if (AttributeSet)
	{
		AttributeSet->SetHealth(AttributeSet->GetMaxHealth());
		AttributeSet->SetMana(AttributeSet->GetMaxMana());
		AttributeSet->SetRage(0.f);
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	if (VisualMesh)
	{
		VisualMesh->SetRelativeRotation(FRotator::ZeroRotator);
	}
	SetActorLocation(Location);
}
