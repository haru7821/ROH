#include "World/ROHStashChest.h"
#include "Core/ROHAssetCatalog.h" // 카탈로그 메시 우선 적용 (b32)
#include "Materials/MaterialInterface.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "ROH.h"

namespace
{
	// 엔진 설치본마다 기본 메시 구성이 달라 후보를 순서대로 시도 (큐브 우선 — 상자 실루엣)
	UStaticMesh* LoadChestMesh()
	{
		static const TCHAR* CandidatePaths[] = {
			TEXT("/Engine/BasicShapes/Cube.Cube"),
			TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),
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

AROHStashChest::AROHStashChest()
{
	// 그레이박스: 링/접근 안내 표시용 틱
	PrimaryActorTick.bCanEverTick = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot")));

	ChestMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChestMesh"));
	ChestMesh->SetupAttachment(RootComponent);
	ChestMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AROHStashChest::BeginPlay()
{
	Super::BeginPlay();

	// 카탈로그(SM_StashChest) 우선, 없으면 기존 그레이박스 상자 그대로 (b32)
	if (ChestMesh && !ChestMesh->GetStaticMesh())
	{
		static const FName ChestCatalogKey(TEXT("SM_StashChest"));
		UStaticMesh* Mesh = ROHAssetCatalog::LoadMesh(ChestCatalogKey);
		const bool bCatalogMesh = Mesh != nullptr;
		if (!Mesh)
		{
			Mesh = LoadChestMesh();
		}

		if (Mesh)
		{
			ChestMesh->SetStaticMesh(Mesh);
			// 낮은 상자: 60×60×50 (기둥 NPC/웨이포인트와 실루엣 구분 — 카탈로그 메시도 동일 정규화)
			const FVector Extent = Mesh->GetBounds().BoxExtent;
			if (Extent.GetMin() > KINDA_SMALL_NUMBER)
			{
				ChestMesh->SetRelativeScale3D(FVector(30.f / Extent.X, 30.f / Extent.Y, 25.f / Extent.Z));
				ChestMesh->SetRelativeLocation(FVector(0.f, 0.f, 25.f));
			}
			if (bCatalogMesh)
			{
				if (UMaterialInterface* OverrideMaterial = ROHAssetCatalog::LoadMaterialForMeshKey(ChestCatalogKey))
				{
					ChestMesh->SetMaterial(0, OverrideMaterial);
				}
			}
		}
		else
		{
			UE_LOG(LogROH, Warning, TEXT("보관함 메시를 찾지 못했습니다 (디버그 링만 표시)"));
		}
	}
}

void AROHStashChest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 그레이박스: 금색 링
	DrawDebugCircle(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 20.f), 150.f,
		24, FColor(255, 200, 60), false, -1.f, 0, 4.f, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);

	// 접근 안내 (키 10 — NPC와 공유, 최근접이 덮어쓰는 간단 규칙 수용)
	if (GEngine)
	{
		const APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		if (Player && FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) < FMath::Square(300.f))
		{
			GEngine->AddOnScreenDebugMessage(10, 0.5f, FColor(255, 200, 60), TEXT("보관함 — E로 열기"));
		}
	}
}
