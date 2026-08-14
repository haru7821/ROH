#include "Core/ROHCheatManager.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"
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
