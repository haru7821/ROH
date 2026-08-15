#pragma once

#include "CoreMinimal.h"
#include "UI/ROHUiWindow.h"
#include "Items/ROHItemTypes.h"
#include "ROHVendorWindow.generated.h"

class AROHTownNpc;
class UTextBlock;

/**
 * 마을 NPC 벤더 창 (docs/12) — 역할 6종을 한 클래스가 분기 처리.
 * 로사: 매입+일반 장비 / 브란: 마법 장비(방문마다 갱신) / 일렌: 룬 매매 /
 * 미로: 물약 / 카론: 도박 / 셀바: 감정 (개당 50골드).
 */
UCLASS()
class ROH_API UROHVendorWindow : public UROHUiWindow
{
	GENERATED_BODY()

public:
	/** 컨트롤러가 AddToViewport 전에 호출 (타이틀/내용은 RefreshContents가 반영). 정의는 cpp — TWeakObjectPtr 대입에 완전한 타입 필요 */
	void SetNpc(AROHTownNpc* InNpc);

	virtual void OnAction(FName InActionId, int32 InActionIndex) override;
	virtual void RefreshContents() override;

	/** 감정 비용 (docs/12: 개당 50골드) */
	static constexpr int32 IdentifyCost = 50;

private:
	/** 구매 공통: 공간 → 골드 순 검증 후 지급. 결과/사유는 OutStatus */
	bool TryPurchase(const FROHItemInstance& Item, int32 Price, FString& OutStatus);

	void ShowStatus(const FString& Message);

	TWeakObjectPtr<AROHTownNpc> Npc;

	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;

	/** 브란 재고 — 창 인스턴스 수명 = 방문마다 갱신 (docs/12) */
	UPROPERTY()
	TArray<FROHItemInstance> SmithStock;

	bool bSmithStockRolled = false;
};
