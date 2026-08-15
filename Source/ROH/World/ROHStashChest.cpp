#include "World/ROHStashChest.h"
#include "Core/ROHAssetCatalog.h" // 카탈로그 메시 우선 적용 (b32)
#include "Core/ROHProceduralVisual.h" // 프로시저럴 조형 (b33)
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

	// 우선순위 (b33): 카탈로그(SM_StashChest) → 프로시저럴 조형(몸체+뚜껑+금띠) → 그레이박스 상자
	if (ChestMesh && !ChestMesh->GetStaticMesh())
	{
		static const FName ChestCatalogKey(TEXT("SM_StashChest"));
		UStaticMesh* Mesh = ROHAssetCatalog::LoadMesh(ChestCatalogKey);
		const bool bCatalogMesh = Mesh != nullptr;

		// 프로시저럴 조형 (b33): ChestMesh를 빈 컨테이너로 쓰고 파트 부착 (노랑 계열 — 미니맵 톤 일치)
		if (!bCatalogMesh)
		{
			using namespace ROHProceduralVisual;
			UStaticMeshComponent* BodyPart = AddPart(*this, *ChestMesh, EROHBasicShape::Cube,
				FVector(0.f, 0.f, 28.f), FRotator::ZeroRotator, FVector(90.f, 70.f, 56.f),
				FLinearColor(0.32f, 0.18f, 0.07f)); // 짙은 갈색 몸체
			if (BodyPart)
			{
				AddPart(*this, *ChestMesh, EROHBasicShape::Cube,
					FVector(-4.f, 0.f, 62.f), FRotator(-10.f, 0.f, 0.f), FVector(94.f, 74.f, 18.f),
					FLinearColor(0.42f, 0.25f, 0.10f)); // 살짝 기울인 뚜껑
				AddPart(*this, *ChestMesh, EROHBasicShape::Cube,
					FVector(44.f, 0.f, 30.f), FRotator::ZeroRotator, FVector(6.f, 72.f, 14.f),
					FLinearColor(1.00f, 0.78f, 0.20f)); // 앞면 가로 금색 띠
				return; // 조형 성공 — 단일 상자 메시는 만들지 않는다 (링/안내 틱은 계속 담당)
			}
			// 몸체(핵심 파트) 실패 시 아래 그레이박스 안전망으로
		}

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
