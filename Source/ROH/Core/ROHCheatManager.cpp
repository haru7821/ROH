#include "Core/ROHCheatManager.h"
#include "Core/ROHGameMode.h"
#include "Items/ROHItemDatabase.h"
#include "Items/ROHInventoryComponent.h"
#include "Items/ROHItemTypes.h"
#include "Character/ROHPlayerCharacter.h"
#include "Character/ROHPlayerClasses.h"
#include "Character/ROHBossCharacter.h"
#include "Campaign/ROHCampaignSubsystem.h"
#include "Progression/ROHProgressionComponent.h"
#include "Progression/ROHSkillTreeComponent.h"
#include "Save/ROHSaveSubsystem.h"
#include "World/ROHZoneManager.h"
#include "EngineUtils.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "ROH.h"

namespace
{
	UAbilitySystemComponent* GetPlayerASC(const UCheatManager* Cheat)
	{
		const APlayerController* PC = Cheat->GetOuterAPlayerController();
		if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PC ? PC->GetPawn() : nullptr))
		{
			return ASI->GetAbilitySystemComponent();
		}
		return nullptr;
	}

	UROHInventoryComponent* GetPlayerInventory(const UCheatManager* Cheat)
	{
		const APlayerController* PC = Cheat->GetOuterAPlayerController();
		if (const AROHPlayerCharacter* Player = Cast<AROHPlayerCharacter>(PC ? PC->GetPawn() : nullptr))
		{
			return Player->GetInventory();
		}
		return nullptr;
	}

	UROHItemDatabase* GetDatabase(const UCheatManager* Cheat)
	{
		const UWorld* World = Cheat->GetWorld();
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UROHItemDatabase>() : nullptr;
	}

	void CheatPrint(const FString& Line)
	{
		UE_LOG(LogROH, Log, TEXT("%s"), *Line);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan, Line);
		}
	}
}

void UROHCheatManager::ROHSetAttr(FName AttributeName, float Value)
{
	UAbilitySystemComponent* ASC = GetPlayerASC(this);
	if (!ASC)
	{
		UE_LOG(LogROH, Warning, TEXT("ROHSetAttr: 플레이어 ASC를 찾을 수 없습니다."));
		return;
	}

	TArray<FGameplayAttribute> Attributes;
	ASC->GetAllAttributes(Attributes);

	for (const FGameplayAttribute& Attribute : Attributes)
	{
		if (Attribute.GetName() == AttributeName.ToString())
		{
			ASC->SetNumericAttributeBase(Attribute, Value);
			UE_LOG(LogROH, Log, TEXT("ROHSetAttr: %s = %.1f"), *Attribute.GetName(), Value);
			return;
		}
	}

	UE_LOG(LogROH, Warning, TEXT("ROHSetAttr: '%s' 어트리뷰트가 없습니다. ROHDumpAttrs로 이름을 확인하세요."), *AttributeName.ToString());
}

void UROHCheatManager::ROHDumpAttrs()
{
	UAbilitySystemComponent* ASC = GetPlayerASC(this);
	if (!ASC)
	{
		return;
	}

	TArray<FGameplayAttribute> Attributes;
	ASC->GetAllAttributes(Attributes);

	for (const FGameplayAttribute& Attribute : Attributes)
	{
		const float Base = ASC->GetNumericAttributeBase(Attribute);
		const float Current = ASC->GetNumericAttribute(Attribute);
		const FString Line = FString::Printf(TEXT("%s: base=%.1f current=%.1f"), *Attribute.GetName(), Base, Current);
		UE_LOG(LogROH, Log, TEXT("%s"), *Line);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Cyan, Line);
		}
	}
}

void UROHCheatManager::ROHGiveItem(FName BaseId, int32 ItemLevel, FString Quality)
{
	UROHItemDatabase* Database = GetDatabase(this);
	UROHInventoryComponent* Inventory = GetPlayerInventory(this);
	if (!Database || !Inventory)
	{
		return;
	}

	EROHItemQuality QualityEnum = EROHItemQuality::Magic;
	if (Quality.Equals(TEXT("normal"), ESearchCase::IgnoreCase))
	{
		QualityEnum = EROHItemQuality::Normal;
	}
	else if (Quality.Equals(TEXT("rare"), ESearchCase::IgnoreCase))
	{
		QualityEnum = EROHItemQuality::Rare;
	}
	else if (Quality.Equals(TEXT("unique"), ESearchCase::IgnoreCase))
	{
		QualityEnum = EROHItemQuality::Unique; // 후보 없는 베이스/ilvl이면 레어 강등 (DB 규칙)
	}
	else if (Quality.Equals(TEXT("set"), ESearchCase::IgnoreCase))
	{
		QualityEnum = EROHItemQuality::Set; // 강등 규칙 동일
	}

	const FROHItemInstance Item = Database->GenerateItem(BaseId, ItemLevel, QualityEnum);
	if (!Item.IsValid())
	{
		CheatPrint(FString::Printf(TEXT("ROHGiveItem: 알 수 없는 베이스 '%s'"), *BaseId.ToString()));
		return;
	}
	if (Inventory->AddItem(Item))
	{
		CheatPrint(FString::Printf(TEXT("획득: %s (접사 %d개)"),
			*Database->GetItemDisplayName(Item).ToString(), Item.Affixes.Num()));
	}
	else
	{
		CheatPrint(TEXT("인벤토리가 가득 찼습니다"));
	}
}

