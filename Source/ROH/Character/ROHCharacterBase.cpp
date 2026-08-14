#include "Character/ROHCharacterBase.h"
#include "Abilities/ROHAbilitySystemComponent.h"
#include "Abilities/ROHGameplayAbility.h"
#include "Character/ROHAttributeSet.h"
#include "Character/ROHMonsterCharacter.h"
#include "ROHGameplayTags.h"
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

	// 그레이박스 비주얼 메시 로드 (생성자 시점엔 엔진 콘텐츠 미마운트)
	if (VisualMesh && !VisualMesh->GetStaticMesh())
	{
		if (UStaticMesh* BodyMesh = LoadGrayboxBodyMesh())
		{
			VisualMesh->SetStaticMesh(BodyMesh);
			// 메시 종류와 무관하게 캡슐 크기에 맞춰 정규화 (바닥 파묻힘 방지)
			const FVector Extent = BodyMesh->GetBounds().BoxExtent;
			const float TargetHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			if (Extent.GetMin() > KINDA_SMALL_NUMBER)
			{
				VisualMesh->SetRelativeScale3D(FVector(40.f / Extent.X, 40.f / Extent.Y, TargetHalfHeight / Extent.Z));
			}
		}
		else
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
	}
}

void AROHCharacterBase::OnMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	GetCharacterMovement()->MaxWalkSpeed = Data.NewValue;
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

	// 파생 공식 (docs/02 §1.3) — M3에서 레벨업/장비 반영 시 GameplayEffect로 이관
	const float MaxHealth = BaseMaxHealth + BaseVitality * 4.f;
	AttributeSet->InitMaxHealth(MaxHealth);
	AttributeSet->InitHealth(MaxHealth);

	const float MaxMana = 20.f + BaseEnergy * 2.f;
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
