#include "Items/ROHItemDatabase.h"
#include "Character/ROHAttributeSet.h"
#include "ROH.h"

void UROHItemDatabase::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	BuildDefaultData();
}

void UROHItemDatabase::BuildDefaultData()
{
	Bases.Reset();
	Affixes.Reset();
	TreasureClasses.Reset();

	// ---------- 베이스 아이템 ----------
	auto AddBase = [this](FName Id, const TCHAR* Name, EROHItemKind Kind, EROHEquipSlot Slot,
		float DmgMin, float DmgMax, float Armor, int32 ReqLevel, float Heal, int32 Gold)
	{
		FROHItemBaseDef Def;
		Def.BaseId = Id;
		Def.DisplayName = FText::FromString(Name);
		Def.Kind = Kind;
		Def.Slot = Slot;
		Def.DamageMin = DmgMin;
		Def.DamageMax = DmgMax;
		Def.Armor = Armor;
		Def.RequiredLevel = ReqLevel;
		Def.PotionHealAmount = Heal;
		Def.GoldValue = Gold;
		Bases.Add(Id, Def);
	};

	//        Id                 이름           종류                       슬롯                  DmgMin DmgMax Armor Lvl Heal Gold
	AddBase("ShortSword",   TEXT("단검"),     EROHItemKind::Equipment, EROHEquipSlot::Weapon, 4.f, 8.f, 0.f, 1, 0.f, 30);
	AddBase("BattleAxe",    TEXT("전투도끼"), EROHItemKind::Equipment, EROHEquipSlot::Weapon, 8.f, 16.f, 0.f, 5, 0.f, 90);
	AddBase("Buckler",      TEXT("버클러"),   EROHItemKind::Equipment, EROHEquipSlot::Shield, 0.f, 0.f, 10.f, 1, 0.f, 25);
	AddBase("RoundShield",  TEXT("원형방패"), EROHItemKind::Equipment, EROHEquipSlot::Shield, 0.f, 0.f, 22.f, 5, 0.f, 80);
	AddBase("Cap",          TEXT("가죽모자"), EROHItemKind::Equipment, EROHEquipSlot::Helm,   0.f, 0.f, 6.f, 1, 0.f, 20);
	AddBase("FullHelm",     TEXT("투구"),     EROHItemKind::Equipment, EROHEquipSlot::Helm,   0.f, 0.f, 15.f, 5, 0.f, 70);
	AddBase("LeatherArmor", TEXT("가죽갑옷"), EROHItemKind::Equipment, EROHEquipSlot::Chest,  0.f, 0.f, 14.f, 1, 0.f, 40);
	AddBase("ChainMail",    TEXT("사슬갑옷"), EROHItemKind::Equipment, EROHEquipSlot::Chest,  0.f, 0.f, 30.f, 6, 0.f, 120);
	AddBase("LeatherBoots", TEXT("가죽장화"), EROHItemKind::Equipment, EROHEquipSlot::Boots,  0.f, 0.f, 5.f, 1, 0.f, 20);
	AddBase("HealthPotion", TEXT("치유물약"), EROHItemKind::Potion,    EROHEquipSlot::None,   0.f, 0.f, 0.f, 1, 60.f, 50);

	// ---------- 접사 풀 ----------
	auto AddAffix = [this](FName Id, const TCHAR* Name, bool bPrefix, FGameplayAttribute Attr,
		float Min, float Max, int32 ReqIlvl, std::initializer_list<EROHEquipSlot> Slots)
	{
		FROHAffixDef Def;
		Def.AffixId = Id;
		Def.DisplayName = FText::FromString(Name);
		Def.bPrefix = bPrefix;
		Def.Attribute = Attr;
		Def.MinValue = Min;
		Def.MaxValue = Max;
		Def.RequiredItemLevel = ReqIlvl;
		Def.AllowedSlots = Slots;
		Affixes.Add(Id, Def);
	};

	// 접두사 (공격/방어 성향)
	AddAffix("Sharp",    TEXT("날카로운"), true, UROHAttributeSet::GetAttackPowerAttribute(), 2.f, 6.f, 1, { EROHEquipSlot::Weapon });
	AddAffix("Brutal",   TEXT("잔혹한"),   true, UROHAttributeSet::GetAttackPowerAttribute(), 6.f, 14.f, 6, { EROHEquipSlot::Weapon });
	AddAffix("Sturdy",   TEXT("견고한"),   true, UROHAttributeSet::GetDefenseAttribute(), 5.f, 15.f, 1, { EROHEquipSlot::Shield, EROHEquipSlot::Helm, EROHEquipSlot::Chest, EROHEquipSlot::Boots });
	AddAffix("Fortified",TEXT("강화된"),   true, UROHAttributeSet::GetDefenseAttribute(), 15.f, 35.f, 6, { EROHEquipSlot::Shield, EROHEquipSlot::Helm, EROHEquipSlot::Chest, EROHEquipSlot::Boots });
	AddAffix("Precise",  TEXT("정밀한"),   true, UROHAttributeSet::GetAttackRatingAttribute(), 15.f, 50.f, 1, { EROHEquipSlot::Weapon, EROHEquipSlot::Helm });
	AddAffix("Lucky",    TEXT("행운의"),   true, UROHAttributeSet::GetMagicFindAttribute(), 5.f, 15.f, 3, {});

	// 접미사 (스탯/저항/자원)
	AddAffix("OfStrength",  TEXT("힘의"),     false, UROHAttributeSet::GetStrengthAttribute(), 2.f, 8.f, 1, {});
	AddAffix("OfDexterity", TEXT("민첩의"),   false, UROHAttributeSet::GetDexterityAttribute(), 2.f, 8.f, 1, {});
	AddAffix("OfVitality",  TEXT("활력의"),   false, UROHAttributeSet::GetVitalityAttribute(), 2.f, 8.f, 3, {});
	AddAffix("OfLife",      TEXT("생명의"),   false, UROHAttributeSet::GetMaxHealthAttribute(), 10.f, 30.f, 1, {});
	AddAffix("OfMana",      TEXT("마나의"),   false, UROHAttributeSet::GetMaxManaAttribute(), 8.f, 20.f, 1, {});
	AddAffix("OfFire",      TEXT("화염막이"), false, UROHAttributeSet::GetFireResistanceAttribute(), 5.f, 20.f, 2, {});
	AddAffix("OfFrost",     TEXT("냉기막이"), false, UROHAttributeSet::GetColdResistanceAttribute(), 5.f, 20.f, 2, {});
	AddAffix("OfStorm",     TEXT("번개막이"), false, UROHAttributeSet::GetLightningResistanceAttribute(), 5.f, 20.f, 2, {});
	AddAffix("OfHaste",     TEXT("신속의"),   false, UROHAttributeSet::GetMoveSpeedAttribute(), 20.f, 60.f, 4, { EROHEquipSlot::Boots });

	// ---------- 트레저 클래스 ----------
	auto MakeEntry = [](EROHTreasureEntryType Type, FName Ref, int32 Weight, int32 GoldMin = 5, int32 GoldMax = 25)
	{
		FROHTreasureEntry Entry;
		Entry.Type = Type;
		Entry.Ref = Ref;
		Entry.Weight = Weight;
		Entry.GoldMin = GoldMin;
		Entry.GoldMax = GoldMax;
		return Entry;
	};

	{
		FROHTreasureClassDef WeaponsTC;
		WeaponsTC.TCId = "TC_Weapons";
		WeaponsTC.Entries.Add(MakeEntry(EROHTreasureEntryType::BaseItem, "ShortSword", 60));
		WeaponsTC.Entries.Add(MakeEntry(EROHTreasureEntryType::BaseItem, "BattleAxe", 40));
		TreasureClasses.Add(WeaponsTC.TCId, WeaponsTC);
	}
	{
		FROHTreasureClassDef ArmorTC;
		ArmorTC.TCId = "TC_Armor";
		ArmorTC.Entries.Add(MakeEntry(EROHTreasureEntryType::BaseItem, "Buckler", 15));
		ArmorTC.Entries.Add(MakeEntry(EROHTreasureEntryType::BaseItem, "RoundShield", 10));
		ArmorTC.Entries.Add(MakeEntry(EROHTreasureEntryType::BaseItem, "Cap", 15));
		ArmorTC.Entries.Add(MakeEntry(EROHTreasureEntryType::BaseItem, "FullHelm", 10));
		ArmorTC.Entries.Add(MakeEntry(EROHTreasureEntryType::BaseItem, "LeatherArmor", 20));
		ArmorTC.Entries.Add(MakeEntry(EROHTreasureEntryType::BaseItem, "ChainMail", 10));
		ArmorTC.Entries.Add(MakeEntry(EROHTreasureEntryType::BaseItem, "LeatherBoots", 20));
		TreasureClasses.Add(ArmorTC.TCId, ArmorTC);
	}
	{
		// 일반 몬스터 기본 TC (docs/02 §3.4 예시 구조)
		FROHTreasureClassDef DefaultTC;
		DefaultTC.TCId = "TC_Default";
		DefaultTC.Entries.Add(MakeEntry(EROHTreasureEntryType::NoDrop, NAME_None, 35));
		DefaultTC.Entries.Add(MakeEntry(EROHTreasureEntryType::Gold, NAME_None, 25, 5, 30));
		DefaultTC.Entries.Add(MakeEntry(EROHTreasureEntryType::SubTable, "TC_Weapons", 12));
		DefaultTC.Entries.Add(MakeEntry(EROHTreasureEntryType::SubTable, "TC_Armor", 18));
		DefaultTC.Entries.Add(MakeEntry(EROHTreasureEntryType::BaseItem, "HealthPotion", 10));
		TreasureClasses.Add(DefaultTC.TCId, DefaultTC);
	}
	{
		// 보스/챔피언용: NoDrop 없음 + 3회 추첨
		FROHTreasureClassDef BossTC;
		BossTC.TCId = "TC_Boss";
		BossTC.Picks = 3;
		BossTC.Entries.Add(MakeEntry(EROHTreasureEntryType::Gold, NAME_None, 25, 30, 100));
		BossTC.Entries.Add(MakeEntry(EROHTreasureEntryType::SubTable, "TC_Weapons", 30));
		BossTC.Entries.Add(MakeEntry(EROHTreasureEntryType::SubTable, "TC_Armor", 35));
		BossTC.Entries.Add(MakeEntry(EROHTreasureEntryType::BaseItem, "HealthPotion", 10));
		TreasureClasses.Add(BossTC.TCId, BossTC);
	}

	UE_LOG(LogROH, Log, TEXT("ItemDatabase: 베이스 %d, 접사 %d, TC %d 등록"),
		Bases.Num(), Affixes.Num(), TreasureClasses.Num());
}