void UROHCheatManager::ROHDumpInventory()
{
	UROHItemDatabase* Database = GetDatabase(this);
	UROHInventoryComponent* Inventory = GetPlayerInventory(this);
	if (!Database || !Inventory)
	{
		return;
	}

	CheatPrint(FString::Printf(TEXT("=== 골드: %d ==="), Inventory->GetGold()));

	// 소켓 표시: " 소켓[카르,벨,-]" (M5)
	auto SocketText = [Database](const FROHItemInstance& Item) -> FString
	{
		if (Item.MaxSockets <= 0)
		{
			return FString();
		}
		TArray<FString> Parts;
		for (int32 SocketIndex = 0; SocketIndex < Item.MaxSockets; ++SocketIndex)
		{
			if (Item.SocketedRunes.IsValidIndex(SocketIndex))
			{
				const FROHRuneDef* Rune = Database->FindRune(Item.SocketedRunes[SocketIndex]);
				Parts.Add(Rune ? Rune->DisplayName.ToString() : Item.SocketedRunes[SocketIndex].ToString());
			}
			else
			{
				Parts.Add(TEXT("-"));
			}
		}
		return FString::Printf(TEXT(" 소켓[%s]"), *FString::Join(Parts, TEXT(",")));
	};

	CheatPrint(TEXT("--- 장비창 ---"));
	for (const auto& Pair : Inventory->GetEquipped())
	{
		FString AffixText;
		for (const FROHAffixRoll& Affix : Pair.Value.Affixes)
		{
			AffixText += FString::Printf(TEXT(" [%s +%.0f]"), *Affix.AffixId.ToString(), Affix.Value);
		}
		CheatPrint(FString::Printf(TEXT("슬롯 %d: %s%s%s"),
			static_cast<int32>(Pair.Key),
			*Database->GetItemDisplayName(Pair.Value).ToString(), *AffixText, *SocketText(Pair.Value)));
	}

	// 세트 장착 집계 (M5 2차): "세트: 잿빛 첨탑 2/3"
	{
		TMap<FName, int32> SetCounts;
		for (const auto& Pair : Inventory->GetEquipped())
		{
			if (Pair.Value.SetPieceId.IsNone())
			{
				continue;
			}
			const FROHSetDef* OwningSet = nullptr;
			if (Database->FindSetPiece(Pair.Value.SetPieceId, &OwningSet) && OwningSet)
			{
				++SetCounts.FindOrAdd(OwningSet->SetId);
			}
		}
		for (const auto& Pair : SetCounts)
		{
			if (const FROHSetDef* Set = Database->FindSet(Pair.Key))
			{
				CheatPrint(FString::Printf(TEXT("세트: %s %d/%d"),
					*Set->DisplayName.ToString(), Pair.Value, Set->Pieces.Num()));
			}
		}
	}

	CheatPrint(TEXT("--- 인벤토리 ---"));
	const TArray<FROHItemInstance>& Items = Inventory->GetItems();
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		CheatPrint(FString::Printf(TEXT("%d: %s (ilvl %d, 접사 %d)%s"),
			i, *Database->GetItemDisplayName(Items[i]).ToString(),
			Items[i].ItemLevel, Items[i].Affixes.Num(), *SocketText(Items[i])));
	}
}

void UROHCheatManager::ROHEquip(int32 ItemIndex)
{
	if (UROHInventoryComponent* Inventory = GetPlayerInventory(this))
	{
		CheatPrint(Inventory->EquipItemByIndex(ItemIndex)
			? FString::Printf(TEXT("장착 완료 (인덱스 %d). ROHDumpAttrs로 스탯 확인"), ItemIndex)
			: TEXT("장착 실패 (인덱스/종류 확인)"));
	}
}

void UROHCheatManager::ROHUsePotion()
{
	if (UROHInventoryComponent* Inventory = GetPlayerInventory(this))
	{
		CheatPrint(Inventory->UseFirstPotion() ? TEXT("물약 사용") : TEXT("물약 없음"));
	}
}

void UROHCheatManager::ROHAddGold(int32 Amount)
{
	if (UROHInventoryComponent* Inventory = GetPlayerInventory(this))
	{
		Inventory->AddGold(Amount);
		CheatPrint(FString::Printf(TEXT("골드: %d"), Inventory->GetGold()));
	}
}

void UROHCheatManager::ROHBuyPotion()
{
	// 정식 창구는 마을 포션상인 미로(벤더 창, docs/12) — 이 치트는 테스트 편의로 유지
	UROHItemDatabase* Database = GetDatabase(this);
	UROHInventoryComponent* Inventory = GetPlayerInventory(this);
	if (!Database || !Inventory)
	{
		return;
	}

	const FROHItemBaseDef* PotionBase = Database->FindBase(TEXT("HealthPotion"));
	if (!PotionBase)
	{
		return;
	}
	if (Inventory->IsFull())
	{
		CheatPrint(TEXT("인벤토리가 가득 찼습니다"));
		return;
	}
	if (!Inventory->SpendGold(PotionBase->GoldValue))
	{
		CheatPrint(FString::Printf(TEXT("골드 부족 (필요: %d)"), PotionBase->GoldValue));
		return;
	}
	if (!Inventory->AddItem(Database->GenerateItem(TEXT("HealthPotion"), 1, EROHItemQuality::Normal)))
	{
		Inventory->AddGold(PotionBase->GoldValue); // 실패 시 환불
		return;
	}
	CheatPrint(FString::Printf(TEXT("치유물약 구매 (-%d 골드, 잔액 %d)"), PotionBase->GoldValue, Inventory->GetGold()));
}

