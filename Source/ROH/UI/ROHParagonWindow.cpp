#include "UI/ROHParagonWindow.h"
#include "Character/ROHPlayerCharacter.h"
#include "Progression/ROHProgressionComponent.h"
#include "Save/ROHAccountSubsystem.h"
#include "Items/ROHItemDatabase.h" // FormatAttributeBonus (어트리뷰트 한글 표기 공용)
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h" // GetSubsystem<T>() 템플릿 인스턴스화에 완전한 타입 필요

namespace
{
	// 분류 한글명 — ID/순서의 단일 소스는 UROHAccountSubsystem::GetParagonCategories
	const TCHAR* ParagonCategoryLabel(FName Category)
	{
		if (Category == TEXT("Offense"))    { return TEXT("공격"); }
		if (Category == TEXT("Defense"))    { return TEXT("방어"); }
		if (Category == TEXT("Precision"))  { return TEXT("정밀"); }
		if (Category == TEXT("RuneAttune")) { return TEXT("룬 조율"); }
		return TEXT("?");
	}
}

int32 UROHParagonWindow::ComputeContentSerial() const
{
	const AROHPlayerCharacter* Player = GetPlayerCharacter();
	const UROHProgressionComponent* Progression = Player ? Player->GetProgression() : nullptr;
	const UROHAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UROHAccountSubsystem>() : nullptr;
	return (Account ? Account->GetChangeSerial() : 0) + (Progression ? Progression->GetChangeSerial() : 0);
}

void UROHParagonWindow::RefreshContents()
{
	if (!ContentBox)
	{
		return;
	}
	ContentBox->ClearChildren();
	StatusText = nullptr;

	AROHPlayerCharacter* Player = GetPlayerCharacter();
	const UROHProgressionComponent* Progression = Player ? Player->GetProgression() : nullptr;
	UROHAccountSubsystem* Account = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UROHAccountSubsystem>() : nullptr;
	if (!Progression || !Account)
	{
		MakeText(ContentBox, TEXT("정복자 정보를 찾을 수 없습니다"), FLinearColor(0.6f, 0.6f, 0.6f));
		return;
	}

	// 만렙(50) 전에는 잠금 안내만 (정복자 XP는 만렙 잉여 경험치만 유입)
	if (Progression->GetLevel() < UROHProgressionComponent::MaxLevel)
	{
		MakeText(ContentBox, FString::Printf(TEXT("정복자는 레벨 %d 달성 후 개방됩니다 (현재 레벨 %d)"),
			UROHProgressionComponent::MaxLevel, Progression->GetLevel()), FLinearColor(0.6f, 0.6f, 0.6f));
		MakeText(ContentBox, TEXT("만렙 이후의 경험치가 계정 정복자 경험치로 전환됩니다."),
			FLinearColor(0.45f, 0.45f, 0.45f));
		return;
	}

	MakeText(ContentBox, FString::Printf(TEXT("정복자 레벨 %d (계정 공유)"), Account->GetParagonLevel()),
		FLinearColor(1.f, 0.85f, 0.2f));
	MakeText(ContentBox, FString::Printf(TEXT("다음 레벨까지 XP: %d / %d"),
		Account->GetParagonXP(), UROHAccountSubsystem::ParagonXPForNextLevel(Account->GetParagonLevel())),
		FLinearColor::White);
	const int32 Points = Account->GetParagonPoints();
	MakeText(ContentBox, FString::Printf(TEXT("미사용 포인트: %d"), Points),
		Points > 0 ? FLinearColor(0.4f, 1.f, 0.4f) : FLinearColor(0.6f, 0.6f, 0.6f));
	StatusText = MakeText(ContentBox, FString(), FLinearColor(1.f, 0.6f, 0.3f));

	MakeText(ContentBox, TEXT("--- 분류별 투자 ([+1] = 1포인트) ---"), FLinearColor(1.f, 0.9f, 0.6f));
	const TArray<FName>& Categories = UROHAccountSubsystem::GetParagonCategories();
	for (int32 CategoryIndex = 0; CategoryIndex < Categories.Num(); ++CategoryIndex)
	{
		const FName Category = Categories[CategoryIndex];
		FGameplayAttribute BonusAttribute;
		float PerPoint = 0.f;
		if (!UROHAccountSubsystem::GetParagonCategoryBonus(Category, BonusAttribute, PerPoint))
		{
			continue;
		}
		const int32* AllocatedPtr = Account->GetParagonAllocations().Find(Category);
		const int32 Allocated = AllocatedPtr ? *AllocatedPtr : 0;

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		ContentBox->AddChild(Row);
		MakeActionButton(Row, TEXT("[+1]"), TEXT("Alloc"), CategoryIndex, FLinearColor(0.4f, 1.f, 0.4f),
			Points > 0 && Allocated < UROHAccountSubsystem::ParagonCategoryCap);
		MakeText(Row, FString::Printf(TEXT(" %s: %d/%d (포인트당 %s)"),
			ParagonCategoryLabel(Category), Allocated, UROHAccountSubsystem::ParagonCategoryCap,
			*UROHItemDatabase::FormatAttributeBonus(BonusAttribute, PerPoint)),
			Allocated > 0 ? FLinearColor::White : FLinearColor(0.75f, 0.75f, 0.75f));
	}
}

void UROHParagonWindow::OnAction(FName InActionId, int32 InActionIndex)
{
	if (InActionId == TEXT("Alloc"))
	{
		AROHPlayerCharacter* Player = GetPlayerCharacter();
		UROHAccountSubsystem* Account = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UROHAccountSubsystem>() : nullptr;
		const TArray<FName>& Categories = UROHAccountSubsystem::GetParagonCategories();
		if (Player && Account && Categories.IsValidIndex(InActionIndex))
		{
			FString Error;
			if (Account->AllocateParagonPoint(Categories[InActionIndex], Error)) // 성공 시 내부 즉시 저장
			{
				Player->ApplyParagonBonuses(); // 투자 반영 GE 재적용 (치트 ROHParagonUp과 동일)
				RefreshNow();
			}
			else if (StatusText)
			{
				StatusText->SetText(FText::FromString(Error));
			}
		}
		return;
	}
	Super::OnAction(InActionId, InActionIndex);
}