const FROHItemBaseDef* UROHItemDatabase::FindBase(FName BaseId) const
{
	return Bases.Find(BaseId);
}

const FROHAffixDef* UROHItemDatabase::FindAffix(FName AffixId) const
{
	return Affixes.Find(AffixId);
}

const FROHTreasureClassDef* UROHItemDatabase::FindTreasureClass(FName TCId) const
{
	return TreasureClasses.Find(TCId);
}

EROHItemQuality UROHItemDatabase::RollQuality(int32 ItemLevel, float MagicFind, FRandomStream& Rng) const
{
	// MF 체감 곡선 (디아블로2 방식): 실효 MF = MF × 250 / (MF + 250)
	const float EffectiveMF = MagicFind > 0.f ? MagicFind * 250.f / (MagicFind + 250.f) : 0.f;

	const float RareChance = 0.05f * (1.f + EffectiveMF / 100.f);
	const float MagicChance = 0.20f * (1.f + EffectiveMF / 100.f);

	const float Roll = Rng.FRand();
	if (Roll < RareChance)
	{
		return EROHItemQuality::Rare;
	}
	if (Roll < RareChance + MagicChance)
	{
		return EROHItemQuality::Magic;
	}
	return EROHItemQuality::Normal;
}

FROHItemInstance UROHItemDatabase::GenerateItem(FName BaseId, int32 ItemLevel, EROHItemQuality Quality, int32 Seed) const
{
	FROHItemInstance Instance;

	const FROHItemBaseDef* Base = FindBase(BaseId);
	if (!Base)
	{
		UE_LOG(LogROH, Warning, TEXT("GenerateItem: 알 수 없는 베이스 '%s'"), *BaseId.ToString());
		return Instance;
	}

	Instance.InstanceId = FGuid::NewGuid();
	Instance.BaseId = BaseId;
	Instance.ItemLevel = FMath::Max(1, ItemLevel);
	Instance.Quality = (Base->Kind == EROHItemKind::Equipment) ? Quality : EROHItemQuality::Normal;
	Instance.Seed = (Seed != 0) ? Seed : FMath::Rand();

	if (Base->Kind == EROHItemKind::Equipment && Instance.Quality != EROHItemQuality::Normal)
	{
		FRandomStream Rng(Instance.Seed);
		RollAffixes(Instance, *Base, Rng);
	}
	return Instance;
}

