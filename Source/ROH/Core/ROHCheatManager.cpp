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

	CheatPrint(TEXT("--- 장비창 ---"));
	for (const auto& Pair : Inventory->GetEquipped())
	{
		FString AffixText;
		for (const FROHAffixRoll& Affix : Pair.Value.Affixes)
		{
			AffixText += FString::Printf(TEXT(" [%s +%.0f]"), *Affix.AffixId.ToString(), Affix.Value);
		}
		CheatPrint(FString::Printf(TEXT("슬롯 %d: %s%s"),
			static_cast<int32>(Pair.Key),
			*Database->GetItemDisplayName(Pair.Value).ToString(), *AffixText));
	}

	CheatPrint(TEXT("--- 인벤토리 ---"));
	const TArray<FROHItemInstance>& Items = Inventory->GetItems();
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		CheatPrint(FString::Printf(TEXT("%d: %s (ilvl %d, 접사 %d)"),
			i, *Database->GetItemDisplayName(Items[i]).ToString(),
			Items[i].ItemLevel, Items[i].Affixes.Num()));
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
	int32 QualityCounts[3] = { 0, 0, 0 }; // Normal, Magic, Rare
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
			const int32 QualityIndex = FMath::Clamp(static_cast<int32>(Item.Quality), 0, 2);
			++QualityCounts[QualityIndex];
			++BaseCounts.FindOrAdd(Item.BaseId);
		}
	}

	CheatPrint(FString::Printf(TEXT("=== 드랍 시뮬레이션: %s × %d회 ==="), *TCId.ToString(), Count));
	CheatPrint(FString::Printf(TEXT("아이템 %d개 (%.1f%%) | 골드 드랍 %d회, 평균 %.1f"),
		TotalItems, 100.f * TotalItems / Count, GoldDrops, GoldDrops > 0 ? static_cast<float>(TotalGold) / GoldDrops : 0.f));
	CheatPrint(FString::Printf(TEXT("등급: 일반 %d / 매직 %d / 레어 %d"),
		QualityCounts[0], QualityCounts[1], QualityCounts[2]));
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