void UROHCheatManager::ROHSimulateDrops(FName TCId, int32 Count)
{
	UROHItemDatabase* Database = GetDatabase(this);
	if (!Database || Count <= 0)
	{
		return;
	}

	int32 TotalItems = 0;
	int32 TotalGold = 0;
	int32 GoldDrops = 0;
	int32 QualityCounts[5] = { 0, 0, 0, 0, 0 }; // Normal, Magic, Rare, Unique, Set
	TMap<FName, int32> BaseCounts;

	for (int32 i = 0; i < Count; ++i)
	{
		const FROHDropResult Drop = Database->RollTreasureClass(TCId, 5, 0.f);
		TotalItems += Drop.Items.Num();
		TotalGold += Drop.Gold;
		if (Drop.Gold > 0)
		{
			++GoldDrops;
		}
		for (const FROHItemInstance& Item : Drop.Items)
		{
			// Set은 enum 말미(6)라 클램프만으론 버킷이 어긋난다 — 명시 매핑
			const int32 QualityIndex = Item.Quality == EROHItemQuality::Set
				? 4 : FMath::Clamp(static_cast<int32>(Item.Quality), 0, 3);
			++QualityCounts[QualityIndex];
			++BaseCounts.FindOrAdd(Item.BaseId);
		}
	}

	CheatPrint(FString::Printf(TEXT("=== 드랍 시뮬레이션: %s × %d회 ==="), *TCId.ToString(), Count));
	CheatPrint(FString::Printf(TEXT("아이템 %d개 (%.1f%%) | 골드 드랍 %d회, 평균 %.1f"),
		TotalItems, 100.f * TotalItems / Count, GoldDrops, GoldDrops > 0 ? static_cast<float>(TotalGold) / GoldDrops : 0.f));
	CheatPrint(FString::Printf(TEXT("등급: 일반 %d / 매직 %d / 레어 %d / 유니크 %d / 세트 %d"),
		QualityCounts[0], QualityCounts[1], QualityCounts[2], QualityCounts[3], QualityCounts[4]));
	for (const auto& Pair : BaseCounts)
	{
		CheatPrint(FString::Printf(TEXT("  %s: %d"), *Pair.Key.ToString(), Pair.Value));
	}
}

void UROHCheatManager::ROHGiveXP(int32 Amount)
{
	const APlayerController* PC = GetOuterAPlayerController();
	AROHPlayerCharacter* Player = Cast<AROHPlayerCharacter>(PC ? PC->GetPawn() : nullptr);
	if (Player && Player->GetProgression())
	{
		Player->GetProgression()->GrantXP(Amount);
		CheatPrint(FString::Printf(TEXT("경험치 +%d (Lv %d)"), Amount, Player->GetProgression()->GetLevel()));
	}
}

void UROHCheatManager::ROHAllocStat(FName StatName, int32 Count)
{
	const APlayerController* PC = GetOuterAPlayerController();
	AROHPlayerCharacter* Player = Cast<AROHPlayerCharacter>(PC ? PC->GetPawn() : nullptr);
	if (!Player || !Player->GetProgression())
	{
		return;
	}
	int32 Applied = 0;
	for (int32 i = 0; i < FMath::Max(1, Count); ++i)
	{
		if (!Player->GetProgression()->AllocateStat(StatName))
		{
			break;
		}
		++Applied;
	}
	CheatPrint(FString::Printf(TEXT("%s +%d (남은 스탯 포인트 %d)"),
		*StatName.ToString(), Applied, Player->GetProgression()->GetStatPoints()));
}

void UROHCheatManager::ROHSkillUp(FName SkillId)
{
	const APlayerController* PC = GetOuterAPlayerController();
	AROHPlayerCharacter* Player = Cast<AROHPlayerCharacter>(PC ? PC->GetPawn() : nullptr);
	if (!Player || !Player->GetSkillTree())
	{
		return;
	}
	FString Error;
	if (Player->GetSkillTree()->InvestPoint(SkillId, Error))
	{
		CheatPrint(FString::Printf(TEXT("%s 랭크 %d (피해 배수 x%.2f)"),
			*SkillId.ToString(), Player->GetSkillTree()->GetRank(SkillId),
			Player->GetSkillTree()->GetDamageMultiplier(SkillId)));
	}
	else
	{
		CheatPrint(FString::Printf(TEXT("투자 실패: %s"), *Error));
	}
}

