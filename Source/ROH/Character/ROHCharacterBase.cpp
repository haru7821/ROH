#include "Character/ROHCharacterBase.h"
#include "Abilities/ROHAbilitySystemComponent.h"
#include "Abilities/ROHGameplayAbility.h"
#include "Character/ROHAttributeSet.h"
#include "Character/ROHMonsterCharacter.h"
#include "Core/ROHAssetCatalog.h" // 카탈로그 메시 우선 적용 (b32)
#include "Core/ROHProceduralVisual.h" // 프로시저럴 조형 (b33)
#include "Materials/MaterialInterface.h"
#include "ROHGameplayTags.h"
#include "GameplayEffect.h" // b29 % 배율 무한 GE 구성
#include "GameplayEffectTypes.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/StaticMesh.h"
#include "ROH.h"

namespace
{
	// 엔진 설치본마다 기본 메시 구성이 달라 후보를 순서대로 시도
	UStaticMesh* LoadGrayboxBodyMesh()
	{
		static const TCHAR* CandidatePaths[] = {
			TEXT("/Engine/BasicShapes/Capsule.Capsule"),
			TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),
			TEXT("/Engine/EngineMeshes/Cylinder.Cylinder"),
			TEXT("/Engine/EngineMeshes/Sphere.Sphere"),
		};
		for (const TCHAR* Path : CandidatePaths)
		{
			if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Path))
			{
				UE_LOG(LogROH, Log, TEXT("그레이박스 메시 로드: %s"), Path);
				return Mesh;
			}
		}
		return nullptr;
	}
}

AROHCharacterBase::AROHCharacterBase()
{
	// 그레이박스 단계: 디버그 드로잉/상태 표시용 틱 활성화 (아트 반영 후 비활성화)
	PrimaryActorTick.bCanEverTick = true;

	AbilitySystemComponent = CreateDefaultSubobject<UROHAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UROHAttributeSet>(TEXT("AttributeSet"));

	// 그레이박스: 엔진 기본 캡슐 메시로 몸통 표시 (아트는 M6)
	// 메시 로드는 BeginPlay에서 수행 — 모듈 로딩 시점(CDO 생성)에는 엔진 콘텐츠가
	// 아직 마운트되지 않아 실패하는 것을 로컬 5.8 빌드에서 확인
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(GetCapsuleComponent());
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.7f));
}

UAbilitySystemComponent* AROHCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

bool AROHCharacterBase::IsAlive() const
{
	return AbilitySystemComponent && !AbilitySystemComponent->HasMatchingGameplayTag(ROHGameplayTags::State_Dead);
}

void AROHCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// 비주얼 메시 로드 (생성자 시점엔 콘텐츠 미마운트 — BeginPlay에서 수행). 우선순위 (b33):
	// 카탈로그 애셋(임포트본) → 프로시저럴 조형(기본값) → 단순 그레이박스(도형 로드 실패 안전망)
	if (VisualMesh && !VisualMesh->GetStaticMesh())
	{
		UStaticMesh* BodyMesh = nullptr;
		bool bCatalogMesh = false;
		const FName CatalogKey = GetCatalogMeshKey();
		if (!CatalogKey.IsNone())
		{
			BodyMesh = ROHAssetCatalog::LoadMesh(CatalogKey);
			bCatalogMesh = BodyMesh != nullptr;
		}

		// 프로시저럴 조형 (b33): 카탈로그가 없을 때 VisualMesh를 빈 컨테이너로 쓰고 파트를 부착
		// (HandleDeath의 눕히기 회전이 컨테이너째 적용). 성공 시 단일 메시는 설정하지 않는다.
		bool bProceduralBuilt = false;
		if (!bCatalogMesh && !CatalogKey.IsNone())
		{
			VisualMesh->SetRelativeScale3D(FVector(1.f)); // 생성자 캡슐 스케일 제거 — 파트 왜곡 방지
			bProceduralBuilt = ROHProceduralVisual::BuildCharacterVisual(*this, *VisualMesh, CatalogKey,
				GetCapsuleComponent()->GetScaledCapsuleRadius(), GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		}

		if (!bCatalogMesh && !bProceduralBuilt)
		{
			BodyMesh = LoadGrayboxBodyMesh();
		}

		if (BodyMesh)
		{
			VisualMesh->SetStaticMesh(BodyMesh);
			// 메시 종류와 무관하게 캡슐 크기에 맞춰 정규화 (바닥 파묻힘 방지 — 카탈로그 메시도 동일 적용)
			const FVector Extent = BodyMesh->GetBounds().BoxExtent;
			const float TargetHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			if (Extent.GetMin() > KINDA_SMALL_NUMBER)
			{
				VisualMesh->SetRelativeScale3D(FVector(40.f / Extent.X, 40.f / Extent.Y, TargetHalfHeight / Extent.Z));
			}
			// 머티리얼 오버라이드는 카탈로그 메시 적용 성공 시에만 (M_<키 몸통>)
			if (bCatalogMesh)
			{
				if (UMaterialInterface* OverrideMaterial = ROHAssetCatalog::LoadMaterialForMeshKey(CatalogKey))
				{
					VisualMesh->SetMaterial(0, OverrideMaterial);
				}
			}
		}
		else if (!bProceduralBuilt)
		{
			// 마지막 안전망: 캡슐 콜리전 와이어프레임이라도 게임 중 표시
			GetCapsuleComponent()->SetHiddenInGame(false);
			UE_LOG(LogROH, Warning, TEXT("그레이박스 메시를 찾지 못해 캡슐 와이어프레임으로 표시합니다"));
		}
	}

	// AI 등 컨트롤러 빙의 전에 스폰되는 경우 대비
	if (AbilitySystemComponent && !AbilitySystemComponent->AbilityActorInfo->OwnerActor.IsValid())
	{
		InitAbilityActorInfo();
	}
}

void AROHCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitAbilityActorInfo();
}

void AROHCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 재생 (docs/10 §2 완전형 — b29, 초당): (VIT×0.05 + Flat) × (1 + %/100), MP는 INT(Energy)×0.1 기반
	// % 가 0이면 ×1.0 — 기존 수치와 완전 동일. 사망 후 재생 금지 — IsAlive 가드 필수 (시체의 Health는 0 < Max)
	if (IsAlive() && AttributeSet)
	{
		if (AttributeSet->GetHealth() < AttributeSet->GetMaxHealth())
		{
			const float HealthGain = (AttributeSet->GetVitality() * 0.05f + AttributeSet->GetHealthRegen())
				* (1.f + AttributeSet->GetHealthRegenPct() / 100.f) * DeltaSeconds;
			if (HealthGain > 0.f)
			{
				AttributeSet->SetHealth(FMath::Min(AttributeSet->GetHealth() + HealthGain, AttributeSet->GetMaxHealth()));
			}
		}
		if (AttributeSet->GetMana() < AttributeSet->GetMaxMana())
		{
			const float ManaGain = (AttributeSet->GetEnergy() * 0.1f + AttributeSet->GetManaRegen())
				* (1.f + AttributeSet->GetManaRegenPct() / 100.f) * DeltaSeconds;
			if (ManaGain > 0.f)
			{
				AttributeSet->SetMana(FMath::Min(AttributeSet->GetMana() + ManaGain, AttributeSet->GetMaxMana()));
			}
		}
	}

	// 그레이박스: 메시 유무와 무관하게 몸통을 매 프레임 선으로 그린다
	// (플레이어 = 초록, 몬스터 = 빨강, 사망 = 회색)
	const FColor BodyColor = !IsAlive() ? FColor(120, 120, 120)
		: (TeamId == 0 ? FColor::Green : FColor::Red);
	DrawDebugCapsule(GetWorld(), GetActorLocation(),
		GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),
		GetCapsuleComponent()->GetScaledCapsuleRadius(),
		GetActorQuat(), BodyColor, false, -1.f, 0, 2.f);

	// 바라보는 방향 표시
	DrawDebugLine(GetWorld(), GetActorLocation(),
		GetActorLocation() + GetActorForwardVector() * 100.f, BodyColor, false, -1.f, 0, 2.f);

	// 플레이어: 좌상단 상태 텍스트 (빌드 태그 포함 — 실행 중인 코드 버전 확인용)
	if (GEngine && IsPlayerControlled() && AttributeSet)
	{
		// 몬스터 수 집계는 0.25초 간격으로 캐시 (매 프레임 월드 순회 방지)
		MonsterCountTimer -= DeltaSeconds;
		if (MonsterCountTimer <= 0.f)
		{
			MonsterCountTimer = 0.25f;
			CachedAliveMonsters = 0;
			for (TActorIterator<AROHMonsterCharacter> It(GetWorld()); It; ++It)
			{
				if (It->IsAlive())
				{
					++CachedAliveMonsters;
				}
			}
		}
		GEngine->AddOnScreenDebugMessage(1, 0.5f, FColor::Yellow,
			FString::Printf(TEXT("ROH %s | 생명 %.0f/%.0f | 분노 %.0f | 마나 %.0f | 몬스터 %d"),
				ROH_BUILD_TAG,
				AttributeSet->GetHealth(), AttributeSet->GetMaxHealth(),
				AttributeSet->GetRage(), AttributeSet->GetMana(), CachedAliveMonsters));
	}
}

