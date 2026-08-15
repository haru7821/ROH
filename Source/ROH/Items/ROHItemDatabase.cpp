#include "Items/ROHItemDatabase.h"
#include "Character/ROHAttributeSet.h"
#include "HAL/PlatformTime.h"
#include "ROH.h"
#include <initializer_list>

namespace
{
	// FMath::Rand()는 상태 공간이 ~32767뿐이라 시드로 부적합 → 사이클 카운터와 결합
	int32 MakeRandomSeed()
	{
		const int32 Seed = static_cast<int32>(FPlatformTime::Cycles() & 0x7fffffff) ^ (FMath::Rand() << 16);
		return Seed != 0 ? Seed : 1;
	}
}

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
	Runes.Reset();
	Runewords.Reset();

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

	// ---------- 룬 12종 (M5 — docs/06 아자크론의 33 룬 봉인 중 발굴된 12종) ----------
	// 소켓 보너스(단일 어트리뷰트) + 룬 위력(티어×0.5%, 최종 피해 곱연산 합산원 — docs/10 §5.2)
	auto AddRune = [this, &AddBase](FName RuneId, const TCHAR* Name, int32 Tier, FGameplayAttribute Attr, float Value)
	{
		FROHRuneDef Def;
		Def.RuneId = RuneId;
		Def.DisplayName = FText::FromString(Name);
		Def.Tier = Tier;
		Def.BonusAttribute = Attr;
		Def.BonusValue = Value;
		Def.RunePower = Tier * 0.5f;
		check(Def.Tier == Runes.Num() + 1); // FindRuneByTier의 인덱스 계약: 등록 순서 = 티어 순서
		Runes.Add(Def);

		// 룬은 아이템: 인벤토리 보관/드랍/합성 재료 (골드 가치는 티어 비례)
		AddBase(GetRuneBaseId(RuneId), *FString::Printf(TEXT("%s 룬"), Name),
			EROHItemKind::Rune, EROHEquipSlot::None, 0.f, 0.f, 0.f, 1, 0.f, 50 * Tier);
	};

	//      Id       이름          티어  소켓 보너스
	AddRune("En",   TEXT("엔"),     1,  UROHAttributeSet::GetAttackRatingAttribute(), 5.f);
	AddRune("Od",   TEXT("오드"),   2,  UROHAttributeSet::GetMaxHealthAttribute(), 8.f);
	AddRune("Kar",  TEXT("카르"),   3,  UROHAttributeSet::GetAttackPowerAttribute(), 2.f);
	AddRune("Mun",  TEXT("문"),     4,  UROHAttributeSet::GetMaxManaAttribute(), 6.f);
	AddRune("Rea",  TEXT("레아"),   5,  UROHAttributeSet::GetFireResistanceAttribute(), 5.f);
	AddRune("Tis",  TEXT("티스"),   6,  UROHAttributeSet::GetColdResistanceAttribute(), 5.f);
	AddRune("Har",  TEXT("하르"),   7,  UROHAttributeSet::GetLightningResistanceAttribute(), 5.f);
	AddRune("Bel",  TEXT("벨"),     8,  UROHAttributeSet::GetHealthRegenAttribute(), 1.f);
	AddRune("Gul",  TEXT("굴"),     9,  UROHAttributeSet::GetDefenseAttribute(), 8.f);
	AddRune("Zar",  TEXT("자르"),   10, UROHAttributeSet::GetCritChanceAttribute(), 3.f);
	AddRune("Keon", TEXT("케온"),   11, UROHAttributeSet::GetAttackSpeedPctAttribute(), 10.f);
	AddRune("Azak", TEXT("아자크"), 12, UROHAttributeSet::GetRunePowerAttribute(), 5.f); // 정점 룬: 보너스도 룬 위력 (중첩 의도)

	// ---------- 룬워드 8종 (M5 — 일반 등급 + 소켓 수/순서 일치 시 완성) ----------
	auto AddRuneword = [this](FName Id, const TCHAR* Name, EROHEquipSlot Slot,
		std::initializer_list<FName> Sequence, std::initializer_list<FROHRunewordBonus> Bonuses, float RunePower)
	{
		FROHRunewordDef Def;
		Def.RunewordId = Id;
		Def.DisplayName = FText::FromString(Name);
		Def.RequiredSlot = Slot;
		Def.RuneSequence = Sequence;
		Def.Bonuses = Bonuses;
		Def.RunePower = RunePower;
		Runewords.Add(Def);
	};
	auto RB = [](FGameplayAttribute Attr, float Value)
	{
		FROHRunewordBonus Bonus;
		Bonus.Attribute = Attr;
		Bonus.Value = Value;
		return Bonus;
	};

	AddRuneword("Oath",     TEXT("맹세"), EROHEquipSlot::Weapon, { "En", "Kar" },
		{ RB(UROHAttributeSet::GetAttackPowerAttribute(), 10.f),
		  RB(UROHAttributeSet::GetAttackRatingAttribute(), 25.f) }, 0.f);
	AddRuneword("Wisdom",   TEXT("지혜"), EROHEquipSlot::Helm, { "Mun", "Tis" },
		{ RB(UROHAttributeSet::GetMaxManaAttribute(), 30.f),
		  RB(UROHAttributeSet::GetCastSpeedPctAttribute(), 10.f),
		  RB(UROHAttributeSet::GetMagicFindAttribute(), 15.f) }, 0.f);
	AddRuneword("Guard",    TEXT("수호"), EROHEquipSlot::Shield, { "Od", "Tis" },
		{ RB(UROHAttributeSet::GetMaxHealthAttribute(), 25.f),
		  RB(UROHAttributeSet::GetColdResistanceAttribute(), 15.f),
		  RB(UROHAttributeSet::GetDefenseAttribute(), 15.f) }, 0.f);
	AddRuneword("Embers",   TEXT("잿불"), EROHEquipSlot::Chest, { "Rea", "Mun" },
		{ RB(UROHAttributeSet::GetFireResistanceAttribute(), 20.f),
		  RB(UROHAttributeSet::GetMaxManaAttribute(), 20.f),
		  RB(UROHAttributeSet::GetManaRegenAttribute(), 2.f) }, 0.f);
	AddRuneword("Bastion",  TEXT("성채"), EROHEquipSlot::Chest, { "Od", "Gul", "Rea" },
		{ RB(UROHAttributeSet::GetMaxHealthAttribute(), 50.f),
		  RB(UROHAttributeSet::GetDefenseAttribute(), 30.f),
		  RB(UROHAttributeSet::GetFireResistanceAttribute(), 15.f) }, 0.f);
	AddRuneword("Tempest",  TEXT("폭풍"), EROHEquipSlot::Weapon, { "Har", "Keon", "Zar" },
		{ RB(UROHAttributeSet::GetLightningResistanceAttribute(), 10.f),
		  RB(UROHAttributeSet::GetAttackSpeedPctAttribute(), 15.f),
		  RB(UROHAttributeSet::GetCritChanceAttribute(), 5.f) }, 5.f);
	AddRuneword("Nether",   TEXT("명계"), EROHEquipSlot::Shield, { "Gul", "Zar", "Azak" },
		{ RB(UROHAttributeSet::GetDefenseAttribute(), 40.f),
		  RB(UROHAttributeSet::GetShadowResistanceAttribute(), 15.f),
		  RB(UROHAttributeSet::GetCritChanceAttribute(), 3.f) }, 6.f);
	AddRuneword("Carnage",  TEXT("학살"), EROHEquipSlot::Weapon, { "Kar", "Bel", "Azak" },
		{ RB(UROHAttributeSet::GetAttackPowerAttribute(), 20.f),
		  RB(UROHAttributeSet::GetCritDamageAttribute(), 30.f) }, 8.f);

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
	AddAffix("Keen",     TEXT("예리한"),   true, UROHAttributeSet::GetCritChanceAttribute(), 2.f, 6.f, 3, { EROHEquipSlot::Weapon });
	AddAffix("Deadly",   TEXT("치명적인"), true, UROHAttributeSet::GetCritDamageAttribute(), 10.f, 30.f, 5, { EROHEquipSlot::Weapon });

	// 접미사 (스탯/저항/자원)
	AddAffix("OfStrength",  TEXT("힘의"),     false, UROHAttributeSet::GetStrengthAttribute(), 2.f, 8.f, 1, {});
	AddAffix("OfDexterity", TEXT("민첩의"),   false, UROHAttributeSet::GetDexterityAttribute(), 2.f, 8.f, 1, {});
	AddAffix("OfVitality",  TEXT("활력의"),   false, UROHAttributeSet::GetVitalityAttribute(), 2.f, 8.f, 3, {});
	AddAffix("OfLife",      TEXT("생명의"),   false, UROHAttributeSet::GetMaxHealthAttribute(), 10.f, 30.f, 1, {});
	AddAffix("OfMana",      TEXT("마나의"),   false, UROHAttributeSet::GetMaxManaAttribute(), 8.f, 20.f, 1, {});
	AddAffix("OfFire",      TEXT("화염막이"), false, UROHAttributeSet::GetFireResistanceAttribute(), 5.f, 20.f, 2, {});
	AddAffix("OfFrost",     TEXT("냉기막이"), false, UROHAttributeSet::GetColdResistanceAttribute(), 5.f, 20.f, 2, {});
	AddAffix("OfStorm",     TEXT("번개막이"), false, UROHAttributeSet::GetLightningResistanceAttribute(), 5.f, 20.f, 2, {});
	AddAffix("OfVenomWard", TEXT("독막이"),   false, UROHAttributeSet::GetPoisonResistanceAttribute(), 5.f, 20.f, 2, {});
	AddAffix("OfShadowWard",TEXT("그림자막이"), false, UROHAttributeSet::GetShadowResistanceAttribute(), 5.f, 20.f, 3, {});
	AddAffix("OfRunes",     TEXT("룬각인의"), false, UROHAttributeSet::GetRunePowerAttribute(), 5.f, 15.f, 5, {}); // 룬 배율 합산원 (docs/10 §5.2)
	AddAffix("OfAlacrity",  TEXT("질풍의"),   false, UROHAttributeSet::GetAttackSpeedPctAttribute(), 5.f, 15.f, 4, { EROHEquipSlot::Weapon }); // 공속 (docs/10 §3.4)
	AddAffix("OfCelerity",  TEXT("쾌속의"),   false, UROHAttributeSet::GetCastSpeedPctAttribute(), 5.f, 15.f, 4, {});
	AddAffix("OfRecovery",  TEXT("회복의"),   false, UROHAttributeSet::GetHealthRegenAttribute(), 1.f, 3.f, 2, {});
	AddAffix("OfClarity",   TEXT("명상의"),   false, UROHAttributeSet::GetManaRegenAttribute(), 1.f, 3.f, 2, {});
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
		DefaultTC.Entries.Add(MakeEntry(EROHTreasureEntryType::Rune, NAME_None, 7)); // M5 룬 드랍
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
		BossTC.Entries.Add(MakeEntry(EROHTreasureEntryType::Rune, NAME_None, 15)); // M5 룬 드랍 (보스 우대)
		TreasureClasses.Add(BossTC.TCId, BossTC);
	}

	UE_LOG(LogROH, Log, TEXT("ItemDatabase: 베이스 %d, 접사 %d, TC %d, 룬 %d, 룬워드 %d 등록"),
		Bases.Num(), Affixes.Num(), TreasureClasses.Num(), Runes.Num(), Runewords.Num());
}