void UROHCheatManager::ROHMaxOut(int32 RankPerSkill)
{
	const APlayerController* PC = GetOuterAPlayerController();
	AROHPlayerCharacter* Player = Cast<AROHPlayerCharacter>(PC ? PC->GetPawn() : nullptr);
	if (!Player || !Player->GetProgression() || !Player->GetSkillTree())
	{
		return;
	}
	UROHProgressionComponent* Progression = Player->GetProgression();
	UROHSkillTreeComponent* SkillTree = Player->GetSkillTree();

	// 1) 레벨 50: 다음 레벨까지 남은 경험치를 정확히 반복 지급 (오버플로 방지)
	int32 SafetyCounter = 0;
	while (Progression->GetLevel() < UROHProgressionComponent::MaxLevel
		&& ++SafetyCounter <= UROHProgressionComponent::MaxLevel)
	{
		Progression->GrantXP(UROHProgressionComponent::XPForNextLevel(Progression->GetLevel()) - Progression->GetXP());
	}

	// 2) 전 스킬 투자 — 필요한 포인트를 지급한 뒤 정식 경로(InvestPoint)로 투자
	//    (패시브 GE 적용/선행 검증이 정상 경로를 타야 하므로 하드포인트 직접 조작 금지)
	RankPerSkill = FMath::Clamp(RankPerSkill, 1, 20);
	const TArray<FROHSkillDef>& Defs = UROHSkillTreeComponent::GetSkillDefs(SkillTree->GetPlayerClass());

	int32 NeededPoints = 0;
	for (const FROHSkillDef& Def : Defs)
	{
		NeededPoints += FMath::Max(0, FMath::Min(RankPerSkill, Def.MaxPoints) - SkillTree->GetRank(Def.SkillId));
	}
	for (int32 i = 0; i < NeededPoints; ++i)
	{
		Progression->RefundSkillPoint();
	}

	// 선행 스킬 관계는 다회 패스로 해소 (정의 순서와 무관하게 수렴)
	int32 Invested = 0;
	for (int32 Pass = 0; Pass < 3; ++Pass)
	{
		for (const FROHSkillDef& Def : Defs)
		{
			FString Error;
			while (SkillTree->GetRank(Def.SkillId) < FMath::Min(RankPerSkill, Def.MaxPoints)
				&& SkillTree->InvestPoint(Def.SkillId, Error))
			{
				++Invested;
			}
		}
	}

	CheatPrint(FString::Printf(TEXT("테스트 세팅 완료: Lv %d | 스킬 투자 %d회 (전 스킬 랭크 %d) | 남은 스킬P %d, 스탯P %d"),
		Progression->GetLevel(), Invested, RankPerSkill,
		Progression->GetSkillPoints(), Progression->GetStatPoints()));
	CheatPrint(TEXT("스탯 분배: ROHAllocStat Strength 50 등 | 슬롯 배치: ROHBindSkill <1~4> <SkillId> | 목록: ROHSkillInfo"));
}

void UROHCheatManager::ROHSkillInfo()
{
	const APlayerController* PC = GetOuterAPlayerController();
	AROHPlayerCharacter* Player = Cast<AROHPlayerCharacter>(PC ? PC->GetPawn() : nullptr);
	if (!Player || !Player->GetSkillTree() || !Player->GetProgression())
	{
		return;
	}
	UROHSkillTreeComponent* SkillTree = Player->GetSkillTree();
	CheatPrint(FString::Printf(TEXT("=== 스킬트리 (스킬 포인트 %d) ==="), Player->GetProgression()->GetSkillPoints()));
	for (const FROHSkillDef& Def : UROHSkillTreeComponent::GetSkillDefs(SkillTree->GetPlayerClass()))
	{
		FString Requirement = FString::Printf(TEXT("요구 Lv %d"), Def.RequiredLevel);
		if (!Def.PrereqSkillId.IsNone())
		{
			Requirement += FString::Printf(TEXT(", 선행 %s"), *Def.PrereqSkillId.ToString());
		}

		FString Detail;
		if (Def.Kind == EROHSkillKind::Passive)
		{
			for (const FROHPassiveBonus& PassiveBonus : Def.PassiveBonuses)
			{
				Detail += FString::Printf(TEXT(" +%s %.0f/랭크"), *PassiveBonus.Attribute.GetName(), PassiveBonus.PerRank);
			}
		}
		else
		{
			const float Multiplier = SkillTree->GetDamageMultiplier(Def.SkillId);
			Detail = Multiplier > 0.f ? FString::Printf(TEXT(" 배수 x%.2f"), Multiplier) : TEXT(" 미습득");
		}
		if (!Def.Synergies.IsEmpty())
		{
			Detail += TEXT(" | 시너지:");
			for (const FROHSkillSynergy& Synergy : Def.Synergies)
			{
				Detail += FString::Printf(TEXT(" %s +%.0f%%/pt"), *Synergy.SkillId.ToString(), Synergy.PerPointPercent);
			}
		}

		CheatPrint(FString::Printf(TEXT("[%s] %s (%s, %s): 랭크 %d/%d [%s]%s"),
			*Def.TreeName.ToString(), *Def.SkillId.ToString(), *Def.DisplayName.ToString(),
			Def.Kind == EROHSkillKind::Passive ? TEXT("패시브") : TEXT("액티브"),
			SkillTree->GetRank(Def.SkillId), Def.MaxPoints, *Requirement, *Detail));
	}
	CheatPrint(TEXT("액티브 스킬 배치: ROHBindSkill <슬롯 1~4> <SkillId> | 리스펙: ROHRespec"));
}

