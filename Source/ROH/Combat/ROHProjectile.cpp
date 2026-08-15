#include "Combat/ROHProjectile.h"
#include "Character/ROHCharacterBase.h"
#include "Core/ROHAssetCatalog.h" // 카탈로그 메시 우선 적용 (b32)
#include "Materials/MaterialInterface.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/StaticMesh.h"
#include "DrawDebugHelpers.h"

namespace
{
	/**
	 * 피해 구성 → 카탈로그 키 (b32): 최대 비중 원소 기준.
	 * 화염/냉기/번개/그림자만 전용 키 — 물리/독은 공용 SM_Projectile로 (현 콘텐츠에 독 투사체 없음).
	 */
	FName ProjectileCatalogMeshKey(const FROHDamageParams& Damage)
	{
		FName BestKey = NAME_None;
		float BestValue = 0.f;
		auto Consider = [&BestKey, &BestValue](float Value, FName Key)
		{
			if (Value > BestValue)
			{
				BestValue = Value;
				BestKey = Key;
			}
		};
		Consider(Damage.FireDamage, TEXT("SM_Projectile_Fire"));
		Consider(Damage.ColdDamage, TEXT("SM_Projectile_Ice"));
		Consider(Damage.LightningDamage, TEXT("SM_Projectile_Lightning"));
		Consider(Damage.ShadowDamage, TEXT("SM_Projectile_Shadow"));
		return BestKey; // 원소 피해가 없으면 None → 공용 키만 시도
	}
}

AROHProjectile::AROHProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	InitialLifeSpan = 3.f;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(15.f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	SetRootComponent(CollisionSphere);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(CollisionSphere);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetRelativeScale3D(FVector(0.25f));

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bRotationFollowsVelocity = true;

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AROHProjectile::OnSphereOverlap);
}

void AROHProjectile::BeginPlay()
{
	Super::BeginPlay();

	// 생성자 시점엔 엔진 콘텐츠가 마운트되지 않아 여기서 로드 (후보 순차 시도)
	if (VisualMesh && !VisualMesh->GetStaticMesh())
	{
		static const TCHAR* CandidatePaths[] = {
			TEXT("/Engine/BasicShapes/Sphere.Sphere"),
			TEXT("/Engine/EngineMeshes/Sphere.Sphere"),
		};
		UStaticMesh* SphereMesh = nullptr;
		for (const TCHAR* Path : CandidatePaths)
		{
			SphereMesh = LoadObject<UStaticMesh>(nullptr, Path);
			if (SphereMesh)
			{
				break;
			}
		}

		if (SphereMesh)
		{
			VisualMesh->SetStaticMesh(SphereMesh);
			const FVector Extent = SphereMesh->GetBounds().BoxExtent;
			if (Extent.GetMin() > KINDA_SMALL_NUMBER)
			{
				VisualMesh->SetRelativeScale3D(FVector(15.f / Extent.X, 15.f / Extent.Y, 15.f / Extent.Z));
			}
		}
		else
		{
			CollisionSphere->SetHiddenInGame(false);
		}
	}
}

void AROHProjectile::InitProjectile(AROHCharacterBase* InSource, const FROHDamageParams& InDamage, float Speed, float InExplosionRadius)
{
	Source = InSource;
	DamageParams = InDamage;
	ExplosionRadius = InExplosionRadius;
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->Velocity = GetActorForwardVector() * Speed;

	// 카탈로그 메시 (b32): 피해 유형은 여기서 확정되므로 BeginPlay(그레이박스)가 아닌 이 시점에 교체.
	// 속성별 키(화염/냉기/번개/그림자) 우선 → 공용 SM_Projectile → 없으면 그레이박스 구체 유지.
	if (VisualMesh)
	{
		FName ProjectileCatalogKey = ProjectileCatalogMeshKey(DamageParams);
		UStaticMesh* CatalogMesh = ROHAssetCatalog::LoadMesh(ProjectileCatalogKey);
		if (!CatalogMesh)
		{
			ProjectileCatalogKey = TEXT("SM_Projectile");
			CatalogMesh = ROHAssetCatalog::LoadMesh(ProjectileCatalogKey);
		}
		if (CatalogMesh)
		{
			VisualMesh->SetStaticMesh(CatalogMesh);
			// 기존 그레이박스와 동일 정규화 (반경 15 — 콜리전 구와 실루엣 일치)
			const FVector Extent = CatalogMesh->GetBounds().BoxExtent;
			if (Extent.GetMin() > KINDA_SMALL_NUMBER)
			{
				VisualMesh->SetRelativeScale3D(FVector(15.f / Extent.X, 15.f / Extent.Y, 15.f / Extent.Z));
			}
			if (UMaterialInterface* OverrideMaterial = ROHAssetCatalog::LoadMaterialForMeshKey(ProjectileCatalogKey))
			{
				VisualMesh->SetMaterial(0, OverrideMaterial);
			}
		}
	}
}

void AROHProjectile::Detonate(const FVector& Location)
{
	if (ExplosionRadius > 0.f && Source.IsValid())
	{
		DrawDebugSphere(GetWorld(), Location, ExplosionRadius, 16, FColor::Orange, false, 0.3f);
		for (AROHCharacterBase* Target : UROHCombatStatics::GetHostileTargetsInRadius(Source.Get(), Location, ExplosionRadius))
		{
			UROHCombatStatics::ApplyDamage(Source.Get(), Target, DamageParams);
		}
	}
	Destroy();
}

void AROHProjectile::OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this || OtherActor == GetInstigator())
	{
		return;
	}

	if (AROHCharacterBase* HitCharacter = Cast<AROHCharacterBase>(OtherActor))
	{
		// 아군(같은 팀)은 통과
		if (!Source.IsValid() || HitCharacter->GetTeamId() == Source->GetTeamId() || !HitCharacter->IsAlive())
		{
			return;
		}
		if (ExplosionRadius > 0.f)
		{
			// 광역형: 직격 대상 포함 폭발 반경으로 일괄 처리
			Detonate(GetActorLocation());
		}
		else
		{
			UROHCombatStatics::ApplyDamage(Source.Get(), HitCharacter, DamageParams);
			Destroy();
		}
	}
	else if (OtherComp && OtherComp->GetCollisionObjectType() == ECC_WorldStatic)
	{
		// 벽/지형에 막힘 (광역형은 그 자리에서 폭발)
		Detonate(GetActorLocation());
	}
}