const FROHRuneDef* UROHItemDatabase::FindRune(FName RuneId) const
{
	for (const FROHRuneDef& Def : Runes)
	{
		if (Def.RuneId == RuneId)
		{
			return &Def;
		}
	}
	return nullptr;
}

const FROHRuneDef* UROHItemDatabase::FindRuneByTier(int32 Tier) const
{
	return Runes.IsValidIndex(Tier - 1) ? &Runes[Tier - 1] : nullptr;
}

const FROHRuneDef* UROHItemDatabase::FindRuneByBaseId(FName BaseId) const
{
	// 베이스 명명 규약 "Rune_<Id>" 역파싱
	const FString BaseString = BaseId.ToString();
	if (!BaseString.StartsWith(TEXT("Rune_")))
	{
		return nullptr;
	}
	return FindRune(FName(*BaseString.RightChop(5)));
}

FName UROHItemDatabase::GetRuneBaseId(FName RuneId)
{
	return FName(*FString::Printf(TEXT("Rune_%s"), *RuneId.ToString()));
}

const FROHRunewordDef* UROHItemDatabase::FindRuneword(FName RunewordId) const
{
	for (const FROHRunewordDef& Def : Runewords)
	{
		if (Def.RunewordId == RunewordId)
		{
			return &Def;
		}
	}
	return nullptr;
}