void AROHCharacterBase::InitAbilityActorInfo()
{
	if (!AbilitySystemComponent)
	{
		return;
	}
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	InitializeAttributes();
	GrantDefaultAbilities();

	if (!bAttributeDelegatesBound)
	{
		bAttributeDelegatesBound = true;
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UROHAttributeSet::GetMoveSpeedAttribute())
			.AddUObject(this, &AROHCharacterBase::OnMoveSpeedChanged);
		// % 배율 (b29): 장비 착탈 등으로 HealthPct/ManaPct가 바뀌면 배율 GE 재적용
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UROHAttributeSet::GetHealthPctAttribute())
			.AddUObject(this, &AROHCharacterBase::OnVitalPercentChanged);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UROHAttributeSet::GetManaPctAttribute())
			.AddUObject(this, &AROHCharacterBase::OnVitalPercentChanged);
	}

	// 초기 반영 (% 전부 0이면 무동작 — 기존 수치 유지)
	RefreshVitalPercentScaling();
}

void AROHCharacterBase::OnMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	GetCharacterMovement()->MaxWalkSpeed = Data.NewValue;
}

void AROHCharacterBase::OnVitalPercentChanged(const FOnAttributeChangeData& Data)
{
	RefreshVitalPercentScaling();
}

void AROHCharacterBase::RefreshVitalPercentScaling()
{
	if (!AbilitySystemComponent || !AttributeSet)
	{
		return;
	}

	// 이전 GE 제거는 새 GE 적용 뒤에 한다 — 제거가 먼저면 Max가 순간 무배율로 하락해
	// PostAttributeChange 클램프가 현재 HP/MP를 영구히 깎는다 (Sup b29 지적).
	// 배율 하락(장비 해제) 시의 클램프는 의도된 동작으로 유지된다.
	const FActiveGameplayEffectHandle PreviousHandle = VitalPctEffectHandle;
	VitalPctEffectHandle = FActiveGameplayEffectHandle();

	const float HealthPct = AttributeSet->GetHealthPct();
	const float ManaPct = AttributeSet->GetManaPct();
	// % 전부 0 = GE 없음 — 리팩터 전 수치와 완전 동일 (b29 회귀 가드)
	if (!FMath::IsNearlyZero(HealthPct) || !FMath::IsNearlyZero(ManaPct))
	{
		// docs/10 §2: HPmax = (베이스 + Flat) × (1 + %HP/100)
		// GAS 집계가 (Base + Additive) × Multiply 라서 MultiplyAdditive 모디파이어 1개 = 문서 공식 그대로.
		// % 합산(10%+10%=20%)은 HealthPct 어트리뷰트가 Additive로 누적한 뒤 여기서 1회 곱한다 (§1).
		UGameplayEffect* PctEffect = NewObject<UGameplayEffect>(GetTransientPackage());
		PctEffect->DurationPolicy = EGameplayEffectDurationType::Infinite;

		auto AddMultiplier = [PctEffect](const FGameplayAttribute& Attribute, float Pct)
		{
			if (FMath::IsNearlyZero(Pct))
			{
				return;
			}
			FGameplayModifierInfo Modifier;
			Modifier.Attribute = Attribute;
			Modifier.ModifierOp = EGameplayModOp::MultiplyAdditive;
			Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.f + Pct / 100.f));
			PctEffect->Modifiers.Add(Modifier);
		};
		AddMultiplier(UROHAttributeSet::GetMaxHealthAttribute(), HealthPct);
		AddMultiplier(UROHAttributeSet::GetMaxManaAttribute(), ManaPct);

		VitalPctEffectHandle = AbilitySystemComponent->ApplyGameplayEffectToSelf(
			PctEffect, 1.f, AbilitySystemComponent->MakeEffectContext());
	}

	if (PreviousHandle.IsValid())
	{
		AbilitySystemComponent->RemoveActiveGameplayEffect(PreviousHandle);
	}
}

