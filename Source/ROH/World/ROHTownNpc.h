#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ROHTownNpc.generated.h"

class UStaticMeshComponent;

/** 마을 NPC 역할 (docs/12 — 액트1 재의 마을 6인) */
UENUM()
enum class EROHNpcRole : uint8
{
	General,      // 잡화상 로사: 매입 + 일반 장비 판매
	Blacksmith,   // 대장장이 브란: 마법 등급 장비 (방문마다 갱신)
	Jeweler,      // 보석상 일렌: 하급 룬 판매/매입
	PotionVendor, // 포션상인 미로: 치유물약
	Gambler,      // 도박사 카론: 운명의 보석 도박
	Identifier    // 식별 주술사 셀바: 미감정 감정
};

/**
 * 마을 NPC (docs/12): 웨이포인트와 같은 그레이박스 기둥 골격 (조금 낮게) + 역할별 링 색.
 * 접근만으로는 아무 일 없음 — 상호작용은 플레이어 Interact 체인(E)에서 벤더 창을 연다.
 * 이름/역할/인사말은 지역 매니저가 스폰 시 주입 (docs/12 로스터가 사양).
 */
UCLASS()
class ROH_API AROHTownNpc : public AActor
{
	GENERATED_BODY()

public:
	AROHTownNpc();

	virtual void Tick(float DeltaSeconds) override;

	/** 지역 매니저가 지연 스폰 중(FinishSpawning 전) 주입 */
	void SetNpcInfo(EROHNpcRole InRole, const FText& InDisplayName, const FText& InGreeting);

	EROHNpcRole GetNpcRole() const { return NpcRole; }
	FText GetDisplayName() const { return DisplayName; }
	FText GetGreeting() const { return Greeting; }

	/** 역할 표기 ("대장장이" 등 — 창 타이틀/접근 메시지 공용) */
	FText GetRoleLabel() const;

	/** 역할별 링/식별 색 */
	FColor GetRoleColor() const;

protected:
	virtual void BeginPlay() override;

	/** 그레이박스 기둥 (엔진 기본 셰이프 — BeginPlay에서 로드, 웨이포인트보다 낮음) */
	UPROPERTY(VisibleAnywhere, Category = "ROH|Npc")
	TObjectPtr<UStaticMeshComponent> PillarMesh;

private:
	EROHNpcRole NpcRole = EROHNpcRole::General; // AActor::Role(ENetRole) 은닉 방지를 위해 NpcRole
	FText DisplayName;
	FText Greeting;
};