void UROHCheatManager::ROHBindSkill(int32 Slot, FName SkillId)
{
	const APlayerController* PC = GetOuterAPlayerController();
	AROHPlayerCharacter* Player = Cast<AROHPlayerCharacter>(PC ? PC->GetPawn() : nullptr);
	if (!Player)
	{
		return;
	}
	FString Error;
	if (Player->BindSkillToSlot(Slot, SkillId, Error))
	{
		CheatPrint(FString::Printf(TEXT("슬롯 %d ← %s (키 %d로 발동)"), Slot, *SkillId.ToString(), Slot));
	}
	else
	{
		CheatPrint(FString::Printf(TEXT("배치 실패: %s"), *Error));
	}
}

void UROHCheatManager::ROHRespec()
{
	const APlayerController* PC = GetOuterAPlayerController();
	AROHPlayerCharacter* Player = Cast<AROHPlayerCharacter>(PC ? PC->GetPawn() : nullptr);
	if (!Player || !Player->GetSkillTree() || !Player->GetProgression())
	{
		return;
	}
	const int32 Refunded = Player->GetSkillTree()->ResetAllPoints();
	CheatPrint(Refunded > 0
		? FString::Printf(TEXT("리스펙 완료: 스킬 포인트 %d 환불 (보유 %d)"), Refunded, Player->GetProgression()->GetSkillPoints())
		: TEXT("환불할 스킬 포인트가 없습니다."));
}

void UROHCheatManager::ROHSetClass(FString ClassName)
{
	APlayerController* PC = GetOuterAPlayerController();
	AROHGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AROHGameMode>() : nullptr;
	if (!PC || !GameMode)
	{
		return;
	}

	TSubclassOf<AROHPlayerCharacter> NewClass;
	if (ClassName.StartsWith(TEXT("war"), ESearchCase::IgnoreCase))
	{
		NewClass = AROHWarriorCharacter::StaticClass();
	}
	else if (ClassName.StartsWith(TEXT("ele"), ESearchCase::IgnoreCase))
	{
		NewClass = AROHElementalistCharacter::StaticClass();
	}
	else
	{
		CheatPrint(TEXT("사용법: ROHSetClass warrior 또는 ROHSetClass elem"));
		return;
	}

	if (GameMode->RespawnPlayerAs(PC, NewClass))
	{
		CheatPrint(FString::Printf(TEXT("클래스 전환: %s (성장/인벤토리는 초기화 — 유지하려면 전환 전 ROHSave)"), *ClassName));
	}
}

void UROHCheatManager::ROHSpawnBoss(FString Which)
{
	const APlayerController* PC = GetOuterAPlayerController();
	const APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
	if (!PlayerPawn || !GetWorld())
	{
		return;
	}

	const bool bMorgath = Which.StartsWith(TEXT("2")) || Which.StartsWith(TEXT("mor"), ESearchCase::IgnoreCase);
	const TSubclassOf<AROHBossCharacter> BossClass = bMorgath
		? TSubclassOf<AROHBossCharacter>(AROHBossMorgath::StaticClass())
		: TSubclassOf<AROHBossCharacter>(AROHBossCharacter::StaticClass());

	const FVector SpawnLocation = PlayerPawn->GetActorLocation() + PlayerPawn->GetActorForwardVector() * 800.f + FVector(0.f, 0.f, 50.f);
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	if (GetWorld()->SpawnActor<AROHBossCharacter>(BossClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams))
	{
		CheatPrint(bMorgath
			? TEXT("액트 보스 모르가스 소환 — 화염탄 3연발은 좌우로 피하고, 하수인부터 정리하세요")
			: TEXT("보스 발타르 소환 — 빨간 장판(내려찍기)은 밖으로 피하세요"));
	}
	else
	{
		CheatPrint(TEXT("보스 소환 실패 (공간 부족)"));
	}
}

void UROHCheatManager::ROHSetDifficulty(FString Name)
{
	UROHCampaignSubsystem* Campaign = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UROHCampaignSubsystem>() : nullptr;
	if (!Campaign)
	{
		return;
	}

	EROHDifficulty NewDifficulty;
	if (Name.StartsWith(TEXT("nor"), ESearchCase::IgnoreCase))
	{
		NewDifficulty = EROHDifficulty::Normal;
	}
	else if (Name.StartsWith(TEXT("night"), ESearchCase::IgnoreCase) || Name.StartsWith(TEXT("악몽")))
	{
		NewDifficulty = EROHDifficulty::Nightmare;
	}
	else if (Name.StartsWith(TEXT("hell"), ESearchCase::IgnoreCase) || Name.StartsWith(TEXT("지옥")))
	{
		NewDifficulty = EROHDifficulty::Hell;
	}
	else
	{
		CheatPrint(TEXT("사용법: ROHSetDifficulty normal | nightmare | hell"));
		return;
	}

	Campaign->SetDifficulty(NewDifficulty);

	const APlayerController* PC = GetOuterAPlayerController();
	if (AROHPlayerCharacter* Player = Cast<AROHPlayerCharacter>(PC ? PC->GetPawn() : nullptr))
	{
		Player->ApplyDifficultyResistPenalty();
	}

	const FROHDifficultyParams Params = UROHCampaignSubsystem::GetDifficultyParams(NewDifficulty);
	CheatPrint(FString::Printf(TEXT("난이도: %s (몬스터 생명 x%.1f/피해 x%.1f/경험치 x%.1f, 저항 %+.0f) — 몬스터 강화는 새 스폰부터"),
		*UROHCampaignSubsystem::GetDifficultyDisplayName(NewDifficulty),
		Params.HealthMult, Params.DamageMult, Params.XPMult, Params.PlayerResistPenalty));
}