const FROHRunewordDef* UROHItemDatabase::MatchRuneword(const FROHItemInstance& Item) const
{
	// 룬워드는 일반 등급 베이스만 (일반템에 가치 부여 — 디아블로2 관례)
	const FROHItemBaseDef* Base = FindBase(Item.BaseId);
	if (!Base || Item.Quality != EROHItemQuality::Normal)
	{
		return nullptr;
	}
	for (const FROHRunewordDef& Def : Runewords)
	{
		if (Def.RequiredSlot == Base->Slot
			&& Item.MaxSockets == Def.RuneSequence.Num()
			&& Item.SocketedRunes == Def.RuneSequence)
		{
			return &Def;
		}
	}
	return nullptr;
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
	Instance.Seed = (Seed != 0) ? Seed : MakeRandomSeed();

	if (Base->Kind == EROHItemKind::Equipment)
	{
		FRandomStream Rng(Instance.Seed);
		// 접사 먼저 소비 → 소켓: 기존 시드의 접사 재현성 유지 (테스트 ROH.Loot 3번 항목)
		if (Instance.Quality != EROHItemQuality::Normal)
		{
			RollAffixes(Instance, *Base, Rng);
		}
		RollSockets(Instance, *Base, Rng);
	}
	return Instance;
}

void UROHItemDatabase::RollSockets(FROHItemInstance& Instance, const FROHItemBaseDef& Base, FRandomStream& Rng) const
{
	// 소켓 가능 부위: 무기/방패/투구/흉갑 (장화 제외 — 디아블로2 관례)
	if (Base.Slot != EROHEquipSlot::Weapon && Base.Slot != EROHEquipSlot::Shield
		&& Base.Slot != EROHEquipSlot::Helm && Base.Slot != EROHEquipSlot::Chest)
	{
		return;
	}

	// 0/1/2/3개 = 40/30/20/10%, ilvl 게이트 (저레벨 3소켓 룬워드 방지)
	const float Roll = Rng.FRand();
	const int32 Rolled = Roll < 0.4f ? 0 : (Roll < 0.7f ? 1 : (Roll < 0.9f ? 2 : 3));
	const int32 IlvlCap = Instance.ItemLevel < 4 ? 1 : (Instance.ItemLevel < 8 ? 2 : 3);
	Instance.MaxSockets = FMath::Min(Rolled, IlvlCap);
}

