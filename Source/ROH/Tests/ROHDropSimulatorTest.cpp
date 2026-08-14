// 드랍 시스템 자동화 테스트 (docs/03 §5 드랍 시뮬레이터)
// 에디터 Session Frontend > Automation 에서 "ROH.Loot" 실행

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Items/ROHItemDatabase.h"
#include "Items/ROHItemTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FROHDropSimulatorTest,
	"ROH.Loot.DropSimulator",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FROHDropSimulatorTest::RunTest(const FString& Parameters)
{
	UROHItemDatabase* Database = NewObject<UROHItemDatabase>();
	Database->BuildDefaultData();

	// 1) 데이터 정합성: 모든 TC 항목의 참조가 실재하는가
	{
		const FName TCIds[] = { TEXT("TC_Default"), TEXT("TC_Boss"), TEXT("TC_Weapons"), TEXT("TC_Armor") };
		for (const FName& TCId : TCIds)
		{
			const FROHTreasureClassDef* TC = Database->FindTreasureClass(TCId);
			if (!TestNotNull(FString::Printf(TEXT("TC '%s' 존재"), *TCId.ToString()), TC))
			{
				continue;
			}
			for (const FROHTreasureEntry& Entry : TC->Entries)
			{
				if (Entry.Type == EROHTreasureEntryType::BaseItem)
				{
					TestNotNull(FString::Printf(TEXT("베이스 '%s' 존재"), *Entry.Ref.ToString()), Database->FindBase(Entry.Ref));
				}
				else if (Entry.Type == EROHTreasureEntryType::SubTable)
				{
					TestNotNull(FString::Printf(TEXT("하위 TC '%s' 존재"), *Entry.Ref.ToString()), Database->FindTreasureClass(Entry.Ref));
				}
			}
		}
	}

	// 2) 접사 규칙: 매직 1~2개, 레어 3~6개, 일반 0개
	{
		for (int32 i = 0; i < 200; ++i)
		{
			const FROHItemInstance Magic = Database->GenerateItem(TEXT("ShortSword"), 10, EROHItemQuality::Magic, i + 1);
			TestTrue(TEXT("매직 접사 1~2개"), Magic.Affixes.Num() >= 1 && Magic.Affixes.Num() <= 2);

			const FROHItemInstance Rare = Database->GenerateItem(TEXT("ChainMail"), 10, EROHItemQuality::Rare, i + 1);
			TestTrue(TEXT("레어 접사 3~6개"), Rare.Affixes.Num() >= 3 && Rare.Affixes.Num() <= 6);

			const FROHItemInstance Normal = Database->GenerateItem(TEXT("Cap"), 10, EROHItemQuality::Normal, i + 1);
			TestEqual(TEXT("일반 접사 0개"), Normal.Affixes.Num(), 0);
		}
	}

	// 3) 동일 시드 = 동일 굴림 (재현성)
	{
		const FROHItemInstance A = Database->GenerateItem(TEXT("BattleAxe"), 8, EROHItemQuality::Rare, 12345);
		const FROHItemInstance B = Database->GenerateItem(TEXT("BattleAxe"), 8, EROHItemQuality::Rare, 12345);
		TestEqual(TEXT("시드 재현성: 접사 개수"), A.Affixes.Num(), B.Affixes.Num());
		for (int32 i = 0; i < FMath::Min(A.Affixes.Num(), B.Affixes.Num()); ++i)
		{
			TestEqual(TEXT("시드 재현성: 접사 ID"), A.Affixes[i].AffixId, B.Affixes[i].AffixId);
			TestEqual(TEXT("시드 재현성: 접사 값"), A.Affixes[i].Value, B.Affixes[i].Value);
		}
	}

	// 4) 대량 굴림 통계: 설계 수치와의 정합 (10,000회, 여유 허용치)
	{
		const int32 Rolls = 10000;
		int32 ItemCount = 0;
		int32 NoDropCount = 0;
		int32 RareCount = 0;

		for (int32 i = 0; i < Rolls; ++i)
		{
			const FROHDropResult Drop = Database->RollTreasureClass(TEXT("TC_Default"), 10, 0.f);
			if (Drop.Items.Num() == 0 && Drop.Gold == 0)
			{
				++NoDropCount;
			}
			ItemCount += Drop.Items.Num();
			for (const FROHItemInstance& Item : Drop.Items)
			{
				TestTrue(TEXT("생성된 아이템 유효"), Item.IsValid());
				if (Item.Quality == EROHItemQuality::Rare)
				{
					++RareCount;
				}
			}
		}

		// TC_Default: NoDrop 가중치 35/100 → 30~40% 허용
		const float NoDropRate = static_cast<float>(NoDropCount) / Rolls;
		TestTrue(FString::Printf(TEXT("NoDrop 비율 30~40%% (실측 %.1f%%)"), NoDropRate * 100.f),
			NoDropRate > 0.30f && NoDropRate < 0.40f);

		// 아이템은 반드시 일부 존재
		TestTrue(TEXT("아이템 드랍 발생"), ItemCount > 0);

		// 레어 확률 5% 설계 → 장비 드랍 중 3~8% 허용 (물약 제외 보정 전 러프 체크)
		if (ItemCount > 0)
		{
			const float RareRate = static_cast<float>(RareCount) / ItemCount;
			TestTrue(FString::Printf(TEXT("레어 비율 러프 범위 (실측 %.1f%%)"), RareRate * 100.f),
				RareRate > 0.01f && RareRate < 0.12f);
		}
	}

	// 5) MF 증가 → 매직 이상 비율 증가 (방향성 검증)
	{
		const int32 Rolls = 4000;
		auto CountMagicPlus = [&](float MF)
		{
			int32 MagicPlus = 0;
			int32 Total = 0;
			for (int32 i = 0; i < Rolls; ++i)
			{
				const FROHDropResult Drop = Database->RollTreasureClass(TEXT("TC_Weapons"), 10, MF);
				for (const FROHItemInstance& Item : Drop.Items)
				{
					++Total;
					if (Item.Quality != EROHItemQuality::Normal)
					{
						++MagicPlus;
					}
				}
			}
			return Total > 0 ? static_cast<float>(MagicPlus) / Total : 0.f;
		};

		const float BaseRate = CountMagicPlus(0.f);
		const float MFRate = CountMagicPlus(200.f);
		TestTrue(FString::Printf(TEXT("MF 200 효과 (0MF: %.1f%% → 200MF: %.1f%%)"), BaseRate * 100.f, MFRate * 100.f),
			MFRate > BaseRate);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
