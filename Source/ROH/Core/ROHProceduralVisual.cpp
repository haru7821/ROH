#include "Core/ROHProceduralVisual.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	// 기본 도형 로드 (BeginPlay 이후 호출 전제 — 엔진 콘텐츠 마운트 완료 시점)
	UStaticMesh* LoadBasicShapeMesh(ROHProceduralVisual::EROHBasicShape Shape)
	{
		const TCHAR* Path = nullptr;
		switch (Shape)
		{
		case ROHProceduralVisual::EROHBasicShape::Cube:     Path = TEXT("/Engine/BasicShapes/Cube.Cube"); break;
		case ROHProceduralVisual::EROHBasicShape::Sphere:   Path = TEXT("/Engine/BasicShapes/Sphere.Sphere"); break;
		case ROHProceduralVisual::EROHBasicShape::Cylinder: Path = TEXT("/Engine/BasicShapes/Cylinder.Cylinder"); break;
		case ROHProceduralVisual::EROHBasicShape::Cone:     Path = TEXT("/Engine/BasicShapes/Cone.Cone"); break;
		default: return nullptr;
		}
		return LoadObject<UStaticMesh>(nullptr, Path);
	}

	// 살구색 (사람/몬스터 머리 공용)
	const FLinearColor ProceduralSkinColor(1.f, 0.76f, 0.60f);
	const FLinearColor ProceduralMonsterHeadColor(0.90f, 0.55f, 0.45f);
}

UMaterialInstanceDynamic* ROHProceduralVisual::ApplyColor(UStaticMeshComponent* MeshComponent, const FLinearColor& Color)
{
	if (!MeshComponent)
	{
		return nullptr;
	}
	// BasicShapeMaterial의 벡터 파라미터명은 "Color" (엔진 기본 도형 전용 파라미터화 머티리얼).
	// 로드 실패 시 색 없이 진행 — 파라미터명이 달라도 SetVectorParameterValue는 조용히 무시된다.
	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!BaseMaterial)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(BaseMaterial, MeshComponent);
	if (!Mid)
	{
		return nullptr;
	}
	Mid->SetVectorParameterValue(TEXT("Color"), Color);
	MeshComponent->SetMaterial(0, Mid);
	return Mid;
}

UStaticMeshComponent* ROHProceduralVisual::AddPart(AActor& Owner, USceneComponent& AttachParent, EROHBasicShape Shape,
	const FVector& RelativeLocation, const FRotator& RelativeRotation, const FVector& PartSize, const FLinearColor& Color)
{
	UStaticMesh* ShapeMesh = LoadBasicShapeMesh(Shape);
	if (!ShapeMesh)
	{
		return nullptr; // 엔진 도형 부재 — 호출측이 그레이박스 안전망 판단
	}

	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(&Owner);
	if (!Part)
	{
		return nullptr;
	}
	Part->SetStaticMesh(ShapeMesh);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 시각 전용 — 판정은 캡슐/루트
	Part->SetGenerateOverlapEvents(false);
	Part->RegisterComponent();
	Part->AttachToComponent(&AttachParent, FAttachmentTransformRules::SnapToTargetIncludingScale);
	Part->SetRelativeLocation(RelativeLocation);
	Part->SetRelativeRotation(RelativeRotation);
	Part->SetRelativeScale3D(PartSize / 100.f); // 기본 도형 = 100cm

	ApplyColor(Part, Color);
	return Part;
}