void UROHCheatManager::ROHSave()
{
	const APlayerController* PC = GetOuterAPlayerController();
	AROHPlayerCharacter* Player = Cast<AROHPlayerCharacter>(PC ? PC->GetPawn() : nullptr);
	UROHSaveSubsystem* SaveSystem = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UROHSaveSubsystem>() : nullptr;
	if (Player && SaveSystem)
	{
		CheatPrint(SaveSystem->SaveCharacter(Player) ? TEXT("세이브 완료") : TEXT("세이브 실패"));
	}
}

void UROHCheatManager::ROHLoad()
{
	const APlayerController* PC = GetOuterAPlayerController();
	AROHPlayerCharacter* Player = Cast<AROHPlayerCharacter>(PC ? PC->GetPawn() : nullptr);
	UROHSaveSubsystem* SaveSystem = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UROHSaveSubsystem>() : nullptr;
	if (Player && SaveSystem)
	{
		CheatPrint(SaveSystem->LoadCharacter(Player) ? TEXT("로드 완료") : TEXT("로드 실패 (세이브 없음/버전 불일치)"));
	}
}

void UROHCheatManager::ROHWarp(int32 ZoneIndex)
{
	APlayerController* PC = GetOuterAPlayerController();
	AROHPlayerCharacter* Player = Cast<AROHPlayerCharacter>(PC ? PC->GetPawn() : nullptr);
	const UROHCampaignSubsystem* Campaign = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UROHCampaignSubsystem>() : nullptr;
	AROHZoneManager* ZoneManager = nullptr;
	for (TActorIterator<AROHZoneManager> It(GetWorld()); It; ++It)
	{
		ZoneManager = *It;
		break;
	}
	if (!Player || !Campaign || !ZoneManager)
	{
		CheatPrint(TEXT("ROHWarp: 지역 매니저가 없습니다"));
		return;
	}

	if (ZoneIndex < 0 || ZoneIndex >= ZoneManager->GetZoneCount())
	{
		// 목록 출력
		for (int32 Index = 0; Index < ZoneManager->GetZoneCount(); ++Index)
		{
			CheatPrint(FString::Printf(TEXT("  %d: %s %s"), Index,
				*ZoneManager->GetZoneName(Index).ToString(),
				Campaign->IsWaypointActivated(Index) ? TEXT("[활성]") : TEXT("[미활성]")));
		}
		CheatPrint(TEXT("사용법: ROHWarp <지역 0~3> (활성화된 웨이포인트만)"));
		return;
	}

	// 이동 로직은 플레이어에 일원화 (웨이포인트 창과 공용) — 결과/사유는 화면 메시지로 표시됨
	Player->TravelToZone(ZoneIndex);
}

void UROHCheatManager::ROHGiveRune(FString TierOrName, int32 Count)
{
	UROHItemDatabase* Database = GetDatabase(this);
	UROHInventoryComponent* Inventory = GetPlayerInventory(this);
	if (!Database || !Inventory)
	{
		return;
	}

	const FROHRuneDef* Rune = TierOrName.IsNumeric()
		? Database->FindRuneByTier(FCString::Atoi(*TierOrName))
		: Database->FindRune(FName(*TierOrName));
	if (!Rune)
	{
		CheatPrint(TEXT("사용법: ROHGiveRune <티어 1~12 또는 룬 ID> [개수] — 목록은 ROHRunes"));
		return;
	}

	Count = FMath::Clamp(Count, 1, 40);
	int32 Given = 0;
	for (int32 i = 0; i < Count; ++i)
	{
		if (Inventory->AddItem(Database->GenerateItem(UROHItemDatabase::GetRuneBaseId(Rune->RuneId), Rune->Tier, EROHItemQuality::Normal)))
		{
			++Given;
		}
	}
	CheatPrint(FString::Printf(TEXT("획득: %s 룬 ×%d (T%d)"),
		*Rune->DisplayName.ToString(), Given, Rune->Tier));
}

