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
#include "Core/ROHPlayerController.h" // ToggleUiWindow/OpenVendorWindow — UI 창
#include "Campaign/ROHCampaignSubsystem.h"
#include "World/ROHWaypoint.h"
#include "World/ROHZoneManager.h"
#include "World/ROHTownNpc.h"
#include "World/ROHStashChest.h"
#include "Save/ROHAccountSubsystem.h"
#include "GameplayEffect.h"
#include "ROHGameplayTags.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h" // GetSubsystem<T>() 템플릿 인스턴스화에 완전한 타입 필요
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
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
	SkillTree->BumpChangeSerial(); // b31: 스킬트리 창/스킬바의 배치 표시 자동 갱신
	return true;
}

TSubclassOf<UROHGameplayAbility> AROHPlayerCharacter::GetSlotAbilityClass(int32 SlotIndex) const
{
	return DefaultAbilities.IsValidIndex(SlotIndex) ? DefaultAbilities[SlotIndex] : nullptr;
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
	ApplyParagonBonuses(); // 계정 공유 — 클래스 전환/로드 후에도 자동 적용
}

void AROHPlayerCharacter::ApplyParagonBonuses()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	if (ParagonEffectHandle.IsValid())
	{
		AbilitySystemComponent->RemoveActiveGameplayEffect(ParagonEffectHandle);
		ParagonEffectHandle = FActiveGameplayEffectHandle();
	}

	const UROHAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UROHAccountSubsystem>() : nullptr;
	if (!Account)
	{
		return;
	}

	// 장비/패시브/난이도 페널티와 동일 패턴의 런타임 무한 GE — 4분류 합산 (M5 최종 정복자)
	UGameplayEffect* ParagonEffect = NewObject<UGameplayEffect>(GetTransientPackage());
	ParagonEffect->DurationPolicy = EGameplayEffectDurationType::Infinite;
	for (const auto& Pair : Account->GetParagonAllocations())
	{
		FGameplayAttribute BonusAttribute;
		float PerPoint = 0.f;
		if (Pair.Value <= 0 || !UROHAccountSubsystem::GetParagonCategoryBonus(Pair.Key, BonusAttribute, PerPoint))
		{
			continue;
		}
		FGameplayModifierInfo Modifier;
		Modifier.Attribute = BonusAttribute;
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(PerPoint * Pair.Value));
		ParagonEffect->Modifiers.Add(Modifier);
	}
	if (ParagonEffect->Modifiers.Num() == 0)
	{
		return;
	}
	ParagonEffectHandle = AbilitySystemComponent->ApplyGameplayEffectToSelf(
		ParagonEffect, 1.f, AbilitySystemComponent->MakeEffectContext());
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
		// 만렙이면 XP 자리를 정복자 진행으로 대체 (M5 최종)
		FString GrowthText;
		const UROHAccountSubsystem* Account = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UROHAccountSubsystem>() : nullptr;
		if (Progression->GetLevel() >= UROHProgressionComponent::MaxLevel && Account)
		{
			GrowthText = FString::Printf(TEXT("정복자 Lv %d (XP %d/%d) | 정복P %d"),
				Account->GetParagonLevel(), Account->GetParagonXP(),
				UROHAccountSubsystem::ParagonXPForNextLevel(Account->GetParagonLevel()),
				Account->GetParagonPoints());
		}
		else
		{
			GrowthText = FString::Printf(TEXT("XP %d/%d"),
				Progression->GetXP(), UROHProgressionComponent::XPForNextLevel(Progression->GetLevel()));
		}
		GEngine->AddOnScreenDebugMessage(5, 0.5f, FColor::White,
			FString::Printf(TEXT("Lv %d | %s | 스탯P %d | 스킬P %d | 골드 %d"),
				Progression->GetLevel(), *GrowthText,
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

	// 1) 근처 지면 드랍 습득 — 웨이포인트 창보다 우선 (기둥 근처 드랍이 못 줍게 되는 것 방지)
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

	// 2) 근처 마을 NPC/보관함 (300uu — 최근접 우선, 웨이포인트보다 앞 단계)
	AROHTownNpc* NearNpc = nullptr;
	float BestNpcDistSq = FMath::Square(300.f);
	for (TActorIterator<AROHTownNpc> It(GetWorld()); It; ++It)
	{
		const float NpcDistSq = FVector::DistSquared(GetActorLocation(), It->GetActorLocation());
		if (NpcDistSq < BestNpcDistSq)
		{
			BestNpcDistSq = NpcDistSq;
			NearNpc = *It;
		}
	}
	const AROHStashChest* NearChest = nullptr;
	float BestChestDistSq = FMath::Square(300.f);
	for (TActorIterator<AROHStashChest> It(GetWorld()); It; ++It)
	{
		const float ChestDistSq = FVector::DistSquared(GetActorLocation(), It->GetActorLocation());
		if (ChestDistSq < BestChestDistSq)
		{
			BestChestDistSq = ChestDistSq;
			NearChest = *It;
		}
	}
	if (NearChest && (!NearNpc || BestChestDistSq < BestNpcDistSq))
	{
		if (AROHPlayerController* PC = Cast<AROHPlayerController>(GetController()))
		{
			PC->OpenStashWindow();
		}
		return; // 캐스트 실패 시에도 NPC 분기로 흘러가지 않도록 차단
	}
	if (NearNpc)
	{
		if (GEngine)
		{
			// 인사말 (docs/12 문구가 사양)
			GEngine->AddOnScreenDebugMessage(2, 4.f, NearNpc->GetRoleColor(),
				FString::Printf(TEXT("%s: \"%s\""), *NearNpc->GetDisplayName().ToString(), *NearNpc->GetGreeting().ToString()));
		}
		if (AROHPlayerController* PC = Cast<AROHPlayerController>(GetController()))
		{
			PC->OpenVendorWindow(NearNpc);
		}
		return;
	}

	// 3) 근처 웨이포인트 = 지역 선택 창 (UI 1차 — 사용자 결정: E 순환 이동 대체)
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
	if (NearWaypoint)
	{
		if (AROHPlayerController* PC = Cast<AROHPlayerController>(GetController()))
		{
			PC->ToggleUiWindow(EROHUiWindowKind::Waypoint);
			return;
		}
	}

	// 4) 인벤토리의 첫 장비 장착
	if (Inventory && Inventory->EquipFirstEquippable())
	{
		ShowMessage(TEXT("장비 장착 완료 (콘솔 ROHDumpAttrs로 스탯 확인)"));
		return;
	}

	ShowMessage(TEXT("상호작용 대상 없음 (장착할 장비/주울 아이템 없음)"));
}

bool AROHPlayerCharacter::TravelToZone(int32 TargetZoneIndex)
{
	// 웨이포인트 창/ROHWarp 공용 이동 (기존 E 순환 로직 대체 — 지역 선택형 UI)
	auto ShowMessage = [](const FString& Text)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(2, 3.f, FColor::Cyan, Text);
		}
	};

	AROHZoneManager* ZoneManager = GetZoneManager();
	const UROHCampaignSubsystem* Campaign = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UROHCampaignSubsystem>() : nullptr;
	if (!ZoneManager || !Campaign || TargetZoneIndex < 0 || TargetZoneIndex >= ZoneManager->GetZoneCount())
	{
		return false;
	}

	const FString ZoneName = ZoneManager->GetZoneName(TargetZoneIndex).ToString();
	if (!Campaign->IsWaypointActivated(TargetZoneIndex))
	{
		ShowMessage(FString::Printf(TEXT("%s: 미발견 — 직접 걸어가 웨이포인트를 발견하세요"), *ZoneName));
		return false;
	}

	if (TeleportTo(ZoneManager->GetWaypointLocation(TargetZoneIndex) + FVector(0.f, 0.f, 100.f), GetActorRotation()))
	{
		ShowMessage(FString::Printf(TEXT("이동: %s"), *ZoneName));
		return true;
	}
	ShowMessage(TEXT("이동 실패 (목적지가 막혀 있음)"));
	return false;
}