void UROHItemDatabase::RollAffixes(FROHItemInstance& Instance, const FROHItemBaseDef& Base, FRandomStream& Rng) const
{
	int32 NumPrefixes = 0;
	int32 NumSuffixes = 0;

	if (Instance.Quality == EROHItemQuality::Magic)
	{
		// 접두/접미 각각 50% 확률, 최소 1개 보장
		NumPrefixes = Rng.FRand() < 0.5f ? 1 : 0;
		NumSuffixes = Rng.FRand() < 0.5f ? 1 : 0;
		if (NumPrefixes + NumSuffixes == 0)
		{
			(Rng.FRand() < 0.5f ? NumPrefixes : NumSuffixes) = 1;
		}
	}
	else if (Instance.Quality == EROHItemQuality::Rare)
	{
		// 3~6개, 접두/접미 각 최대 3
		const int32 Total = Rng.RandRange(3, 6);
		NumPrefixes = FMath::Clamp(Rng.RandRange(1, Total - 1), 1, 3);
		NumSuffixes = FMath::Clamp(Total - NumPrefixes, 1, 3);
	}

	auto RollFromPool = [&](bool bPrefix, int32 Count)
	{
		TArray<const FROHAffixDef*> Pool = GetEligibleAffixes(Base, Instance.ItemLevel, bPrefix);
		for (int32 i = 0; i < Count && Pool.Num() > 0; ++i)
		{
			const int32 PickIndex = Rng.RandRange(0, Pool.Num() - 1);
			const FROHAffixDef* Def = Pool[PickIndex];
			Pool.RemoveAt(PickIndex); // 동일 접사 중복 방지

			FROHAffixRoll Roll;
			Roll.AffixId = Def->AffixId;
			Roll.Attribute = Def->Attribute;
			Roll.Value = FMath::RoundToFloat(Rng.FRandRange(Def->MinValue, Def->MaxValue));
			Instance.Affixes.Add(Roll);
		}
	};

	RollFromPool(true, NumPrefixes);
	RollFromPool(false, NumSuffixes);
}

