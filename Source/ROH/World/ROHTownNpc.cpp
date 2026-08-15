#include "World/ROHTownNpc.h"
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
	// 엔진 설치본마다 기본 메시 구성이 달라 후보를 순서대로 시도 (웨이포인트와 동일 패턴)
	UStaticMesh* LoadNpcPillarMesh()
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

	// 역할 → 카탈로그 키 (b32, docs/13 — enum 이름 기반 표기)
	FName NpcCatalogMeshKey(EROHNpcRole Role)
	{
		switch (Role)
		{
		case EROHNpcRole::General:      return TEXT("SM_Npc_General");
		case EROHNpcRole::Blacksmith:   return TEXT("SM_Npc_Blacksmith");
		case EROHNpcRole::Jeweler:      return TEXT("SM_Npc_Jeweler");
		case EROHNpcRole::PotionVendor: return TEXT("SM_Npc_PotionVendor");
		case EROHNpcRole::Gambler:      return TEXT("SM_Npc_Gambler");
		case EROHNpcRole::Identifier:   return TEXT("SM_Npc_Identifier");
		default:                        return NAME_None;
		}
	}
}

AROHTownNpc::AROHTownNpc()
{
	// 그레이박스: 링/접근 안내 표시용 틱
	PrimaryActorTick.bCanEverTick = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot")));

	PillarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PillarMesh"));
	PillarMesh->SetupAttachment(RootComponent);
	PillarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AROHTownNpc::SetNpcInfo(EROHNpcRole InRole, const FText& InDisplayName, const FText& InGreeting)
{
	NpcRole = InRole;
	DisplayName = InDisplayName;
	Greeting = InGreeting;
}

FText AROHTownNpc::GetRoleLabel() const
{
	switch (NpcRole)
	{
	case EROHNpcRole::General:      return NSLOCTEXT("ROH", "NpcRoleGeneral", "잡화상");
	case EROHNpcRole::Blacksmith:   return NSLOCTEXT("ROH", "NpcRoleBlacksmith", "대장장이");
	case EROHNpcRole::Jeweler:      return NSLOCTEXT("ROH", "NpcRoleJeweler", "보석상");
	case EROHNpcRole::PotionVendor: return NSLOCTEXT("ROH", "NpcRolePotion", "포션상인");
	case EROHNpcRole::Gambler:      return NSLOCTEXT("ROH", "NpcRoleGambler", "도박사");
	case EROHNpcRole::Identifier:   return NSLOCTEXT("ROH", "NpcRoleIdentifier", "식별 주술사");
	default:                        return FText::GetEmpty();
	}
}

FColor AROHTownNpc::GetRoleColor() const
{
	// 등급 팔레트와 결 맞춤: 로사 흰 / 브란 주황 / 일렌 파랑 / 미로 초록 / 카론 금 / 셀바 자주
	switch (NpcRole)
	{
	case EROHNpcRole::Blacksmith:   return FColor(255, 140, 40);
	case EROHNpcRole::Jeweler:      return FColor(100, 150, 255);
	case EROHNpcRole::PotionVendor: return FColor(80, 220, 80);
	case EROHNpcRole::Gambler:      return FColor(255, 200, 60);
	case EROHNpcRole::Identifier:   return FColor(160, 60, 220);
	default:                        return FColor::White;
	}
}

void AROHTownNpc::BeginPlay()
{
	Super::BeginPlay();

	// 기둥 메시 로드 (생성자 시점엔 콘텐츠 미마운트 — 웨이포인트와 동일 사유):
	// 역할별 카탈로그(SM_Npc_<역할>) 우선, 없으면 기존 그레이박스 기둥 그대로 (b32).
	// 역할은 지연 스폰 중(FinishSpawning 전) SetNpcInfo로 주입되므로 BeginPlay 시점엔 확정 상태.
	if (PillarMesh && !PillarMesh->GetStaticMesh())
	{
		const FName NpcCatalogKey = NpcCatalogMeshKey(NpcRole);
		UStaticMesh* Mesh = ROHAssetCatalog::LoadMesh(NpcCatalogKey);
		const bool bCatalogMesh = Mesh != nullptr;

		// 프로시저럴 조형 (b33): 기둥 대신 사람 실루엣 — 몸통(역할색)+머리(살구)+로브 어깨(역할색 어둡게).
		// 이름/역할/링 표시 로직은 무변경 (틱이 계속 담당)
		if (!bCatalogMesh)
		{
			using namespace ROHProceduralVisual;
			const FLinearColor RoleColor = FLinearColor::FromSRGBColor(GetRoleColor());
			UStaticMeshComponent* BodyPart = AddPart(*this, *PillarMesh, EROHBasicShape::Cylinder,
				FVector(0.f, 0.f, 65.f), FRotator::ZeroRotator, FVector(56.f, 56.f, 130.f), RoleColor);
			if (BodyPart)
			{
				AddPart(*this, *PillarMesh, EROHBasicShape::Cone,
					FVector(0.f, 0.f, 128.f), FRotator::ZeroRotator, FVector(72.f, 72.f, 50.f),
					RoleColor * 0.45f); // 로브 어깨 (역할색 어둡게)
				AddPart(*this, *PillarMesh, EROHBasicShape::Sphere,
					FVector(0.f, 0.f, 172.f), FRotator::ZeroRotator, FVector(42.f, 42.f, 42.f),
					FLinearColor(1.f, 0.76f, 0.60f)); // 살구색 머리
				return; // 조형 성공 — 단일 기둥 메시는 만들지 않는다
			}
			// 몸통(핵심 파트) 실패 시 아래 그레이박스 안전망으로
		}

		if (!Mesh)
		{
			Mesh = LoadNpcPillarMesh();
		}

		if (Mesh)
		{
			PillarMesh->SetStaticMesh(Mesh);
			// 웨이포인트(반경 30/높이 300)보다 낮은 사람 크기 기둥: 반경 30, 높이 220 — 카탈로그 메시도 동일
			const FVector Extent = Mesh->GetBounds().BoxExtent;
			if (Extent.GetMin() > KINDA_SMALL_NUMBER)
			{
				PillarMesh->SetRelativeScale3D(FVector(30.f / Extent.X, 30.f / Extent.Y, 110.f / Extent.Z));
				PillarMesh->SetRelativeLocation(FVector(0.f, 0.f, 110.f));
			}
			if (bCatalogMesh)
			{
				if (UMaterialInterface* OverrideMaterial = ROHAssetCatalog::LoadMaterialForMeshKey(NpcCatalogKey))
				{
					PillarMesh->SetMaterial(0, OverrideMaterial);
				}
			}
		}
		else
		{
			UE_LOG(LogROH, Warning, TEXT("NPC 기둥 메시를 찾지 못했습니다 (디버그 링만 표시)"));
		}
	}
}

void AROHTownNpc::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 그레이박스: 역할 색 링
	DrawDebugCircle(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 20.f), 150.f,
		24, GetRoleColor(), false, -1.f, 0, 4.f, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);

	// 접근 안내 (반경 300 — Interact 사거리와 동일해 겹침 최소화. NPC끼리 겹치면 나중 틱이 덮어씀: 간단 우선)
	if (GEngine)
	{
		const APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		if (Player && FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) < FMath::Square(300.f))
		{
			GEngine->AddOnScreenDebugMessage(10, 0.5f, GetRoleColor(),
				FString::Printf(TEXT("%s (%s) — E로 대화"), *DisplayName.ToString(), *GetRoleLabel().ToString()));
		}
	}
}