void AROHPlayerCharacter::PlayAncientCelebration(const FString& ItemName)
{
	// 그레이박스 당첨 연출 — 최종안은 M6에서 나이아가라/사운드/카메라셰이크로 교체 (애셋 금지 단계)
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor(220, 60, 60),
			FString::Printf(TEXT("★★★ 고대무기 강림: %s ★★★"), *ItemName));
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor(255, 200, 60),
			TEXT("영웅이 사용하던 고대의 힘이 깃들었다"));
	}

	// 월드 연출: 2.4초간 0.2초 간격 12틱 — 확장 링 2겹(진홍/금 교차) + 상승 나선 점.
	// Blizzard와 동일한 자체 정리 타이머 패턴 (TSharedRef 카운터/핸들 + CreateWeakLambda)
	TSharedRef<int32> TicksLeft = MakeShared<int32>(12);
	TSharedRef<FTimerHandle> TimerRef = MakeShared<FTimerHandle>();
	GetWorldTimerManager().SetTimer(*TimerRef,
		FTimerDelegate::CreateWeakLambda(this, [this, TicksLeft, TimerRef]()
		{
			const int32 TickIndex = 12 - *TicksLeft; // 0..11
			const FVector Center = GetActorLocation()
				- FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 10.f);

			// 확장 링 2겹 — 틱마다 진홍/금 교차
			const float RingRadius = 80.f + TickIndex * 60.f;
			const FColor OuterColor = (TickIndex % 2 == 0) ? FColor(220, 60, 60) : FColor(255, 200, 60);
			const FColor InnerColor = (TickIndex % 2 == 0) ? FColor(255, 200, 60) : FColor(220, 60, 60);
			DrawDebugCircle(GetWorld(), Center, RingRadius, 32, OuterColor, false, 0.25f, 0, 6.f,
				FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
			DrawDebugCircle(GetWorld(), Center, RingRadius * 0.6f, 32, InnerColor, false, 0.25f, 0, 6.f,
				FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);

			// 상승 나선 점 8개 (황금각 회전, 틱당 25uu 상승)
			for (int32 PointIndex = 0; PointIndex < 8; ++PointIndex)
			{
				const float SpiralAngle = FMath::DegreesToRadians(TickIndex * 137.5f + PointIndex * 45.f);
				const FVector SpiralPoint = Center + FVector(
					FMath::Cos(SpiralAngle) * 120.f, FMath::Sin(SpiralAngle) * 120.f, TickIndex * 25.f);
				DrawDebugPoint(GetWorld(), SpiralPoint, 12.f, FColor(255, 200, 60), false, 0.3f);
			}

			if (--(*TicksLeft) <= 0)
			{
				GetWorldTimerManager().ClearTimer(*TimerRef);
			}
		}), 0.2f, true);
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