TArray<const FROHAffixDef*> UROHItemDatabase::GetEligibleAffixes(const FROHItemBaseDef& Base, int32 ItemLevel, bool bPrefix) const
{
	TArray<const FROHAffixDef*> Result;
	for (const auto& Pair : Affixes)
	{
		const FROHAffixDef& Def = Pair.Value;
		if (Def.bPrefix != bPrefix || Def.RequiredItemLevel > ItemLevel)
		{
			continue;
		}
		if (Def.AllowedSlots.Num() > 0 && !Def.AllowedSlots.Contains(Base.Slot))
		{
			continue;
		}
		Result.Add(&Def);
	}
	return Result;
}

FROHDropResult UROHItemDatabase::RollTreasureClass(FName TCId, int32 ItemLevel, float MagicFind) const
{
	FROHDropResult Result;

	const FROHTreasureClassDef* TC = FindTreasureClass(TCId);
	if (!TC)
	{
		UE_LOG(LogROH, Warning, TEXT("RollTreasureClass: 알 수 없는 TC '%s'"), *TCId.ToString());
		return Result;
	}

	FRandomStream Rng(FMath::Rand());

	for (int32 Pick = 0; Pick < TC->Picks; ++Pick)
	{
		// 계층 추첨 (하위 TC는 최대 8단계까지 안전 순회)
		const FROHTreasureClassDef* Current = TC;
		for (int32 Depth = 0; Depth < 8 && Current; ++Depth)
		{
			int32 TotalWeight = 0;
			for (const FROHTreasureEntry& Entry : Current->Entries)
			{
				TotalWeight += FMath::Max(0, Entry.Weight);
			}
			if (TotalWeight <= 0)
			{
				break;
			}

			int32 Roll = Rng.RandRange(0, TotalWeight - 1);
			const FROHTreasureEntry* Chosen = nullptr;
			for (const FROHTreasureEntry& Entry : Current->Entries)
			{
				Roll -= FMath::Max(0, Entry.Weight);
				if (Roll < 0)
				{
					Chosen = &Entry;
					break;
				}
			}
			if (!Chosen)
			{
				break;
			}

			if (Chosen->Type == EROHTreasureEntryType::NoDrop)
			{
				break;
			}
			if (Chosen->Type == EROHTreasureEntryType::Gold)
			{
				Result.Gold += Rng.RandRange(Chosen->GoldMin, Chosen->GoldMax);
				break;
			}
			if (Chosen->Type == EROHTreasureEntryType::BaseItem)
			{
				const EROHItemQuality Quality = RollQuality(ItemLevel, MagicFind, Rng);
				FROHItemInstance Item = GenerateItem(Chosen->Ref, ItemLevel, Quality, Rng.RandRange(1, MAX_int32 - 1));
				if (Item.IsValid())
				{
					Result.Items.Add(Item);
				}
				break;
			}
			// SubTable → 하위 TC로 내려가서 재추첨
			Current = FindTreasureClass(Chosen->Ref);
		}
	}
	return Result;
}