void UROHCheatManager::ROHTransmute()
{
	UROHItemDatabase* Database = GetDatabase(this);
	UROHInventoryComponent* Inventory = GetPlayerInventory(this);
	if (!Database || !Inventory)
	{
		return;
	}

	// 티어별 보유 룬 인덱스 수집
	TMap<int32, TArray<int32>> IndicesByTier;
	const TArray<FROHItemInstance>& Items = Inventory->GetItems();
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (const FROHRuneDef* Rune = Database->FindRuneByBaseId(Items[i].BaseId))
		{
			IndicesByTier.FindOrAdd(Rune->Tier).Add(i);
		}
	}

	// 낮은 티어 우선 3:1 합성 (최고 티어 12는 합성 불가)
	for (int32 Tier = 1; Tier < 12; ++Tier)
	{
		const TArray<int32>* Indices = IndicesByTier.Find(Tier);
		if (!Indices || Indices->Num() < 3)
		{
			continue;
		}

		// 앞쪽 3개를 소비 — RemoveAt 무효화 방지를 위해 내림차순으로 정렬해 뒤부터 제거
		TArray<int32> Consume((*Indices).GetData(), 3);
		Consume.Sort([](int32 A, int32 B) { return A > B; });
		for (const int32 RemoveIndex : Consume)
		{
			Inventory->RemoveItemAt(RemoveIndex);
		}

		const FROHRuneDef* Result = Database->FindRuneByTier(Tier + 1);
		if (Result)
		{
			Inventory->AddItem(Database->GenerateItem(UROHItemDatabase::GetRuneBaseId(Result->RuneId), Result->Tier, EROHItemQuality::Normal));
			CheatPrint(FString::Printf(TEXT("합성 성공: T%d 룬 3개 → %s 룬 (T%d)"),
				Tier, *Result->DisplayName.ToString(), Result->Tier));
		}
		return;
	}

	const TArray<int32>* ApexIndices = IndicesByTier.Find(12);
	CheatPrint(ApexIndices && ApexIndices->Num() >= 3
		? TEXT("아자크는 최고 티어 룬입니다 — 더 이상 합성할 수 없습니다")
		: TEXT("같은 티어 룬 3개가 필요합니다 (ROHRunes로 보유 확인)"));
}

void UROHCheatManager::ROHRunes()
{
	UROHItemDatabase* Database = GetDatabase(this);
	UROHInventoryComponent* Inventory = GetPlayerInventory(this);
	if (!Database || !Inventory)
	{
		return;
	}

	TMap<FName, int32> Counts;
	for (const FROHItemInstance& Item : Inventory->GetItems())
	{
		if (const FROHRuneDef* Rune = Database->FindRuneByBaseId(Item.BaseId))
		{
			++Counts.FindOrAdd(Rune->RuneId);
		}
	}

	CheatPrint(TEXT("=== 룬 12종 (3개 → 상위 1개: ROHTransmute) ==="));
	for (const FROHRuneDef& Rune : Database->GetRunes())
	{
		const int32* Count = Counts.Find(Rune.RuneId);
		CheatPrint(FString::Printf(TEXT("T%02d %s(%s): %s +%.0f | 룬위력 +%.1f%% | 보유 %d"),
			Rune.Tier, *Rune.DisplayName.ToString(), *Rune.RuneId.ToString(),
			*Rune.BonusAttribute.GetName(), Rune.BonusValue, Rune.RunePower, Count ? *Count : 0));
	}

	auto SlotName = [](EROHEquipSlot Slot) -> const TCHAR*
	{
		switch (Slot)
		{
		case EROHEquipSlot::Weapon: return TEXT("무기");
		case EROHEquipSlot::Shield: return TEXT("방패");
		case EROHEquipSlot::Helm:   return TEXT("투구");
		case EROHEquipSlot::Chest:  return TEXT("흉갑");
		case EROHEquipSlot::Boots:  return TEXT("장화");
		default:                    return TEXT("?");
		}
	};

	CheatPrint(TEXT("=== 룬워드 8종 (일반 등급 + 소켓 수/순서 일치 시 완성) ==="));
	for (const FROHRunewordDef& Runeword : Database->GetRunewords())
	{
		TArray<FString> Sequence;
		for (const FName& RuneId : Runeword.RuneSequence)
		{
			const FROHRuneDef* Rune = Database->FindRune(RuneId);
			Sequence.Add(Rune ? Rune->DisplayName.ToString() : RuneId.ToString());
		}
		CheatPrint(FString::Printf(TEXT("[%s] %s — %s"),
			*Runeword.DisplayName.ToString(), SlotName(Runeword.RequiredSlot), *FString::Join(Sequence, TEXT("+"))));
	}
}

void UROHCheatManager::ROHSocket(int32 ItemIndex, int32 RuneItemIndex)
{
	UROHInventoryComponent* Inventory = GetPlayerInventory(this);
	if (!Inventory)
	{
		return;
	}
	FString Error;
	if (Inventory->SocketRune(ItemIndex, RuneItemIndex, Error))
	{
		CheatPrint(TEXT("소켓 완료 — ROHDumpInventory로 확인"));
	}
	else
	{
		CheatPrint(FString::Printf(TEXT("소켓 실패: %s"), *Error));
	}
}

void UROHCheatManager::ROHSalvage(int32 ItemIndex)
{
	if (UROHInventoryComponent* Inventory = GetPlayerInventory(this))
	{
		FString Message;
		Inventory->SalvageUnique(ItemIndex, Message);
		CheatPrint(Message);
	}
}

void UROHCheatManager::ROHForgeAncient(int32 ItemIndex)
{
	if (UROHInventoryComponent* Inventory = GetPlayerInventory(this))
	{
		FString Message;
		Inventory->ForgeAncient(ItemIndex, Message);
		CheatPrint(Message);
	}
}

