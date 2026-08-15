#include "World/ROHWaypoint.h"
#include "Campaign/ROHCampaignSubsystem.h"
#include "Character/ROHPlayerCharacter.h"
#include "Core/ROHAssetCatalog.h" // 카탈로그 메시 우선 적용 (b32)
#include "Materials/MaterialInterface.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h" // GetSubsystem<T>() 템플릿 인스턴스화에 완전한 타입 필요
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "ROH.h"

namespace
{
	// 엔진 설치본마다 기본 메시 구성이 달라 후보를 순서대로 시도 (CharacterBase와 동일 패턴)
	UStaticMesh* LoadWaypointPillarMesh()
	{
		static const TCHAR* CandidatePaths[] = {
			TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),
			TEXT("/Engine/BasicShapes/Cube.Cube"),
			TEXT("/Engine/EngineMeshes/Cylinder.Cylinder"),
		};
		for (const TCHAR* Path : CandidatePaths)
		{
			if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Path))
			{
				return Mesh;
			}
		}
		return nullptr;
	}
}

AROHWaypoint::AROHWaypoint()
{
	// 그레이박스: 발밑 링 디버그 표시용 틱
	PrimaryActorTick.bCanEverTick = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot")));

	PillarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PillarMesh"));
	PillarMesh->SetupAttachment(RootComponent);
	PillarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ActivationSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ActivationSphere"));
	ActivationSphere->SetupAttachment(RootComponent);
	ActivationSphere->InitSphereRadius(250.f);
	ActivationSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ActivationSphere->SetCollisionObjectType(ECC_WorldDynamic);
	ActivationSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	ActivationSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ActivationSphere->OnComponentBeginOverlap.AddDynamic(this, &AROHWaypoint::OnSphereOverlap);
}

void AROHWaypoint::SetZoneInfo(int32 InZoneIndex, const FText& InZoneName)
{
	ZoneIndex = InZoneIndex;
	ZoneName = InZoneName;
}

void AROHWaypoint::BeginPlay()
{
	Super::BeginPlay();

	// 기둥 메시 로드 (생성자 시점엔 콘텐츠 미마운트 — CharacterBase와 동일 사유):
	// 카탈로그(SM_Waypoint) 우선, 없으면 기존 그레이박스 기둥 그대로 (b32)
	if (PillarMesh && !PillarMesh->GetStaticMesh())
	{
		static const FName WaypointCatalogKey(TEXT("SM_Waypoint"));
		UStaticMesh* Mesh = ROHAssetCatalog::LoadMesh(WaypointCatalogKey);
		const bool bCatalogMesh = Mesh != nullptr;
		if (!Mesh)
		{
			Mesh = LoadWaypointPillarMesh();
		}

		if (Mesh)
		{
			PillarMesh->SetStaticMesh(Mesh);
			// 메시 종류와 무관하게 가늘고 긴 기둥으로 정규화 (반경 30, 높이 300 — 카탈로그 메시도 동일)
			const FVector Extent = Mesh->GetBounds().BoxExtent;
			if (Extent.GetMin() > KINDA_SMALL_NUMBER)
			{
				PillarMesh->SetRelativeScale3D(FVector(30.f / Extent.X, 30.f / Extent.Y, 150.f / Extent.Z));
				PillarMesh->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
			}
			if (bCatalogMesh)
			{
				if (UMaterialInterface* OverrideMaterial = ROHAssetCatalog::LoadMaterialForMeshKey(WaypointCatalogKey))
				{
					PillarMesh->SetMaterial(0, OverrideMaterial);
				}
			}
		}
		else
		{
			// 안전망: 메시 없이도 틱의 디버그 링이 위치를 표시한다
			UE_LOG(LogROH, Warning, TEXT("웨이포인트 기둥 메시를 찾지 못했습니다 (디버그 링만 표시)"));
		}
	}
}

void AROHWaypoint::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 그레이박스: 활성 = 파란 링, 미활성 = 회색 링
	const FColor RingColor = IsActivated() ? FColor(60, 140, 255) : FColor(110, 110, 110);
	DrawDebugCircle(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 20.f), 200.f,
		32, RingColor, false, -1.f, 0, 4.f, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
}

bool AROHWaypoint::IsActivated() const
{
	const UROHCampaignSubsystem* Campaign = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UROHCampaignSubsystem>() : nullptr;
	return Campaign && Campaign->IsWaypointActivated(ZoneIndex);
}

void AROHWaypoint::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!Cast<AROHPlayerCharacter>(OtherActor))
	{
		return;
	}

	UROHCampaignSubsystem* Campaign = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UROHCampaignSubsystem>() : nullptr;
	if (Campaign && Campaign->ActivateWaypoint(ZoneIndex))
	{
		UE_LOG(LogROH, Log, TEXT("웨이포인트 활성화: %d (%s)"), ZoneIndex, *ZoneName.ToString());
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Green,
				FString::Printf(TEXT("웨이포인트 활성화: %s (E로 이동)"), *ZoneName.ToString()));
		}
	}
}
