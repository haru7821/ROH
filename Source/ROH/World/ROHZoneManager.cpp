#include "World/ROHZoneManager.h"
#include "World/ROHMonsterSpawner.h"
#include "World/ROHWaypoint.h"
#include "Character/ROHMonsterCharacter.h"
#include "Character/ROHBossCharacter.h"
#include "Campaign/ROHCampaignSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h" // GetSubsystem<T>() 템플릿 인스턴스화에 완전한 타입 필요
#include "ROH.h"

AROHZoneManager::AROHZoneManager()
{
	// 그레이박스: 지역 경계 디버그 표시용 틱 (아트/포스트 존 볼륨 도입 시 비활성화)
	PrimaryActorTick.bCanEverTick = true;
}

void AROHZoneManager::BuildZoneTable()
{
	// 지역 4개: 마을 → 들판 → 지하묘지(발타르) → 성소(모르가스). +X 6000uu 간격 (docs/06 액트1 동선)
	Zones.Reset();

	FROHZoneDef Town;
	Town.Name = FText::FromString(TEXT("재의 마을"));
	Town.CenterOffset = FVector::ZeroVector;
	Town.Radius = 2500.f;
	Town.bTown = true; // 안전 지대: 스포너 없음, 웨이포인트 자동 활성화
	Zones.Add(Town);

	FROHZoneDef Fields;
	Fields.Name = FText::FromString(TEXT("잿빛 들판"));
	Fields.CenterOffset = FVector(6000.f, 0.f, 0.f);
	Fields.Radius = 3000.f;
	Fields.SpawnClasses = { AROHMonster_Grunt::StaticClass(), AROHMonster_Archer::StaticClass(), AROHMonster_Charger::StaticClass() };
	Fields.MaxAlive = 8;
	Fields.MonsterLevelBonus = 0;
	Zones.Add(Fields);

	FROHZoneDef Catacombs;
	Catacombs.Name = FText::FromString(TEXT("무너진 지하묘지"));
	Catacombs.CenterOffset = FVector(12000.f, 0.f, 0.f);
	Catacombs.Radius = 3000.f;
	Catacombs.SpawnClasses = { AROHMonster_Grunt::StaticClass(), AROHMonster_Brute::StaticClass(), AROHMonster_Hexer::StaticClass(), AROHMonster_Stalker::StaticClass() };
	Catacombs.MaxAlive = 10;
	Catacombs.MonsterLevelBonus = 5;
	Catacombs.BossClass = AROHBossCharacter::StaticClass(); // 발타르
	Catacombs.BossMaxQuestStage = 1;                        // 발타르 처치(→2) 이후엔 스폰 안 함
	Zones.Add(Catacombs);

	FROHZoneDef Sanctum;
	Sanctum.Name = FText::FromString(TEXT("재의 성소"));
	Sanctum.CenterOffset = FVector(18000.f, 0.f, 0.f);
	Sanctum.Radius = 3000.f;
	Sanctum.SpawnClasses = { AROHMonster_Brute::StaticClass(), AROHMonster_Hexer::StaticClass(), AROHMonster_Stalker::StaticClass() };
	Sanctum.MaxAlive = 6;
	Sanctum.MonsterLevelBonus = 8;
	Sanctum.BossClass = AROHBossMorgath::StaticClass();
	Sanctum.BossMaxQuestStage = 2; // 모르가스 처치(→3) 이후엔 스폰 안 함
	Zones.Add(Sanctum);
}

