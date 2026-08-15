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

	// 어트리뷰트 한글 라벨 + %단위 여부 (툴팁/정복자 창 표기 — FormatAttributeBonus 전용)
	struct FAttributeLabelEntry
	{
		FGameplayAttribute Attribute;
		const TCHAR* Label = TEXT("?");
		bool bPercent = false;
	};

	const FAttributeLabelEntry* FindAttributeLabelEntry(const FGameplayAttribute& Attribute)
	{
		static const TArray<FAttributeLabelEntry> Entries = {
			{ UROHAttributeSet::GetAttackPowerAttribute(),         TEXT("공격력"),          false },
			{ UROHAttributeSet::GetDefenseAttribute(),             TEXT("방어등급"),        false },
			{ UROHAttributeSet::GetAttackRatingAttribute(),        TEXT("명중률"),          false },
			{ UROHAttributeSet::GetMaxHealthAttribute(),           TEXT("최대 생명력"),     false },
			{ UROHAttributeSet::GetMaxManaAttribute(),             TEXT("최대 마나"),       false },
			{ UROHAttributeSet::GetHealthRegenAttribute(),         TEXT("생명력 재생"),     false },
			{ UROHAttributeSet::GetManaRegenAttribute(),           TEXT("마나 재생"),       false },
			{ UROHAttributeSet::GetStrengthAttribute(),            TEXT("힘"),              false },
			{ UROHAttributeSet::GetDexterityAttribute(),           TEXT("민첩"),            false },
			{ UROHAttributeSet::GetVitalityAttribute(),            TEXT("활력"),            false },
			{ UROHAttributeSet::GetEnergyAttribute(),              TEXT("에너지"),          false },
			{ UROHAttributeSet::GetMagicFindAttribute(),           TEXT("매직파인드"),      true },
			{ UROHAttributeSet::GetCritChanceAttribute(),          TEXT("치명타 확률"),     true },
			{ UROHAttributeSet::GetCritDamageAttribute(),          TEXT("치명타 피해"),     true },
			{ UROHAttributeSet::GetRunePowerAttribute(),           TEXT("룬위력"),          true },
			{ UROHAttributeSet::GetAttackSpeedPctAttribute(),      TEXT("공격 속도"),       true },
			{ UROHAttributeSet::GetCastSpeedPctAttribute(),        TEXT("시전 속도"),       true },
			{ UROHAttributeSet::GetPhysicalResistanceAttribute(),  TEXT("물리 피해감소"),   true },
			{ UROHAttributeSet::GetFireResistanceAttribute(),      TEXT("화염 저항"),       true },
			{ UROHAttributeSet::GetColdResistanceAttribute(),      TEXT("냉기 저항"),       true },
			{ UROHAttributeSet::GetLightningResistanceAttribute(), TEXT("번개 저항"),       true },
			{ UROHAttributeSet::GetPoisonResistanceAttribute(),    TEXT("독 저항"),         true },
			{ UROHAttributeSet::GetShadowResistanceAttribute(),    TEXT("그림자 저항"),     true },
			{ UROHAttributeSet::GetMoveSpeedAttribute(),           TEXT("이동 속도"),       false },
			// % 배율 7종 (docs/10 §1/§3.1 — b29): 동명 Flat과는 % 접미로 구분 (예: "최대 생명력 +30" vs "+8%")
			{ UROHAttributeSet::GetHealthPctAttribute(),           TEXT("최대 생명력"),     true },
			{ UROHAttributeSet::GetManaPctAttribute(),             TEXT("최대 마나"),       true },
			{ UROHAttributeSet::GetHealthRegenPctAttribute(),      TEXT("생명력 재생"),     true },
			{ UROHAttributeSet::GetManaRegenPctAttribute(),        TEXT("마나 재생"),       true },
			{ UROHAttributeSet::GetPhysicalDamagePctAttribute(),   TEXT("물리 피해"),       true },
			{ UROHAttributeSet::GetElementalDamagePctAttribute(),  TEXT("원소 피해"),       true },
			{ UROHAttributeSet::GetGlobalDamagePctAttribute(),     TEXT("모든 피해"),       true },
		};
		for (const FAttributeLabelEntry& Entry : Entries)
		{
			if (Entry.Attribute == Attribute)
			{
				return &Entry;
			}
		}
		return nullptr;
	}

	// 정수는 소수점 없이, 고대 ×1.5 등 반정수는 한 자리로
	FString FormatBonusNumber(float Value)
	{
		const float Rounded = FMath::RoundToFloat(Value);
		return FMath::IsNearlyEqual(Value, Rounded, 0.01f)
			? FString::Printf(TEXT("%.0f"), Rounded)
			: FString::Printf(TEXT("%.1f"), Value);
	}

	// 장비 슬롯 한글명 (툴팁 종류 표기 — 유니티 빌드에서 창 쪽 익명 헬퍼와 충돌하지 않게 고유명)
	const TCHAR* TooltipSlotLabel(EROHEquipSlot Slot)
	{
		switch (Slot)
		{
		case EROHEquipSlot::Weapon: return TEXT("무기");
		case EROHEquipSlot::Shield: return TEXT("방패");
		case EROHEquipSlot::Helm:   return TEXT("투구");
		case EROHEquipSlot::Chest:  return TEXT("흉갑");
		case EROHEquipSlot::Boots:  return TEXT("장화");
		default:                    return TEXT("장비");
		}
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
	Uniques.Reset();
	Sets.Reset();

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
	AddBase("SaintRelic",   TEXT("성유물 조각"), EROHItemKind::Material, EROHEquipSlot::None, 0.f, 0.f, 0.f, 1, 0.f, 100); // 유니크 분해 재료 (M5 2차)
	AddBase("FateGem",      TEXT("운명의 보석"), EROHItemKind::Material, EROHEquipSlot::None, 0.f, 0.f, 0.f, 1, 0.f, 500); // 도박 재화 (M5 3차 — 고난이도 전용 드랍)

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

	// ---------- 유니크 15종 (M5 2차 — 아자크론 세계관의 순교/타락 성인 무구, docs/06) ----------
	// 접사 대신 고정 옵션, 소켓 없음 (룬워드와 축 분리). 분해 → 성유물 조각 → 고대 합성 (×1.5, 룬위력 +3)
	auto AddUnique = [this](FName Id, const TCHAR* Name, FName BaseId, int32 ReqIlvl,
		std::initializer_list<FROHRunewordBonus> Bonuses, float RunePower)
	{
		FROHUniqueDef Def;
		Def.UniqueId = Id;
		Def.DisplayName = FText::FromString(Name);
		Def.BaseId = BaseId;
		Def.RequiredItemLevel = ReqIlvl;
		Def.Bonuses = Bonuses;
		Def.RunePower = RunePower;
		Uniques.Add(Def);
	};

	// 하급 (ilvl 1+) — 순교 초기의 성인들
	AddUnique("StRahal",  TEXT("성 라할의 대검"),     "ShortSword",   1,
		{ RB(UROHAttributeSet::GetAttackPowerAttribute(), 8.f),
		  RB(UROHAttributeSet::GetAttackRatingAttribute(), 30.f),
		  RB(UROHAttributeSet::GetCritChanceAttribute(), 3.f) }, 0.f);
	AddUnique("StMirne",  TEXT("성 미르네의 서약"),   "Buckler",      1,
		{ RB(UROHAttributeSet::GetMaxHealthAttribute(), 30.f),
		  RB(UROHAttributeSet::GetDefenseAttribute(), 15.f),
		  RB(UROHAttributeSet::GetFireResistanceAttribute(), 10.f) }, 0.f);
	AddUnique("StCassian",TEXT("성 카시안의 장막"),   "LeatherArmor", 1,
		{ RB(UROHAttributeSet::GetDefenseAttribute(), 20.f),
		  RB(UROHAttributeSet::GetShadowResistanceAttribute(), 15.f),
		  RB(UROHAttributeSet::GetMoveSpeedAttribute(), 20.f) }, 0.f);
	AddUnique("StObel",   TEXT("성 오벨의 관"),       "Cap",          1,
		{ RB(UROHAttributeSet::GetMaxManaAttribute(), 25.f),
		  RB(UROHAttributeSet::GetEnergyAttribute(), 5.f),
		  RB(UROHAttributeSet::GetCastSpeedPctAttribute(), 8.f) }, 0.f);
	AddUnique("StIven",   TEXT("성 이벤의 걸음"),     "LeatherBoots", 1,
		{ RB(UROHAttributeSet::GetMoveSpeedAttribute(), 40.f),
		  RB(UROHAttributeSet::GetDexterityAttribute(), 5.f),
		  RB(UROHAttributeSet::GetColdResistanceAttribute(), 10.f) }, 0.f);

	// 중급 (ilvl 6+) — 봉인 전쟁기의 성인들
	AddUnique("StJudith", TEXT("성 유디트의 최후"),   "BattleAxe",    6,
		{ RB(UROHAttributeSet::GetAttackPowerAttribute(), 15.f),
		  RB(UROHAttributeSet::GetCritDamageAttribute(), 40.f),
		  RB(UROHAttributeSet::GetAttackSpeedPctAttribute(), 10.f) }, 0.f);
	AddUnique("StHelos",  TEXT("성 헬로스의 보루"),   "RoundShield",  6,
		{ RB(UROHAttributeSet::GetDefenseAttribute(), 35.f),
		  RB(UROHAttributeSet::GetMaxHealthAttribute(), 40.f),
		  RB(UROHAttributeSet::GetPhysicalResistanceAttribute(), 5.f) }, 0.f); // PDR (docs/10 §4.3)
	AddUnique("StMorwen", TEXT("성 모르웬의 응시"),   "FullHelm",     6,
		{ RB(UROHAttributeSet::GetAttackRatingAttribute(), 60.f),
		  RB(UROHAttributeSet::GetCritChanceAttribute(), 5.f),
		  RB(UROHAttributeSet::GetMagicFindAttribute(), 20.f) }, 0.f);
	AddUnique("StKassel", TEXT("성 카셀의 심장"),     "ChainMail",    6,
		{ RB(UROHAttributeSet::GetMaxHealthAttribute(), 60.f),
		  RB(UROHAttributeSet::GetHealthRegenAttribute(), 3.f),
		  RB(UROHAttributeSet::GetFireResistanceAttribute(), 10.f),
		  RB(UROHAttributeSet::GetColdResistanceAttribute(), 10.f) }, 0.f);
	AddUnique("StVeyra",  TEXT("성 베이라의 속삭임"), "ShortSword",   6,
		{ RB(UROHAttributeSet::GetEnergyAttribute(), 8.f),
		  RB(UROHAttributeSet::GetCastSpeedPctAttribute(), 12.f),
		  RB(UROHAttributeSet::GetLightningResistanceAttribute(), 10.f) }, 0.f);

	// 상급 (ilvl 12+) — 타락 직전의 마지막 성인들 (룬 위력 부여)
	// b29: 상위 티어 일부에 % 옵션 소량 부여 (docs/10 §1 체감용 — 옵션 배열 append만, ID/구조 불변)
	AddUnique("StRakhom", TEXT("성 라콤의 분노"),     "BattleAxe",    12,
		{ RB(UROHAttributeSet::GetAttackPowerAttribute(), 25.f),
		  RB(UROHAttributeSet::GetCritChanceAttribute(), 6.f),
		  RB(UROHAttributeSet::GetCritDamageAttribute(), 50.f),
		  RB(UROHAttributeSet::GetPhysicalDamagePctAttribute(), 10.f) }, 8.f); // b29 % 옵션
	AddUnique("StElara",  TEXT("성 엘라라의 성벽"),   "RoundShield",  12,
		{ RB(UROHAttributeSet::GetDefenseAttribute(), 50.f),
		  RB(UROHAttributeSet::GetMaxHealthAttribute(), 50.f),
		  RB(UROHAttributeSet::GetShadowResistanceAttribute(), 15.f) }, 3.f);
	AddUnique("StNoctis", TEXT("성 녹티스의 면갑"),   "FullHelm",     12,
		{ RB(UROHAttributeSet::GetMaxManaAttribute(), 40.f),
		  RB(UROHAttributeSet::GetCritChanceAttribute(), 4.f),
		  RB(UROHAttributeSet::GetMagicFindAttribute(), 25.f),
		  RB(UROHAttributeSet::GetElementalDamagePctAttribute(), 8.f) }, 4.f); // b29 % 옵션
	AddUnique("StSerin",  TEXT("성 세린의 갑주"),     "ChainMail",    12,
		{ RB(UROHAttributeSet::GetMaxHealthAttribute(), 80.f),
		  RB(UROHAttributeSet::GetDefenseAttribute(), 40.f),
		  RB(UROHAttributeSet::GetHealthRegenAttribute(), 4.f),
		  RB(UROHAttributeSet::GetHealthPctAttribute(), 8.f) }, 5.f); // b29 % 옵션
	AddUnique("StAvelo",  TEXT("성 아벨로의 낙인"),   "ShortSword",   12,
		{ RB(UROHAttributeSet::GetAttackPowerAttribute(), 20.f),
		  RB(UROHAttributeSet::GetAttackSpeedPctAttribute(), 15.f),
		  RB(UROHAttributeSet::GetAttackRatingAttribute(), 80.f) }, 6.f);

	// ---------- 세트 4종 (M5 2차 — 고대 유적지 테마, 장착 조합 보너스가 본체) ----------
	auto MakePiece = [](FName PieceId, const TCHAR* Name, FName BaseId, std::initializer_list<FROHRunewordBonus> Bonuses)
	{
		FROHSetPieceDef Piece;
		Piece.PieceId = PieceId;
		Piece.DisplayName = FText::FromString(Name);
		Piece.BaseId = BaseId;
		Piece.Bonuses = Bonuses;
		return Piece;
	};

	{
		// ① 저레벨 3피스: 근접 입문 세트
		FROHSetDef Spire;
		Spire.SetId = "AshenSpire";
		Spire.DisplayName = FText::FromString(TEXT("잿빛 첨탑"));
		Spire.RequiredItemLevel = 1;
		Spire.Pieces.Add(MakePiece("SpireBlade", TEXT("잿빛 첨탑의 파수검"), "ShortSword",
			{ RB(UROHAttributeSet::GetAttackPowerAttribute(), 4.f), RB(UROHAttributeSet::GetAttackRatingAttribute(), 15.f) }));
		Spire.Pieces.Add(MakePiece("SpireCrown", TEXT("잿빛 첨탑의 관모"), "Cap",
			{ RB(UROHAttributeSet::GetMaxManaAttribute(), 10.f) }));
		Spire.Pieces.Add(MakePiece("SpireVest", TEXT("잿빛 첨탑의 예복"), "LeatherArmor",
			{ RB(UROHAttributeSet::GetDefenseAttribute(), 10.f) }));
		Spire.CountBonuses.Add(2, { RB(UROHAttributeSet::GetMaxHealthAttribute(), 20.f) });
		Spire.CountBonuses.Add(3, { RB(UROHAttributeSet::GetFireResistanceAttribute(), 10.f),
			RB(UROHAttributeSet::GetColdResistanceAttribute(), 10.f),
			RB(UROHAttributeSet::GetLightningResistanceAttribute(), 10.f),
			RB(UROHAttributeSet::GetPoisonResistanceAttribute(), 10.f),
			RB(UROHAttributeSet::GetShadowResistanceAttribute(), 10.f),
			RB(UROHAttributeSet::GetAttackPowerAttribute(), 6.f) });
		Spire.FullSetRunePower = 0.f;
		Sets.Add(Spire);
	}
	{
		// ② 저레벨 2피스: 생존 소세트
		FROHSetDef Grave;
		Grave.SetId = "SunkenGrave";
		Grave.DisplayName = FText::FromString(TEXT("가라앉은 묘역"));
		Grave.RequiredItemLevel = 1;
		Grave.Pieces.Add(MakePiece("GraveWall", TEXT("가라앉은 묘역의 패방패"), "Buckler",
			{ RB(UROHAttributeSet::GetDefenseAttribute(), 8.f), RB(UROHAttributeSet::GetMaxHealthAttribute(), 10.f) }));
		Grave.Pieces.Add(MakePiece("GraveTread", TEXT("가라앉은 묘역의 장화"), "LeatherBoots",
			{ RB(UROHAttributeSet::GetMoveSpeedAttribute(), 15.f) }));
		Grave.CountBonuses.Add(2, { RB(UROHAttributeSet::GetMaxHealthAttribute(), 25.f),
			RB(UROHAttributeSet::GetColdResistanceAttribute(), 15.f) });
		Grave.FullSetRunePower = 0.f;
		Sets.Add(Grave);
	}
	{
		// ③ 고레벨 5피스: 풀슬롯 최종 지향 세트
		FROHSetDef Remains;
		Remains.SetId = "AzakronRemains";
		Remains.DisplayName = FText::FromString(TEXT("아자크론의 잔해"));
		Remains.RequiredItemLevel = 10;
		Remains.Pieces.Add(MakePiece("RemainsAxe", TEXT("아자크론 잔해의 파쇄도끼"), "BattleAxe",
			{ RB(UROHAttributeSet::GetAttackPowerAttribute(), 10.f), RB(UROHAttributeSet::GetCritChanceAttribute(), 2.f) }));
		Remains.Pieces.Add(MakePiece("RemainsShield", TEXT("아자크론 잔해의 수호방패"), "RoundShield",
			{ RB(UROHAttributeSet::GetDefenseAttribute(), 20.f) }));
		Remains.Pieces.Add(MakePiece("RemainsHelm", TEXT("아자크론 잔해의 철관"), "FullHelm",
			{ RB(UROHAttributeSet::GetAttackRatingAttribute(), 30.f) }));
		Remains.Pieces.Add(MakePiece("RemainsMail", TEXT("아자크론 잔해의 갑주"), "ChainMail",
			{ RB(UROHAttributeSet::GetMaxHealthAttribute(), 30.f) }));
		Remains.Pieces.Add(MakePiece("RemainsGreaves", TEXT("아자크론 잔해의 군화"), "LeatherBoots",
			{ RB(UROHAttributeSet::GetMoveSpeedAttribute(), 20.f) }));
		Remains.CountBonuses.Add(2, { RB(UROHAttributeSet::GetMaxHealthAttribute(), 30.f) });
		Remains.CountBonuses.Add(3, { RB(UROHAttributeSet::GetFireResistanceAttribute(), 10.f),
			RB(UROHAttributeSet::GetColdResistanceAttribute(), 10.f),
			RB(UROHAttributeSet::GetLightningResistanceAttribute(), 10.f),
			RB(UROHAttributeSet::GetPoisonResistanceAttribute(), 10.f),
			RB(UROHAttributeSet::GetShadowResistanceAttribute(), 10.f) });
		Remains.CountBonuses.Add(4, { RB(UROHAttributeSet::GetAttackPowerAttribute(), 15.f),
			RB(UROHAttributeSet::GetDefenseAttribute(), 40.f) });
		Remains.CountBonuses.Add(5, { RB(UROHAttributeSet::GetCritChanceAttribute(), 5.f),
			RB(UROHAttributeSet::GetCritDamageAttribute(), 30.f),
			RB(UROHAttributeSet::GetAttackSpeedPctAttribute(), 10.f),
			RB(UROHAttributeSet::GetGlobalDamagePctAttribute(), 5.f) }); // b29 % 옵션 (풀세트 전용)
		Remains.FullSetRunePower = 10.f;
		Sets.Add(Remains);
	}
	{
		// ④ 고레벨 3피스: 캐스터 지향
		FROHSetDef Altar;
		Altar.SetId = "SilentAltar";
		Altar.DisplayName = FText::FromString(TEXT("침묵의 제단"));
		Altar.RequiredItemLevel = 10;
		Altar.Pieces.Add(MakePiece("AltarBlade", TEXT("침묵 제단의 의식검"), "ShortSword",
			{ RB(UROHAttributeSet::GetEnergyAttribute(), 6.f), RB(UROHAttributeSet::GetCastSpeedPctAttribute(), 8.f) }));
		Altar.Pieces.Add(MakePiece("AltarCrown", TEXT("침묵 제단의 성문관"), "Cap",
			{ RB(UROHAttributeSet::GetMaxManaAttribute(), 20.f), RB(UROHAttributeSet::GetManaRegenAttribute(), 1.f) }));
		Altar.Pieces.Add(MakePiece("AltarRobe", TEXT("침묵 제단의 제의복"), "LeatherArmor",
			{ RB(UROHAttributeSet::GetDefenseAttribute(), 15.f), RB(UROHAttributeSet::GetEnergyAttribute(), 4.f) }));
		Altar.CountBonuses.Add(2, { RB(UROHAttributeSet::GetMaxManaAttribute(), 30.f),
			RB(UROHAttributeSet::GetCastSpeedPctAttribute(), 10.f) });
		Altar.CountBonuses.Add(3, { RB(UROHAttributeSet::GetEnergyAttribute(), 10.f),
			RB(UROHAttributeSet::GetManaRegenAttribute(), 2.f),
			RB(UROHAttributeSet::GetElementalDamagePctAttribute(), 8.f), // b29 % 옵션 (풀세트 전용)
			RB(UROHAttributeSet::GetFireResistanceAttribute(), 8.f),
			RB(UROHAttributeSet::GetColdResistanceAttribute(), 8.f),
			RB(UROHAttributeSet::GetLightningResistanceAttribute(), 8.f),
			RB(UROHAttributeSet::GetPoisonResistanceAttribute(), 8.f),
			RB(UROHAttributeSet::GetShadowResistanceAttribute(), 8.f) });
		Altar.FullSetRunePower = 4.f;
		Sets.Add(Altar);
	}

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

	// % 배율 접사 (docs/10 §1/§3.1 — b29): 합산식 % 어트리뷰트 공급원.
	// 전체 피해 %는 고 ilvl 전용 희귀 접사 (룬각인 OfRunes보다 상위 게이트)
	AddAffix("Savage",      TEXT("흉포한"),   true,  UROHAttributeSet::GetPhysicalDamagePctAttribute(), 5.f, 15.f, 8, { EROHEquipSlot::Weapon });
	AddAffix("Arcane",      TEXT("비전의"),   true,  UROHAttributeSet::GetElementalDamagePctAttribute(), 5.f, 15.f, 8, { EROHEquipSlot::Weapon });
	AddAffix("OfVigor",     TEXT("생기의"),   false, UROHAttributeSet::GetHealthPctAttribute(), 3.f, 10.f, 4, {});
	AddAffix("OfSpirit",    TEXT("정기의"),   false, UROHAttributeSet::GetManaPctAttribute(), 3.f, 10.f, 4, {});
	AddAffix("OfAnnihilation", TEXT("절멸의"), false, UROHAttributeSet::GetGlobalDamagePctAttribute(), 3.f, 8.f, 12, {});

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

	UE_LOG(LogROH, Log, TEXT("ItemDatabase: 베이스 %d, 접사 %d, TC %d, 룬 %d, 룬워드 %d, 유니크 %d, 세트 %d 등록"),
		Bases.Num(), Affixes.Num(), TreasureClasses.Num(), Runes.Num(), Runewords.Num(), Uniques.Num(), Sets.Num());
}