bool ROHProceduralVisual::BuildCharacterVisual(AActor& Owner, USceneComponent& AttachParent, FName CatalogKey,
	float CapsuleRadius, float CapsuleHalfHeight)
{
	const float R = FMath::Max(CapsuleRadius, 10.f);
	const float H = FMath::Max(CapsuleHalfHeight, 20.f);
	int32 BuiltCount = 0;

	// 파트 헬퍼: 좌표계는 캡슐 중심 원점 (발 = -H, 정수리 = +H)
	auto Part = [&Owner, &AttachParent, &BuiltCount](EROHBasicShape Shape, const FVector& Location,
		const FRotator& Rotation, const FVector& Size, const FLinearColor& Color)
	{
		UStaticMeshComponent* Built = AddPart(Owner, AttachParent, Shape, Location, Rotation, Size, Color);
		if (Built)
		{
			++BuiltCount;
		}
		return Built;
	};
	// 표준 휴머노이드 (몸통 원기둥 + 머리 구)
	auto Humanoid = [&Part, R, H](float WidthMult, float HeadZMult, const FLinearColor& BodyColor, const FLinearColor& HeadColor)
	{
		Part(EROHBasicShape::Cylinder, FVector(0.f, 0.f, -0.3f * H), FRotator::ZeroRotator,
			FVector(1.7f * R * WidthMult, 1.7f * R * WidthMult, 1.4f * H), BodyColor);
		Part(EROHBasicShape::Sphere, FVector(0.f, 0.f, HeadZMult * H), FRotator::ZeroRotator,
			FVector(1.2f * R, 1.2f * R, 1.2f * R), HeadColor);
	};

	if (CatalogKey == TEXT("SM_Player_Warrior"))
	{
		// 전사: 강철 회색 몸통 + 머리 + 등의 세로 대검 (전방 +X → 등 = -X)
		Humanoid(1.f, 0.6f, FLinearColor(0.35f, 0.37f, 0.42f), ProceduralSkinColor);
		Part(EROHBasicShape::Cube, FVector(-R - 10.f, 0.f, 0.1f * H), FRotator::ZeroRotator,
			FVector(10.f, 28.f, 1.6f * H), FLinearColor(0.60f, 0.62f, 0.68f));
	}
	else if (CatalogKey == TEXT("SM_Player_Elementalist"))
	{
		// 원소술사: 보라 원뿔 로브 + 머리 + 오른손 지팡이 + 끝 구슬 (청록 — 발광 강등: 색만)
		Part(EROHBasicShape::Cone, FVector(0.f, 0.f, -0.25f * H), FRotator::ZeroRotator,
			FVector(2.6f * R, 2.6f * R, 1.5f * H), FLinearColor(0.40f, 0.15f, 0.70f));
		Part(EROHBasicShape::Sphere, FVector(0.f, 0.f, 0.6f * H), FRotator::ZeroRotator,
			FVector(1.2f * R, 1.2f * R, 1.2f * R), ProceduralSkinColor);
		Part(EROHBasicShape::Cylinder, FVector(0.f, R + 12.f, -0.05f * H), FRotator::ZeroRotator,
			FVector(6.f, 6.f, 1.3f * H), FLinearColor(0.35f, 0.20f, 0.08f));
		Part(EROHBasicShape::Sphere, FVector(0.f, R + 12.f, 0.62f * H), FRotator::ZeroRotator,
			FVector(18.f, 18.f, 18.f), FLinearColor(0.10f, 0.90f, 0.95f));
	}
	else if (CatalogKey == TEXT("SM_Monster_Grunt"))
	{
		// 졸개: 기본 휴머노이드
		Humanoid(1.f, 0.6f, FLinearColor(0.55f, 0.10f, 0.10f), ProceduralMonsterHeadColor);
	}
	else if (CatalogKey == TEXT("SM_Monster_Archer"))
	{
		// 사수: 얇은 몸통 + 등 뒤 세로 활 (얇은 박스)
		Humanoid(0.75f, 0.6f, FLinearColor(0.70f, 0.28f, 0.16f), ProceduralMonsterHeadColor);
		Part(EROHBasicShape::Cube, FVector(-R - 8.f, 0.f, 0.1f * H), FRotator::ZeroRotator,
			FVector(6.f, 8.f, 1.2f * H), FLinearColor(0.30f, 0.16f, 0.06f));
	}
	else if (CatalogKey == TEXT("SM_Monster_Charger"))
	{
		// 돌격병: 몸통 전방 15° 기울임 + 이마 뿔 원뿔
		Part(EROHBasicShape::Cylinder, FVector(0.1f * H, 0.f, -0.3f * H), FRotator(-15.f, 0.f, 0.f),
			FVector(1.7f * R, 1.7f * R, 1.4f * H), FLinearColor(0.80f, 0.16f, 0.10f));
		Part(EROHBasicShape::Sphere, FVector(0.25f * H, 0.f, 0.55f * H), FRotator::ZeroRotator,
			FVector(1.2f * R, 1.2f * R, 1.2f * R), ProceduralMonsterHeadColor);
		Part(EROHBasicShape::Cone, FVector(0.25f * H + 0.7f * R, 0.f, 0.65f * H), FRotator(-50.f, 0.f, 0.f),
			FVector(18.f, 18.f, 40.f), FLinearColor(0.85f, 0.80f, 0.70f));
	}
	else if (CatalogKey == TEXT("SM_Monster_Brute") || CatalogKey == TEXT("SM_Boss_Baltar"))
	{
		// 덩치: 폭 1.5배 + 짧은 목(머리 낮게). 발타르 = 같은 골격(큰 캡슐로 자동 확대) + 어깨 뿔 2
		Humanoid(1.5f, 0.45f, CatalogKey == TEXT("SM_Boss_Baltar")
			? FLinearColor(0.30f, 0.03f, 0.03f) : FLinearColor(0.38f, 0.05f, 0.05f), ProceduralMonsterHeadColor);
		if (CatalogKey == TEXT("SM_Boss_Baltar"))
		{
			Part(EROHBasicShape::Cone, FVector(0.f, 1.2f * R, 0.35f * H), FRotator(0.f, 0.f, 30.f),
				FVector(24.f, 24.f, 60.f), FLinearColor(0.15f, 0.15f, 0.15f));
			Part(EROHBasicShape::Cone, FVector(0.f, -1.2f * R, 0.35f * H), FRotator(0.f, 0.f, -30.f),
				FVector(24.f, 24.f, 60.f), FLinearColor(0.15f, 0.15f, 0.15f));
		}
	}
	else if (CatalogKey == TEXT("SM_Monster_Hexer") || CatalogKey == TEXT("SM_Boss_Morgath"))
	{
		// 주술사: 원뿔 로브 + 머리 + 뾰족 모자. 모르가스 = 같은 골격 + 양옆 부유 구슬 2 (화염 주황)
		const bool bMorgath = CatalogKey == TEXT("SM_Boss_Morgath");
		Part(EROHBasicShape::Cone, FVector(0.f, 0.f, -0.25f * H), FRotator::ZeroRotator,
			FVector(2.4f * R, 2.4f * R, 1.5f * H), bMorgath
			? FLinearColor(0.42f, 0.08f, 0.30f) : FLinearColor(0.50f, 0.12f, 0.35f));
		Part(EROHBasicShape::Sphere, FVector(0.f, 0.f, 0.55f * H), FRotator::ZeroRotator,
			FVector(1.2f * R, 1.2f * R, 1.2f * R), ProceduralMonsterHeadColor);
		Part(EROHBasicShape::Cone, FVector(0.f, 0.f, 0.85f * H), FRotator::ZeroRotator,
			FVector(1.4f * R, 1.4f * R, 0.9f * H), bMorgath
			? FLinearColor(0.30f, 0.05f, 0.22f) : FLinearColor(0.35f, 0.08f, 0.25f));
		if (bMorgath)
		{
			Part(EROHBasicShape::Sphere, FVector(0.f, R + 36.f, 0.3f * H), FRotator::ZeroRotator,
				FVector(24.f, 24.f, 24.f), FLinearColor(1.f, 0.45f, 0.05f));
			Part(EROHBasicShape::Sphere, FVector(0.f, -R - 36.f, 0.3f * H), FRotator::ZeroRotator,
				FVector(24.f, 24.f, 24.f), FLinearColor(1.f, 0.45f, 0.05f));
		}
	}
	else if (CatalogKey == TEXT("SM_Monster_Stalker"))
	{
		// 추적자: 낮고 긴 수평 몸통 + 전방 머리 + 후방 꼬리 원뿔
		Part(EROHBasicShape::Cylinder, FVector(0.f, 0.f, -0.45f * H), FRotator(90.f, 0.f, 0.f),
			FVector(1.4f * R, 1.4f * R, 2.2f * H), FLinearColor(0.62f, 0.20f, 0.20f));
		Part(EROHBasicShape::Sphere, FVector(1.1f * H, 0.f, -0.35f * H), FRotator::ZeroRotator,
			FVector(R, R, R), ProceduralMonsterHeadColor);
		Part(EROHBasicShape::Cone, FVector(-1.2f * H, 0.f, -0.45f * H), FRotator(-100.f, 0.f, 0.f),
			FVector(0.8f * R, 0.8f * R, 0.9f * H), FLinearColor(0.45f, 0.14f, 0.14f));
	}

	return BuiltCount > 0;
}