void AROHZoneManager::BeginPlay()
{
	Super::BeginPlay();

	BuildZoneTable();

	UROHCampaignSubsystem* Campaign = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UROHCampaignSubsystem>() : nullptr;

	// 마을 웨이포인트는 시작부터 사용 가능
	if (Campaign)
	{
		Campaign->ActivateWaypoint(0);
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (int32 ZoneIndex = 0; ZoneIndex < Zones.Num(); ++ZoneIndex)
	{
		const FROHZoneDef& Zone = Zones[ZoneIndex];
		const FVector Center = GetZoneCenter(ZoneIndex);

		// 웨이포인트: 지연 스폰으로 ZoneIndex/이름을 BeginPlay(오버랩 활성화) 전에 주입
		const FTransform WaypointTransform(FRotator::ZeroRotator, Center);
		if (AROHWaypoint* Waypoint = GetWorld()->SpawnActorDeferred<AROHWaypoint>(
			AROHWaypoint::StaticClass(), WaypointTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
		{
			Waypoint->SetZoneInfo(ZoneIndex, Zone.Name);
			Waypoint->FinishSpawning(WaypointTransform);
		}

		// 스포너 (마을 제외). 스포너의 초기 스폰은 BeginPlay의 0.5초 타이머라 스폰 직후 설정 주입이 안전
		if (!Zone.bTown && Zone.SpawnClasses.Num() > 0)
		{
			const FVector SpawnerLocation = Center + FVector(0.f, 800.f, 100.f);
			if (AROHMonsterSpawner* Spawner = GetWorld()->SpawnActor<AROHMonsterSpawner>(
				AROHMonsterSpawner::StaticClass(), SpawnerLocation, FRotator::ZeroRotator, SpawnParams))
			{
				Spawner->ConfigureSpawner(Zone.SpawnClasses, Zone.MaxAlive, Zone.MonsterLevelBonus);
			}
		}

		// 지역 보스: 해당 퀘스트 단계를 아직 넘지 않았을 때만 (세이브 로드 후 재스폰 방지)
		const int32 QuestStage = Campaign ? Campaign->GetQuestStage() : 0;
		if (Zone.BossClass && QuestStage <= Zone.BossMaxQuestStage)
		{
			const FVector BossLocation = Center + FVector(1500.f, 0.f, 100.f);
			GetWorld()->SpawnActor<AROHMonsterCharacter>(Zone.BossClass, BossLocation, FRotator::ZeroRotator, SpawnParams);
		}
	}

	UE_LOG(LogROH, Log, TEXT("지역 매니저: %d개 지역 구성 완료 (%s)"), Zones.Num(), *GetActorLocation().ToCompactString());
}

void AROHZoneManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 그레이박스: 지역 경계 표시 (마을 초록, 전투 지역 주황). 지역명은 플레이어 HUD가 담당
	for (int32 ZoneIndex = 0; ZoneIndex < Zones.Num(); ++ZoneIndex)
	{
		const FColor BoundaryColor = Zones[ZoneIndex].bTown ? FColor::Green : FColor::Orange;
		DrawDebugCircle(GetWorld(), GetZoneCenter(ZoneIndex) + FVector(0.f, 0.f, 30.f), Zones[ZoneIndex].Radius,
			64, BoundaryColor, false, -1.f, 0, 8.f, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
	}
}

int32 AROHZoneManager::GetZoneIndexAt(const FVector& Location) const
{
	int32 BestIndex = INDEX_NONE;
	float BestDistSq = TNumericLimits<float>::Max();
	for (int32 ZoneIndex = 0; ZoneIndex < Zones.Num(); ++ZoneIndex)
	{
		const float DistSq = FVector::DistSquared2D(Location, GetZoneCenter(ZoneIndex));
		if (DistSq <= FMath::Square(Zones[ZoneIndex].Radius) && DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestIndex = ZoneIndex;
		}
	}
	return BestIndex;
}

FText AROHZoneManager::GetZoneName(int32 ZoneIndex) const
{
	return Zones.IsValidIndex(ZoneIndex) ? Zones[ZoneIndex].Name : FText::GetEmpty();
}

bool AROHZoneManager::IsTown(int32 ZoneIndex) const
{
	return Zones.IsValidIndex(ZoneIndex) && Zones[ZoneIndex].bTown;
}

FVector AROHZoneManager::GetWaypointLocation(int32 ZoneIndex) const
{
	return Zones.IsValidIndex(ZoneIndex) ? GetZoneCenter(ZoneIndex) : GetActorLocation();
}

FVector AROHZoneManager::GetZoneCenter(int32 ZoneIndex) const
{
	return GetActorLocation() + (Zones.IsValidIndex(ZoneIndex) ? Zones[ZoneIndex].CenterOffset : FVector::ZeroVector);
}