const FROHSetDef* UROHItemDatabase::FindSet(FName SetId) const
{
	for (const FROHSetDef& Def : Sets)
	{
		if (Def.SetId == SetId)
		{
			return &Def;
		}
	}
	return nullptr;
}

const FROHSetPieceDef* UROHItemDatabase::FindSetPiece(FName PieceId, const FROHSetDef** OutSet) const
{
	for (const FROHSetDef& Def : Sets)
	{
		for (const FROHSetPieceDef& Piece : Def.Pieces)
		{
			if (Piece.PieceId == PieceId)
			{
				if (OutSet)
				{
					*OutSet = &Def;
				}
				return &Piece;
			}
		}
	}
	if (OutSet)
	{
		*OutSet = nullptr;
	}
	return nullptr;
}

const FROHUniqueDef* UROHItemDatabase::FindUnique(FName UniqueId) const
{
	for (const FROHUniqueDef& Def : Uniques)
	{
		if (Def.UniqueId == UniqueId)
		{
			return &Def;
		}
	}
	return nullptr;
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

	// M5 2차 밴드 순서: 유니크 → 세트 → 레어 → 매직 (FRand 1회)
	const float UniqueChance = 0.01f * (1.f + EffectiveMF / 100.f);
	const float SetChance = 0.015f * (1.f + EffectiveMF / 100.f);
	const float RareChance = 0.05f * (1.f + EffectiveMF / 100.f);
	const float MagicChance = 0.20f * (1.f + EffectiveMF / 100.f);

	const float Roll = Rng.FRand();
	if (Roll < UniqueChance)
	{
		return EROHItemQuality::Unique;
	}
	if (Roll < UniqueChance + SetChance)
	{
		return EROHItemQuality::Set;
	}
	if (Roll < UniqueChance + SetChance + RareChance)
	{
		return EROHItemQuality::Rare;
	}
	if (Roll < UniqueChance + SetChance + RareChance + MagicChance)
	{
		return EROHItemQuality::Magic;
	}
	return EROHItemQuality::Normal;
}

