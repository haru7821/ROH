#include "World/ROHTownNpc.h"
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

	// 기둥 메시 로드 (생성자 시점엔 엔진 콘텐츠 미마운트 — 웨이포인트와 동일 사유)
	if (PillarMesh && !PillarMesh->GetStaticMesh())
	{
		if (UStaticMesh* Mesh = LoadNpcPillarMesh())
		{
			PillarMesh->SetStaticMesh(Mesh);
			// 웨이포인트(반경 30/높이 300)보다 낮은 사람 크기 기둥: 반경 30, 높이 220
			const FVector Extent = Mesh->GetBounds().BoxExtent;
			if (Extent.GetMin() > KINDA_SMALL_NUMBER)
			{
				PillarMesh->SetRelativeScale3D(FVector(30.f / Extent.X, 30.f / Extent.Y, 110.f / Extent.Z));
				PillarMesh->SetRelativeLocation(FVector(0.f, 0.f, 110.f));
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
