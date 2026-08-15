#include "UI/ROHMinimapWidget.h"
#include "Character/ROHCharacterBase.h"
#include "Character/ROHMonsterCharacter.h"
#include "Character/ROHBossCharacter.h"
#include "World/ROHTownNpc.h"
#include "World/ROHWaypoint.h"
#include "World/ROHStashChest.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "EngineUtils.h" // TActorIterator
#include "Engine/World.h"
#include "Rendering/DrawElements.h" // FSlateDrawElement
#include "Styling/CoreStyle.h"      // FCoreStyle::Get().GetBrush
#include "GameFramework/Pawn.h"

void UROHMinimapWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree)
	{
		return;
	}

	// 상시 HUD — 클릭 이동을 막지 않는다
	SetVisibility(ESlateVisibility::HitTestInvisible);

	// 배경 박스 (점은 NativePaint가 이 위 레이어에 그린다). 크기는 컨트롤러가
	// SetDesiredSizeInViewport(200×200)로 지정 — 뷰포트 슬롯이 지오메트리를 결정한다.
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.04f, 0.55f));
	WidgetTree->RootWidget = Background;
}

void UROHMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 액터 수집은 0.25초 간격 캐시 — 매 프레임 TActorIterator 비용 회피
	CacheTimer -= InDeltaTime;
	if (CacheTimer <= 0.f)
	{
		CacheTimer = CacheInterval;
		RebuildMarkCache();
	}
}

void UROHMinimapWidget::RebuildMarkCache()
{
	CachedMarks.Reset();
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	auto AddMark = [this](const AActor* Actor, const FLinearColor& Color, bool bCheckAlive)
	{
		FROHMinimapMark Mark;
		Mark.Actor = Actor;
		Mark.Color = Color;
		Mark.bCheckAlive = bCheckAlive;
		CachedMarks.Add(Mark);
	};

	// 몬스터/보스 (플레이어 진영 제외, 생존만 — 사망은 Paint에서도 재검증)
	for (TActorIterator<AROHCharacterBase> It(World); It; ++It)
	{
		const AROHCharacterBase* Character = *It;
		if (!Character || Character->GetTeamId() == 0 || !Character->IsAlive())
		{
			continue;
		}
		FLinearColor Color(1.f, 0.2f, 0.2f); // 일반 몬스터 빨강
		if (Character->IsA<AROHBossCharacter>())
		{
			Color = FLinearColor(0.7f, 0.2f, 0.9f); // 보스 자주
		}
		else if (const AROHMonsterCharacter* Monster = Cast<AROHMonsterCharacter>(Character))
		{
			if (Monster->GetRank() != EROHMonsterRank::Normal)
			{
				Color = FLinearColor(1.f, 0.55f, 0.1f); // 정예(챔피언/고유) 주황
			}
		}
		AddMark(Character, Color, /*bCheckAlive=*/true);
	}

	// 마을 NPC 초록 / 웨이포인트 파랑 / 계정 보관함 노랑
	for (TActorIterator<AROHTownNpc> It(World); It; ++It)
	{
		AddMark(*It, FLinearColor(0.2f, 1.f, 0.3f), false);
	}
	for (TActorIterator<AROHWaypoint> It(World); It; ++It)
	{
		AddMark(*It, FLinearColor(0.3f, 0.6f, 1.f), false);
	}
	for (TActorIterator<AROHStashChest> It(World); It; ++It)
	{
		AddMark(*It, FLinearColor(1.f, 0.85f, 0.2f), false);
	}
}

int32 UROHMinimapWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 MaxLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const APawn* PlayerPawn = GetOwningPlayerPawn();
	if (!PlayerPawn)
	{
		return MaxLayer;
	}

	const FVector2f LocalSize = FVector2f(AllottedGeometry.GetLocalSize());
	const FVector2f MapCenter = LocalSize * 0.5f;
	const float Scale = (FMath::Min(LocalSize.X, LocalSize.Y) * 0.5f) / WorldRadius;
	const FVector PlayerLocation = PlayerPawn->GetActorLocation();
	const FSlateBrush* DotBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
	const int32 DotLayer = MaxLayer + 1;

	auto DrawDot = [&](const FVector2f& DotPosition, float DotSize, const FLinearColor& DotColor)
	{
		FSlateDrawElement::MakeBox(OutDrawElements, DotLayer,
			AllottedGeometry.ToPaintGeometry(FVector2f(DotSize, DotSize),
				FSlateLayoutTransform(DotPosition - FVector2f(DotSize * 0.5f, DotSize * 0.5f))),
			DotBrush, ESlateDrawEffect::None, DotColor);
	};

	for (const FROHMinimapMark& Mark : CachedMarks)
	{
		const AActor* Actor = Mark.Actor.Get();
		if (!Actor)
		{
			continue; // 파괴된 액터 (0.25s 캐시 사이 소멸 — 약참조가 흡수)
		}
		if (Mark.bCheckAlive)
		{
			const AROHCharacterBase* Character = Cast<AROHCharacterBase>(Actor);
			if (!Character || !Character->IsAlive())
			{
				continue; // 사망 몬스터 즉시 제외
			}
		}
		const FVector Delta = Actor->GetActorLocation() - PlayerLocation;
		if (Delta.SizeSquared2D() > FMath::Square(WorldRadius))
		{
			continue;
		}
		// 플레이어 중심 고정, 북쪽(X+)=위: 화면X = 중앙 + 동서(Y)·축척, 화면Y = 중앙 − 남북(X)·축척
		const FVector2f DotPosition = MapCenter
			+ FVector2f(static_cast<float>(Delta.Y) * Scale, -static_cast<float>(Delta.X) * Scale);
		DrawDot(DotPosition, 5.f, Mark.Color);
	}

	// 플레이어: 중앙 흰 점
	DrawDot(MapCenter, 7.f, FLinearColor::White);

	return DotLayer;
}
