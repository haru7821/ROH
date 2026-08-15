#include "World/ROHZoneManager.h"
#include "World/ROHMonsterSpawner.h"
#include "World/ROHWaypoint.h"
#include "World/ROHTownNpc.h"
#include "World/ROHStashChest.h"
#include "Character/ROHMonsterCharacter.h"
#include "Character/ROHBossCharacter.h"
#include "Campaign/ROHCampaignSubsystem.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h" // TActorIterator (마을 스포너/침입 몬스터 검사, b34)
#include "TimerManager.h"
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

			// 계정 보관함: 웨이포인트 옆 (M5 최종 — 내용물은 UROHAccountSubsystem 소유)
			GetWorld()->SpawnActor<AROHStashChest>(AROHStashChest::StaticClass(),
				Center + FVector(300.f, 300.f, 0.f), FRotator::ZeroRotator, SpawnParams);
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

	// 마을 안전지대 강제 (b34): 맵에 배치된 잔재 스포너 제거 + 침입 몬스터 정리 타이머.
	// 존 배치 이후에 수행해야 방금 스폰한 전투 지역 스포너까지 검사 대상에 들어간다
	// (전투 지역 스포너는 마을 반경 밖이라 제거되지 않는다).
	RemoveSpawnersInTownZones();

	bool bHasTownZone = false;
	for (const FROHZoneDef& Zone : Zones)
	{
		if (Zone.bTown)
		{
			bHasTownZone = true;
			break;
		}
	}
	if (bHasTownZone)
	{
		// 저빈도 폴링(1초): 잔재 스포너가 이미 스폰한 개체 + 플레이어를 쫓아 들어온 개체 백스톱
		GetWorld()->GetTimerManager().SetTimer(TownCleanupTimerHandle, this,
			&AROHZoneManager::CleanupTownIntruders, 1.f, true);
	}

	UE_LOG(LogROH, Log, TEXT("지역 매니저: %d개 지역 구성 완료 (%s)"), Zones.Num(), *GetActorLocation().ToCompactString());
}

bool AROHZoneManager::IsInsideTownZone(const FVector& Location) const
{
	for (int32 ZoneIndex = 0; ZoneIndex < Zones.Num(); ++ZoneIndex)
	{
		if (Zones[ZoneIndex].bTown
			&& FVector::DistSquared2D(Location, GetZoneCenter(ZoneIndex)) <= FMath::Square(Zones[ZoneIndex].Radius))
		{
			return true;
		}
	}
	return false;
}

void AROHZoneManager::RemoveSpawnersInTownZones()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 순회 중 Destroy는 반복자 무효화 위험이 있어 수집 후 제거.
	// 판정 (Sup b34): 맵에 직접 배치된 스포너(IsZoneOwned=false)는 위치와 무관하게 제거한다 —
	// 지역 설정을 못 받아 기본값으로 도는 잔재이고, 마을 반경 경계에 걸치면 위치 판정만으로는
	// 새어나간다. 지역 매니저가 만든 스포너는 마을 안에 있을 때만 제거(정상 상황엔 해당 없음).
	// 이 함수는 존 배치 루프 뒤에 도는데, 그 스포너들은 스폰 직후 ConfigureSpawner를 받으므로 안전.
	TArray<AROHMonsterSpawner*> DoomedSpawners;
	for (TActorIterator<AROHMonsterSpawner> It(World); It; ++It)
	{
		AROHMonsterSpawner* Spawner = *It;
		if (!Spawner || Spawner->IsActorBeingDestroyed())
		{
			continue;
		}
		if (!Spawner->IsZoneOwned() || IsInsideTownZone(Spawner->GetActorLocation()))
		{
			DoomedSpawners.Add(Spawner);
		}
	}

	for (AROHMonsterSpawner* Spawner : DoomedSpawners)
	{
		UE_LOG(LogROH, Warning, TEXT("지역 매니저가 관리하지 않는 몬스터 스포너를 제거했습니다 (%s) — 맵에 배치된 잔재입니다"),
			*Spawner->GetActorLocation().ToCompactString());
		Spawner->Destroy();
	}
}

void AROHZoneManager::CleanupTownIntruders()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 대상: 적대 팀(TeamId != 0) 몬스터/보스만. 플레이어(TeamId 0)·NPC·웨이포인트·보관함은
	// AROHMonsterCharacter가 아니거나 팀이 달라 애초에 순회 대상에 들어오지 않는다.
	TArray<AROHMonsterCharacter*> Intruders;
	for (TActorIterator<AROHMonsterCharacter> It(World); It; ++It)
	{
		AROHMonsterCharacter* Monster = *It;
		if (!Monster || Monster->IsActorBeingDestroyed() || Monster->GetTeamId() == 0)
		{
			continue;
		}
		// 시체는 SetLifeSpan이 정리한다 — 여기서 건드리면 아래 OnDeath 재브로드캐스트로 카운트 이중 감소
		if (!Monster->IsAlive())
		{
			continue;
		}
		// 보스는 퀘스트 필수 자원 — 지우면 세션 내 재조우 불가 (존 보스는 BeginPlay 1회 스폰).
		// 마을까지 따라오면 플레이어가 처치하면 되고, 처치는 정상 퀘스트 진행으로 집계된다 (Sup b34)
		if (Monster->IsA<AROHBossCharacter>())
		{
			continue;
		}
		if (Monster->IsExemptFromTownCleanup()) // 치트 소환 (+그 하수인) — 마을 테스트 보호
		{
			continue;
		}
		if (IsInsideTownZone(Monster->GetActorLocation()))
		{
			Intruders.Add(Monster);
		}
	}

	for (AROHMonsterCharacter* Monster : Intruders)
	{
		// 로그는 Verbose (1초 폴링이라 스팸 방지). 조용히 소멸 — 드랍/경험치 없음(사망 처리 아님)
		UE_LOG(LogROH, Verbose, TEXT("마을 침입 몬스터 제거: %s (%s)"),
			*Monster->GetName(), *Monster->GetActorLocation().ToCompactString());
		// 스포너 개체 수 회수 + 재스폰 예약 (Sup b34): Destroy는 사망 파이프라인을 우회해
		// OnMonsterDeath가 안 불리고, 그러면 추격당해 마을로 끌려온 개체만큼 AliveCount가
		// 영구 누수돼 해당 지역이 비어버린다. 구독자는 스포너뿐이라 보상/퀘스트 집계는 영향 없음
		Monster->OnDeath.Broadcast(Monster);
		Monster->Destroy();
	}
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