void AROHCharacterBase::InitializeAttributes()
{
	if (!AttributeSet)
	{
		return;
	}

	AttributeSet->InitStrength(BaseStrength);
	AttributeSet->InitDexterity(BaseDexterity);
	AttributeSet->InitVitality(BaseVitality);
	AttributeSet->InitEnergy(BaseEnergy);
	AttributeSet->InitCharacterLevel(BaseLevel);

	// 파생 공식 (docs/10 §2): HPmax = HPbase + (레벨-1)×HPPerLevel + VIT×5, MPmax = MPbase + INT×2
	// 레벨 성장분은 플레이어 프로그레션(LevelUp/RestoreState)과 몬스터 BaseLevel 프리셋 양쪽에서 일관 적용
	const float MaxHealth = BaseMaxHealth + (BaseLevel - 1.f) * HealthPerLevel + BaseVitality * 5.f;
	AttributeSet->InitMaxHealth(MaxHealth);
	AttributeSet->InitHealth(MaxHealth);

	const float MaxMana = BaseMaxMana + BaseEnergy * 2.f;
	AttributeSet->InitMaxMana(MaxMana);
	AttributeSet->InitMana(MaxMana);

	AttributeSet->InitRage(0.f);
	AttributeSet->InitAttackRating(BaseDexterity * 5.f);
	AttributeSet->InitDefense(BaseDexterity * 2.f);
	AttributeSet->InitMoveSpeed(BaseMoveSpeed);

	GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed;
}

void AROHCharacterBase::GrantDefaultAbilities()
{
	if (bAbilitiesGranted)
	{
		return;
	}
	bAbilitiesGranted = true;

	for (const TSubclassOf<UROHGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		GrantAbility(AbilityClass);
	}
}

FGameplayAbilitySpecHandle AROHCharacterBase::GrantAbility(TSubclassOf<UROHGameplayAbility> AbilityClass)
{
	if (!AbilitySystemComponent || !AbilityClass)
	{
		return FGameplayAbilitySpecHandle();
	}
	return AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
}

void AROHCharacterBase::HandleDamageTaken(float Damage, AActor* InstigatorActor)
{
	// M1: 피격 연출 훅 (추후 히트 플래시/사운드/데미지 텍스트)
}

void AROHCharacterBase::HandleDeath(AActor* Killer)
{
	if (!IsAlive())
	{
		return;
	}

	AbilitySystemComponent->AddLooseGameplayTag(ROHGameplayTags::State_Dead);
	AbilitySystemComponent->CancelAllAbilities();

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 그레이박스 사망 연출: 몸통을 눕힌다
	if (VisualMesh)
	{
		VisualMesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	}

	OnDeath.Broadcast(this);
}