FText UROHItemDatabase::GetItemDisplayName(const FROHItemInstance& Instance) const
{
	const FROHItemBaseDef* Base = FindBase(Instance.BaseId);
	if (!Base)
	{
		return FText::FromString(TEXT("???"));
	}

	// 매직: "접두 베이스" 또는 "베이스 (접미)" / 레어: 고정 칭호
	if (Instance.Quality == EROHItemQuality::Rare)
	{
		return FText::Format(NSLOCTEXT("ROH", "RareItemName", "빛나는 {0}"), Base->DisplayName);
	}
	if (Instance.Quality == EROHItemQuality::Magic && Instance.Affixes.Num() > 0)
	{
		if (const FROHAffixDef* First = FindAffix(Instance.Affixes[0].AffixId))
		{
			return FText::Format(NSLOCTEXT("ROH", "MagicItemName", "{0} {1}"), First->DisplayName, Base->DisplayName);
		}
	}
	return Base->DisplayName;
}

FColor UROHItemDatabase::GetQualityColor(EROHItemQuality Quality)
{
	switch (Quality)
	{
	case EROHItemQuality::Magic:    return FColor(80, 120, 255);
	case EROHItemQuality::Rare:     return FColor(255, 220, 60);
	case EROHItemQuality::Unique:   return FColor(200, 160, 80);
	case EROHItemQuality::Runeword: return FColor(255, 140, 40);
	default:                        return FColor::White;
	}
}
