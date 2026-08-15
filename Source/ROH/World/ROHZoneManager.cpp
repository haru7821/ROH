#include "World/ROHZoneManager.h"
#include "World/ROHMonsterSpawner.h"
#include "World/ROHWaypoint.h"
#include "World/ROHTownNpc.h"
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

		// 마을 NPC 6인 (docs/12 로스터가 사양) — bTown 존마다 동일 로스터 스폰
		// (액트 확장 시 존 테이블에 마을이 늘어도 이 블록이 그대로 커버한다)
		if (Zone.bTown)
		{
			SpawnTownNpcs(Center);
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

void AROHZoneManager::SpawnTownNpcs(const FVector& TownCenter)
{
	// docs/12 액트1 로스터 (이름/역할/인사말이 사양 — 문구 수정 금지)
	struct FNpcRosterEntry
	{
		EROHNpcRole Role;
		const TCHAR* Name;
		const TCHAR* Greeting;
	};
	static const FNpcRosterEntry Roster[] = {
		{ EROHNpcRole::General,      TEXT("로사"), TEXT("쓸 만한 게 있으면 내려놓고 가. 값은 후하게 쳐주지… 여기선 골드도 재가 되니까.") },
		{ EROHNpcRole::Blacksmith,   TEXT("브란"), TEXT("내 조상이 성인들의 검을 벼렸다. 네 것도 부끄럽지 않게 만들어주지.") },
		{ EROHNpcRole::Jeweler,      TEXT("일렌"), TEXT("룬은 신의 언어다. …물론, 팔기도 하지.") },
		{ EROHNpcRole::PotionVendor, TEXT("미로"), TEXT("이 약초, 목숨 걸고 캐온 거예요. 진짜라니까요?") },
		{ EROHNpcRole::Gambler,      TEXT("카론"), TEXT("운명의 보석을 가져와. 영웅들이 쥐던 무기가… 네 것이 될 수도 있지.") },
		{ EROHNpcRole::Identifier,   TEXT("셀바"), TEXT("이리 줘 보렴… 물건은 거짓말을 하지 않아. 사람과 달리.") },
	};

	// 마을 중심(웨이포인트) 주변 반경 600, 60도 간격 호 — -X(입구 반대) 방향이 비도록 -150°~+150°
	for (int32 NpcIndex = 0; NpcIndex < UE_ARRAY_COUNT(Roster); ++NpcIndex)
	{
		const float AngleRad = FMath::DegreesToRadians(-150.f + NpcIndex * 60.f);
		const FVector NpcLocation = TownCenter + FVector(FMath::Cos(AngleRad) * 600.f, FMath::Sin(AngleRad) * 600.f, 0.f);
		const FTransform NpcTransform(FRotator::ZeroRotator, NpcLocation);
		if (AROHTownNpc* Npc = GetWorld()->SpawnActorDeferred<AROHTownNpc>(
			AROHTownNpc::StaticClass(), NpcTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
		{
			Npc->SetNpcInfo(Roster[NpcIndex].Role,
				FText::FromString(Roster[NpcIndex].Name), FText::FromString(Roster[NpcIndex].Greeting));
			Npc->FinishSpawning(NpcTransform);
		}
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