FROHItemInstance UROHItemDatabase::GenerateItem(FName BaseId, int32 ItemLevel, EROHItemQuality Quality, int32 Seed, bool bAsUnidentifiedDrop) const
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

		// 미감정 (docs/12): 드랍 경로의 마법+ 장비만 — 등급 강등 후에도 최종 등급 기준으로 판정
		auto MarkUnidentified = [&Instance, bAsUnidentifiedDrop]()
		{
			if (bAsUnidentifiedDrop && Instance.Quality != EROHItemQuality::Normal)
			{
				Instance.bUnidentified = true;
			}
		};

		// 유니크 (M5 2차): 베이스+ilvl 충족 후보에서 1개 선택 — 접사/소켓 없음 (고정 옵션이 대체)
		if (Instance.Quality == EROHItemQuality::Unique)
		{
			TArray<const FROHUniqueDef*> Candidates;
			for (const FROHUniqueDef& Def : Uniques)
			{
				if (Def.BaseId == Instance.BaseId && Instance.ItemLevel >= Def.RequiredItemLevel)
				{
					Candidates.Add(&Def);
				}
			}
			if (Candidates.Num() > 0)
			{
				Instance.UniqueId = Candidates[Rng.RandRange(0, Candidates.Num() - 1)]->UniqueId;
				MarkUnidentified();
				return Instance;
			}
			// 후보 없음 → 레어 강등 (강등 판정은 스트림 미소비 — 기존 등급 경로의 소비 순서 불변)
			Instance.Quality = EROHItemQuality::Rare;
		}

		// 세트 (M5 2차): 유니크와 동일 규칙 — 베이스+ilvl 적합 피스 선택, 접사/소켓 없음
		if (Instance.Quality == EROHItemQuality::Set)
		{
			TArray<const FROHSetPieceDef*> PieceCandidates;
			for (const FROHSetDef& SetDef : Sets)
			{
				if (Instance.ItemLevel < SetDef.RequiredItemLevel)
				{
					continue;
				}
				for (const FROHSetPieceDef& Piece : SetDef.Pieces)
				{
					if (Piece.BaseId == Instance.BaseId)
					{
						PieceCandidates.Add(&Piece);
					}
				}
			}
			if (PieceCandidates.Num() > 0)
			{
				Instance.SetPieceId = PieceCandidates[Rng.RandRange(0, PieceCandidates.Num() - 1)]->PieceId;
				MarkUnidentified();
				return Instance;
			}
			Instance.Quality = EROHItemQuality::Rare;
		}

		// 접사 먼저 소비 → 소켓: 기존 시드의 접사 재현성 유지 (테스트 ROH.Loot 3번 항목)
		if (Instance.Quality != EROHItemQuality::Normal)
		{
			RollAffixes(Instance, *Base, Rng);
		}
		RollSockets(Instance, *Base, Rng);
		MarkUnidentified();
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
				// 드랍 경로: 마법+ 장비는 미감정 상태로 (docs/12 — 룬/물약 등은 등급 Normal이라 무영향)
				FROHItemInstance Item = GenerateItem(Chosen->Ref, ItemLevel, Quality, Rng.RandRange(1, MAX_int32 - 1), /*bAsUnidentifiedDrop=*/true);
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

	// 미감정 (docs/12): 정체 숨김 — 베이스명만, 등급색은 유지 (색으로 기대감)
	if (Instance.bUnidentified)
	{
		return FText::Format(NSLOCTEXT("ROH", "UnidentifiedItemName", "미감정 {0}"), Base->DisplayName);
	}

	// 유니크/고대 (M5 2차): 성인 무구 고유명, 고대는 "[고대]" 접두
	if (!Instance.UniqueId.IsNone())
	{
		if (const FROHUniqueDef* Unique = FindUnique(Instance.UniqueId))
		{
			return Instance.Quality == EROHItemQuality::Ancient
				? FText::Format(NSLOCTEXT("ROH", "AncientItemName", "[고대] {0}"), Unique->DisplayName)
				: Unique->DisplayName;
		}
	}

	// 세트 피스 (M5 2차): 유적지 고유명 ("잿빛 첨탑의 파수검")
	if (!Instance.SetPieceId.IsNone())
	{
		if (const FROHSetPieceDef* Piece = FindSetPiece(Instance.SetPieceId))
		{
			return Piece->DisplayName;
		}
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

void UROHItemDatabase::RefreshItemAffixes(FROHItemInstance& Item) const
{
	for (FROHAffixRoll& Affix : Item.Affixes)
	{
		if (const FROHAffixDef* Def = FindAffix(Affix.AffixId))
		{
			Affix.Attribute = Def->Attribute;
		}
	}
}

FROHResolvedEquipBonuses UROHItemDatabase::ResolveEquipBonuses(const FROHItemInstance& Item) const
{
	// 장착 GE(ApplyEquipEffect)와 툴팁의 단일 소스 — 출처 추가 시 반드시 여기만 수정
	FROHResolvedEquipBonuses Resolved;

	const FROHItemBaseDef* Base = FindBase(Item.BaseId);
	if (!Base)
	{
		return Resolved;
	}

	auto AddBonus = [](TArray<FROHResolvedBonus>& Bonuses, const FGameplayAttribute& Attribute, float Value)
	{
		FROHResolvedBonus Bonus;
		Bonus.Attribute = Attribute;
		Bonus.Value = Value;
		Bonuses.Add(Bonus);
	};

	// 베이스 성능
	if (Base->DamageMax > 0.f)
	{
		AddBonus(Resolved.BaseBonuses, UROHAttributeSet::GetAttackPowerAttribute(), (Base->DamageMin + Base->DamageMax) * 0.5f);
	}
	if (Base->Armor > 0.f)
	{
		AddBonus(Resolved.BaseBonuses, UROHAttributeSet::GetDefenseAttribute(), Base->Armor);
	}

	// 접사 (라이브 해석 — Item.Affixes와 인덱스 정렬 유지, 무효 어트리뷰트 필터는 GE 적용측 담당)
	for (const FROHAffixRoll& Affix : Item.Affixes)
	{
		AddBonus(Resolved.AffixBonuses, Affix.Attribute, Affix.Value);
	}

	// 소켓 룬 (세이브엔 ID만 — 보너스는 항상 DB에서 해석)
	for (const FName& RuneId : Item.SocketedRunes)
	{
		if (const FROHRuneDef* Rune = FindRune(RuneId))
		{
			AddBonus(Resolved.RuneBonuses, Rune->BonusAttribute, Rune->BonusValue);
			Resolved.TotalRunePower += Rune->RunePower;
		}
	}

	// 완성 룬워드
	if (!Item.RunewordId.IsNone())
	{
		if (const FROHRunewordDef* Runeword = FindRuneword(Item.RunewordId))
		{
			for (const FROHRunewordBonus& Bonus : Runeword->Bonuses)
			{
				AddBonus(Resolved.RunewordBonuses, Bonus.Attribute, Bonus.Value);
			}
			Resolved.TotalRunePower += Runeword->RunePower;
		}
	}

	// 세트 피스 자체 옵션 (조합 보너스는 RefreshSetBonuses의 별도 GE가 담당)
	if (!Item.SetPieceId.IsNone())
	{
		if (const FROHSetPieceDef* Piece = FindSetPiece(Item.SetPieceId))
		{
			for (const FROHRunewordBonus& Bonus : Piece->Bonuses)
			{
				AddBonus(Resolved.SetPieceBonuses, Bonus.Attribute, Bonus.Value);
			}
		}
	}

	// 유니크/고대 고정 옵션 — 고대는 옵션 ×1.5 + 룬 위력 +3
	if (!Item.UniqueId.IsNone())
	{
		if (const FROHUniqueDef* Unique = FindUnique(Item.UniqueId))
		{
			const bool bAncient = (Item.Quality == EROHItemQuality::Ancient);
			const float BonusMult = bAncient ? AncientBonusMult : 1.f;
			for (const FROHRunewordBonus& Bonus : Unique->Bonuses)
			{
				AddBonus(Resolved.UniqueBonuses, Bonus.Attribute, Bonus.Value * BonusMult);
			}
			Resolved.TotalRunePower += Unique->RunePower + (bAncient ? AncientRunePowerBonus : 0.f);
		}
	}
	return Resolved;
}

FString UROHItemDatabase::FormatAttributeBonus(const FGameplayAttribute& Attribute, float Value)
{
	const FAttributeLabelEntry* Entry = FindAttributeLabelEntry(Attribute);
	const FString Label = Entry ? FString(Entry->Label) : (Attribute.IsValid() ? Attribute.GetName() : FString(TEXT("?")));
	return FString::Printf(TEXT("%s %s%s%s"), *Label,
		Value < 0.f ? TEXT("-") : TEXT("+"), *FormatBonusNumber(FMath::Abs(Value)),
		(Entry && Entry->bPercent) ? TEXT("%") : TEXT(""));
}

const TCHAR* UROHItemDatabase::GetQualityLabel(EROHItemQuality Quality)
{
	switch (Quality)
	{
	case EROHItemQuality::Magic:    return TEXT("마법");
	case EROHItemQuality::Rare:     return TEXT("레어");
	case EROHItemQuality::Set:      return TEXT("세트");
	case EROHItemQuality::Unique:   return TEXT("유니크");
	case EROHItemQuality::Runeword: return TEXT("룬워드");
	case EROHItemQuality::Ancient:  return TEXT("고대");
	default:                        return TEXT("일반");
	}
}

FText UROHItemDatabase::GetItemTooltip(const FROHItemInstance& Item) const
{
	const FROHItemBaseDef* Base = FindBase(Item.BaseId);
	if (!Base)
	{
		return FText::FromString(TEXT("알 수 없는 아이템"));
	}

	TArray<FString> Lines;
	Lines.Add(GetItemDisplayName(Item).ToString());

	// 등급/종류/아이템 레벨
	if (Base->Kind == EROHItemKind::Equipment)
	{
		Lines.Add(FString::Printf(TEXT("%s %s | 아이템 레벨 %d"),
			GetQualityLabel(Item.Quality), TooltipSlotLabel(Base->Slot), Item.ItemLevel));
	}
	else
	{
		switch (Base->Kind)
		{
		case EROHItemKind::Potion:   Lines.Add(TEXT("종류: 물약")); break;
		case EROHItemKind::Rune:     Lines.Add(TEXT("종류: 룬 (소켓 재료)")); break;
		case EROHItemKind::Material: Lines.Add(TEXT("종류: 재료")); break;
		default: break;
		}
	}

	// 기본 성능 (베이스 정의에 있는 것만)
	if (Base->DamageMax > 0.f)
	{
		Lines.Add(FString::Printf(TEXT("무기 피해 %.0f~%.0f (평균 %s)"),
			Base->DamageMin, Base->DamageMax, *FormatBonusNumber((Base->DamageMin + Base->DamageMax) * 0.5f)));
	}
	if (Base->Armor > 0.f)
	{
		Lines.Add(FString::Printf(TEXT("방어등급 %.0f"), Base->Armor));
	}
	if (Base->PotionHealAmount > 0.f)
	{
		Lines.Add(FString::Printf(TEXT("사용 시 생명력 %.0f 회복"), Base->PotionHealAmount));
	}
	if (Base->RequiredLevel > 1)
	{
		Lines.Add(FString::Printf(TEXT("필요 레벨 %d"), Base->RequiredLevel));
	}

	// 룬 아이템: 소켓 시 보너스 (베이스 명명 규약 역조회)
	if (const FROHRuneDef* RuneDef = FindRuneByBaseId(Item.BaseId))
	{
		Lines.Add(FString::Printf(TEXT("티어 %d — 소켓 시: %s, 룬위력 +%.1f%%"),
			RuneDef->Tier, *FormatAttributeBonus(RuneDef->BonusAttribute, RuneDef->BonusValue), RuneDef->RunePower));
		Lines.Add(TEXT("같은 룬 3개 → 상위 티어 1개 (ROHTransmute)"));
	}

	const FROHResolvedEquipBonuses Resolved = ResolveEquipBonuses(Item);

	if (Item.bUnidentified)
	{
		// 미감정 (docs/12): 접사/고정 옵션 전부 숨김 — 등급색만으로 기대감
		Lines.Add(TEXT("미감정 — 감정 필요 (셀바 50골드 / ROHIdentify)"));
	}
	else
	{
		// 접사 (라이브 해석 — Resolved.AffixBonuses는 Item.Affixes와 인덱스 정렬)
		if (Item.Affixes.Num() > 0)
		{
			Lines.Add(TEXT("--- 접사 ---"));
			for (int32 AffixIndex = 0; AffixIndex < Item.Affixes.Num(); ++AffixIndex)
			{
				const FROHAffixDef* AffixDef = FindAffix(Item.Affixes[AffixIndex].AffixId);
				const FROHResolvedBonus& Bonus = Resolved.AffixBonuses[AffixIndex];
				Lines.Add(FString::Printf(TEXT("%s (%s)"),
					*FormatAttributeBonus(Bonus.Attribute, Bonus.Value),
					AffixDef ? *AffixDef->DisplayName.ToString() : *Item.Affixes[AffixIndex].AffixId.ToString()));
			}
		}
	}

	// 소켓/삽입 룬 (미감정도 소켓 수는 표시 — 삽입은 감정 후 가능)
	if (Base->Kind == EROHItemKind::Equipment && Item.MaxSockets > 0)
	{
		Lines.Add(FString::Printf(TEXT("--- 소켓 %d/%d ---"), Item.SocketedRunes.Num(), Item.MaxSockets));
		for (int32 SocketIndex = 0; SocketIndex < Item.MaxSockets; ++SocketIndex)
		{
			if (!Item.SocketedRunes.IsValidIndex(SocketIndex))
			{
				Lines.Add(TEXT("(빈 소켓)"));
			}
			else if (const FROHRuneDef* SocketRune = FindRune(Item.SocketedRunes[SocketIndex]))
			{
				Lines.Add(FString::Printf(TEXT("%s 룬: %s, 룬위력 +%.1f%%"),
					*SocketRune->DisplayName.ToString(),
					*FormatAttributeBonus(SocketRune->BonusAttribute, SocketRune->BonusValue), SocketRune->RunePower));
			}
			else
			{
				Lines.Add(FString::Printf(TEXT("%s (미등록 룬)"), *Item.SocketedRunes[SocketIndex].ToString()));
			}
		}
	}

	if (!Item.bUnidentified)
	{
		// 완성 룬워드 보너스
		if (!Item.RunewordId.IsNone())
		{
			if (const FROHRunewordDef* Runeword = FindRuneword(Item.RunewordId))
			{
				Lines.Add(FString::Printf(TEXT("--- 룬워드: %s ---"), *Runeword->DisplayName.ToString()));
				for (const FROHResolvedBonus& Bonus : Resolved.RunewordBonuses)
				{
					Lines.Add(FormatAttributeBonus(Bonus.Attribute, Bonus.Value));
				}
				if (Runeword->RunePower > 0.f)
				{
					Lines.Add(FString::Printf(TEXT("룬위력 +%.1f%%"), Runeword->RunePower));
				}
			}
		}

		// 유니크/고대 고정 옵션 (고대는 ×1.5 반영치 표기)
		if (!Item.UniqueId.IsNone() && Resolved.UniqueBonuses.Num() > 0)
		{
			Lines.Add(Item.Quality == EROHItemQuality::Ancient
				? TEXT("--- 고대 옵션 (유니크 ×1.5) ---") : TEXT("--- 유니크 옵션 ---"));
			for (const FROHResolvedBonus& Bonus : Resolved.UniqueBonuses)
			{
				Lines.Add(FormatAttributeBonus(Bonus.Attribute, Bonus.Value));
			}
		}

		// 세트 자체 옵션 + 소속
		if (!Item.SetPieceId.IsNone())
		{
			if (Resolved.SetPieceBonuses.Num() > 0)
			{
				Lines.Add(TEXT("--- 세트 옵션 ---"));
				for (const FROHResolvedBonus& Bonus : Resolved.SetPieceBonuses)
				{
					Lines.Add(FormatAttributeBonus(Bonus.Attribute, Bonus.Value));
				}
			}
			const FROHSetDef* OwningSet = nullptr;
			if (FindSetPiece(Item.SetPieceId, &OwningSet) && OwningSet)
			{
				Lines.Add(FString::Printf(TEXT("세트: %s (%d피스 — 장착 수만큼 보너스)"),
					*OwningSet->DisplayName.ToString(), OwningSet->Pieces.Num()));
			}
		}

		// 총 룬위력 (룬+룬워드+유니크/고대 — docs/10 §5.2 최종 피해 배율 합산원)
		if (Resolved.TotalRunePower > 0.f)
		{
			Lines.Add(FString::Printf(TEXT("룬위력 합계 +%.1f%% (최종 피해 배율)"), Resolved.TotalRunePower));
		}
	}

	Lines.Add(FString::Printf(TEXT("가치: %d골드"), Base->GoldValue));
	return FText::FromString(FString::Join(Lines, TEXT("\n")));
}

FColor UROHItemDatabase::GetQualityColor(EROHItemQuality Quality)
{
	// 소유자 확정 팔레트: 흰 일반 / 파랑 매직 / 노랑 레어 / 초록 세트 / 금 유니크 / 자주 룬워드 / 진홍 고대
	switch (Quality)
	{
	case EROHItemQuality::Magic:    return FColor(100, 150, 255);
	case EROHItemQuality::Rare:     return FColor(255, 255, 0);
	case EROHItemQuality::Set:      return FColor(80, 220, 80);
	case EROHItemQuality::Unique:   return FColor(255, 200, 60);
	case EROHItemQuality::Runeword: return FColor(160, 60, 220);
	case EROHItemQuality::Ancient:  return FColor(220, 60, 60);
	default:                        return FColor::White;
	}
}