void UROHCheatManager::ROHUniques()
{
	UROHItemDatabase* Database = GetDatabase(this);
	if (!Database)
	{
		return;
	}

	CheatPrint(TEXT("=== 유니크 15종 (드랍 1%+MF | 분해: ROHSalvage → 고대 합성: ROHForgeAncient) ==="));
	for (const FROHUniqueDef& Unique : Database->GetUniques())
	{
		const FROHItemBaseDef* Base = Database->FindBase(Unique.BaseId);
		TArray<FString> BonusParts;
		for (const FROHRunewordBonus& Bonus : Unique.Bonuses)
		{
			BonusParts.Add(FString::Printf(TEXT("%s +%.0f"), *Bonus.Attribute.GetName(), Bonus.Value));
		}
		if (Unique.RunePower > 0.f)
		{
			BonusParts.Add(FString::Printf(TEXT("룬위력 +%.0f%%"), Unique.RunePower));
		}
		CheatPrint(FString::Printf(TEXT("%s (%s, ilvl %d+): %s"),
			*Unique.DisplayName.ToString(),
			Base ? *Base->DisplayName.ToString() : *Unique.BaseId.ToString(),
			Unique.RequiredItemLevel, *FString::Join(BonusParts, TEXT(", "))));
	}
}

void UROHCheatManager::ROHSets()
{
	UROHItemDatabase* Database = GetDatabase(this);
	if (!Database)
	{
		return;
	}

	CheatPrint(TEXT("=== 세트 4종 (드랍 1.5%+MF | 장착 피스 수만큼 보너스 누적) ==="));
	for (const FROHSetDef& Set : Database->GetSets())
	{
		CheatPrint(FString::Printf(TEXT("[%s] (ilvl %d+, %d피스)"),
			*Set.DisplayName.ToString(), Set.RequiredItemLevel, Set.Pieces.Num()));
		for (const FROHSetPieceDef& Piece : Set.Pieces)
		{
			const FROHItemBaseDef* Base = Database->FindBase(Piece.BaseId);
			CheatPrint(FString::Printf(TEXT("  - %s (%s)"),
				*Piece.DisplayName.ToString(), Base ? *Base->DisplayName.ToString() : *Piece.BaseId.ToString()));
		}
		// 임계 오름차순 출력
		TArray<int32> Thresholds;
		Set.CountBonuses.GetKeys(Thresholds);
		Thresholds.Sort();
		for (const int32 Threshold : Thresholds)
		{
			TArray<FString> BonusParts;
			for (const FROHRunewordBonus& Bonus : Set.CountBonuses[Threshold])
			{
				BonusParts.Add(FString::Printf(TEXT("%s +%.0f"), *Bonus.Attribute.GetName(), Bonus.Value));
			}
			CheatPrint(FString::Printf(TEXT("  %d피스: %s"), Threshold, *FString::Join(BonusParts, TEXT(", "))));
		}
		if (Set.FullSetRunePower > 0.f)
		{
			CheatPrint(FString::Printf(TEXT("  풀세트: 룬위력 +%.0f%%"), Set.FullSetRunePower));
		}
	}
}

void UROHCheatManager::ROHGamble()
{
	const APlayerController* PC = GetOuterAPlayerController();
	AROHPlayerCharacter* Player = Cast<AROHPlayerCharacter>(PC ? PC->GetPawn() : nullptr);
	UROHInventoryComponent* Inventory = Player ? Player->GetInventory() : nullptr;
	if (!Player || !Inventory)
	{
		return;
	}

	FString Message;
	bool bJackpot = false;
	Inventory->GambleWithGems(Message, bJackpot);
	CheatPrint(Message);

	if (bJackpot && Inventory->GetItems().Num() > 0)
	{
		// 결과는 방금 AddItem으로 마지막에 추가됨 — 이름을 뽑아 연출 호출
		const UROHItemDatabase* Database = GetDatabase(this);
		const FString ItemName = Database
			? Database->GetItemDisplayName(Inventory->GetItems().Last()).ToString() : FString();
		Player->PlayAncientCelebration(ItemName);
	}
}

void UROHCheatManager::ROHGiveGem(int32 Count)
{
	UROHItemDatabase* Database = GetDatabase(this);
	UROHInventoryComponent* Inventory = GetPlayerInventory(this);
	if (!Database || !Inventory)
	{
		return;
	}

	Count = FMath::Clamp(Count, 1, 40);
	int32 Given = 0;
	for (int32 i = 0; i < Count; ++i)
	{
		if (Inventory->AddItem(Database->GenerateItem(TEXT("FateGem"), 1, EROHItemQuality::Normal)))
		{
			++Given;
		}
	}
	CheatPrint(FString::Printf(TEXT("획득: 운명의 보석 ×%d (도박: ROHGamble — %d개 소모)"),
		Given, UROHInventoryComponent::GambleGemCost));
}

void UROHCheatManager::ROHIdentify()
{
	if (UROHInventoryComponent* Inventory = GetPlayerInventory(this))
	{
		const int32 Identified = Inventory->IdentifyAll();
		CheatPrint(Identified > 0
			? FString::Printf(TEXT("감정 완료: %d개 (정식 창구: 마을 셀바 — 개당 50골드)"), Identified)
			: TEXT("미감정 아이템이 없습니다"));
	}
}
