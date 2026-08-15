#include "UI/ROHWaypointWindow.h"
#include "Character/ROHPlayerCharacter.h"
#include "World/ROHZoneManager.h"
#include "Campaign/ROHCampaignSubsystem.h"
#include "Components/VerticalBox.h"
#include "Engine/GameInstance.h" // GetSubsystem<T>() 템플릿 인스턴스화에 완전한 타입 필요

void UROHWaypointWindow::RefreshContents()
{
	if (!ContentBox)
	{
		return;
	}
	ContentBox->ClearChildren();

	AROHPlayerCharacter* Player = GetPlayerCharacter();
	AROHZoneManager* ZoneManager = Player ? Player->GetZoneManager() : nullptr;
	const UROHCampaignSubsystem* Campaign = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UROHCampaignSubsystem>() : nullptr;
	if (!ZoneManager || !Campaign)
	{
		MakeText(ContentBox, TEXT("지역 정보를 찾을 수 없습니다"), FLinearColor(0.6f, 0.6f, 0.6f));
		return;
	}

	const int32 CurrentZone = Player ? ZoneManager->GetZoneIndexAt(Player->GetActorLocation()) : INDEX_NONE;
	for (int32 ZoneIndex = 0; ZoneIndex < ZoneManager->GetZoneCount(); ++ZoneIndex)
	{
		FString Label = ZoneManager->GetZoneName(ZoneIndex).ToString();
		if (ZoneManager->IsTown(ZoneIndex))
		{
			Label += TEXT(" (안전 지대)");
		}

		if (ZoneIndex == CurrentZone)
		{
			MakeActionButton(ContentBox, Label + TEXT(" (현재)"), TEXT("Zone"), ZoneIndex, FLinearColor::White, false);
		}
		else if (Campaign->IsWaypointActivated(ZoneIndex))
		{
			MakeActionButton(ContentBox, Label, TEXT("Zone"), ZoneIndex, FLinearColor(0.5f, 0.75f, 1.f), true);
		}
		else
		{
			MakeActionButton(ContentBox, Label + TEXT(" — 미발견"), TEXT("Zone"), ZoneIndex, FLinearColor(0.45f, 0.45f, 0.45f), false);
		}
	}
}

void UROHWaypointWindow::OnAction(FName InActionId, int32 InActionIndex)
{
	if (InActionId == TEXT("Zone"))
	{
		if (AROHPlayerCharacter* Player = GetPlayerCharacter())
		{
			Player->TravelToZone(InActionIndex);
		}
		RequestClose(); // 이동 성공/실패와 무관하게 닫기 (실패 사유는 화면 메시지)
		return;
	}
	Super::OnAction(InActionId, InActionIndex);
}
