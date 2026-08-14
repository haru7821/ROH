#include "Combat/ROHProjectile.h"
#include "Character/ROHCharacterBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

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
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(SphereMesh.Object);
		VisualMesh->SetRelativeScale3D(FVector(0.25f));
	}

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bRotationFollowsVelocity = true;

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AROHProjectile::OnSphereOverlap);
}

void AROHProjectile::InitProjectile(AROHCharacterBase* InSource, const FROHDamageParams& InDamage, float Speed)
{
	Source = InSource;
	DamageParams = InDamage;
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->Velocity = GetActorForwardVector() * Speed;
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
		UROHCombatStatics::ApplyDamage(Source.Get(), HitCharacter, DamageParams);
		Destroy();
	}
	else if (OtherComp && OtherComp->GetCollisionObjectType() == ECC_WorldStatic)
	{
		// 벽/지형에 막힘
		Destroy();
	}
}