FROHItemInstance UROHItemDatabase::GenerateRuneDrop(int32 ItemLevel, FRandomStream& Rng) const
{
	// 티어 상한 = ilvl/4 + 1 (캡 12), 티어당 가중치 반감 → 저티어 위주, 고티어는 고레벨 파밍 동기
	const int32 MaxTier = FMath::Clamp(ItemLevel / 4 + 1, 1, 12);
	int32 TotalWeight = 0;
	for (int32 Tier = 1; Tier <= MaxTier; ++Tier)
	{
		TotalWeight += 1 << (MaxTier - Tier);
	}

	int32 Roll = Rng.RandRange(0, TotalWeight - 1);
	int32 ChosenTier = 1;
	for (int32 Tier = 1; Tier <= MaxTier; ++Tier)
	{
		Roll -= 1 << (MaxTier - Tier);
		if (Roll < 0)
		{
			ChosenTier = Tier;
			break;
		}
	}

	const FROHRuneDef* Rune = FindRuneByTier(ChosenTier);
	if (!Rune)
	{
		return FROHItemInstance();
	}
	return GenerateItem(GetRuneBaseId(Rune->RuneId), ItemLevel, EROHItemQuality::Normal, Rng.RandRange(1, MAX_int32 - 1));
}

void UROHItemDatabase::RollAffixes(FROHItemInstance& Instance, const FROHItemBaseDef& Base, FRandomStream& Rng) const
{
	int32 NumPrefixes = 0;
	int32 NumSuffixes = 0;
	int32 MinTotal = 0;

	if (Instance.Quality == EROHItemQuality::Magic)
	{
		// 접두/접미 각각 50% 확률, 최소 1개 보장
		NumPrefixes = Rng.FRand() < 0.5f ? 1 : 0;
		NumSuffixes = Rng.FRand() < 0.5f ? 1 : 0;
		if (NumPrefixes + NumSuffixes == 0)
		{
			(Rng.FRand() < 0.5f ? NumPrefixes : NumSuffixes) = 1;
		}
		MinTotal = 1;
	}
	else if (Instance.Quality == EROHItemQuality::Rare)
	{
		// 3~6개, 접두/접미 각 최대 3
		const int32 Total = Rng.RandRange(3, 6);
		NumPrefixes = FMath::Clamp(Rng.RandRange(1, Total - 1), 1, 3);
		NumSuffixes = FMath::Clamp(Total - NumPrefixes, 1, 3);
		MinTotal = 3;
	}

	TArray<const FROHAffixDef*> PrefixPool = GetEligibleAffixes(Base, Instance.ItemLevel, true);
	TArray<const FROHAffixDef*> SuffixPool = GetEligibleAffixes(Base, Instance.ItemLevel, false);

	auto RollFromPool = [&](TArray<const FROHAffixDef*>& Pool, int32 Count)
	{
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

	RollFromPool(PrefixPool, NumPrefixes);
	RollFromPool(SuffixPool, NumSuffixes);

	// 저레벨 등에서 한쪽 풀이 부족해 최소 개수 미달이면 반대쪽 풀에서 보충
	while (Instance.Affixes.Num() < MinTotal && (PrefixPool.Num() + SuffixPool.Num()) > 0)
	{
		RollFromPool(PrefixPool.Num() > 0 ? PrefixPool : SuffixPool, 1);
	}
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

	FRandomStream Rng(MakeRandomSeed());

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
			if (Chosen->Type == EROHTreasureEntryType::Rune)
			{
				FROHItemInstance Rune = GenerateRuneDrop(ItemLevel, Rng);
				if (Rune.IsValid())
				{
					Result.Items.Add(Rune);
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

	// 룬워드 완성품: "[룬워드] 베이스"
	if (!Instance.RunewordId.IsNone())
	{
		if (const FROHRunewordDef* Runeword = FindRuneword(Instance.RunewordId))
		{
			return FText::Format(NSLOCTEXT("ROH", "RunewordItemName", "[{0}] {1}"), Runeword->DisplayName, Base->DisplayName);
		}
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
