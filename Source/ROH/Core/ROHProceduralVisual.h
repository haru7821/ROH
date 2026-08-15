#pragma once

#include "CoreMinimal.h"

class AActor;
class USceneComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * 프로시저럴 비주얼 (b33 — 코드로 만드는 "애셋"):
 * 엔진 기본 도형(/Engine/BasicShapes)을 다중 컴포넌트로 조합해 실루엣 있는 조형을 만든다.
 * 임포트 없이 update.bat만으로 시각 품질을 올리는 축 — 우선순위는
 * 카탈로그 애셋(있으면) → 프로시저럴 조형(기본값) → 단순 그레이박스(도형 로드 실패 안전망).
 * 전 파트 시각 전용 (NoCollision — 게임플레이 판정은 기존 캡슐/루트 담당).
 */
namespace ROHProceduralVisual
{
	/** 기본 도형 (/Engine/BasicShapes — 100cm 기준 크기) */
	enum class EROHBasicShape : uint8
	{
		Cube,
		Sphere,
		Cylinder,
		Cone
	};

	/**
	 * 색 적용: BasicShapeMaterial 기반 MID 생성 후 "Color" 벡터 파라미터 설정, 슬롯 0 지정.
	 * 머티리얼 로드 실패 시 nullptr — 색 없이 형태만 (우아한 강등, 로그 스팸 없음).
	 * 반환 MID는 런타임 재채색용 (웨이포인트 크리스탈 활성 전환 등).
	 */
	ROH_API UMaterialInstanceDynamic* ApplyColor(UStaticMeshComponent* MeshComponent, const FLinearColor& Color);

	/**
	 * 시각 전용 자식 파트 생성 + 등록 + 부착(SnapToTarget) + 상대 트랜스폼 + 색.
	 * PartSize는 cm (기본 도형이 100cm라 내부에서 /100 스케일). 도형 로드 실패 시 nullptr.
	 * BeginPlay 시점(콘텐츠 마운트 후)에만 호출할 것.
	 */
	ROH_API UStaticMeshComponent* AddPart(AActor& Owner, USceneComponent& AttachParent, EROHBasicShape Shape,
		const FVector& RelativeLocation, const FRotator& RelativeRotation, const FVector& PartSize, const FLinearColor& Color);

	/**
	 * 캐릭터 조형 조립 (b32 카탈로그 키 기반 — 키가 곧 종족/직업 식별자).
	 * AttachParent(비어 있는 VisualMesh 컨테이너) 아래에 파트를 부착한다 —
	 * HandleDeath의 눕히기 회전이 조형 전체에 그대로 적용된다.
	 * 파트를 1개도 못 만들면 false (호출측은 기존 그레이박스 폴백).
	 */
	ROH_API bool BuildCharacterVisual(AActor& Owner, USceneComponent& AttachParent, FName CatalogKey,
		float CapsuleRadius, float CapsuleHalfHeight);
}
