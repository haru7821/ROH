#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Items/ROHItemTypes.h"
#include "ROHAccountSubsystem.generated.h"

/**
 * 계정 데이터 소유자 (M5 최종): 정복자 성장 + 스태시 창고.
 * Initialize에서 슬롯 로드(1회면 충분 — 게임 인스턴스 수명), 변경 직후 즉시 저장
 * (계정 데이터는 잃으면 아프다 — 스태시 입출금/정복자 투자/정복자 레벨업 시점마다 SaveAccount).
 * 컴포넌트/UI는 전부 여기를 참조한다 (플레이어 폰은 GE 적용 브리지일 뿐).
 */
UCLASS()
class ROH_API UROHAccountSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static const TCHAR* AccountSlotName;
	static constexpr int32 StashCapacity = 60;
	static constexpr int32 ParagonCategoryCap = 100;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	bool SaveAccount();

	// --- 정복자 ---
	int32 GetParagonLevel() const { return ParagonLevel; }
	int32 GetParagonXP() const { return ParagonXP; }
	int32 GetParagonPoints() const { return ParagonPoints; }
	const TMap<FName, int32>& GetParagonAllocations() const { return ParagonAllocations; }

	/** 다음 정복자 레벨까지 필요한 경험치 (무한 곡선) */
	static int32 ParagonXPForNextLevel(int32 InParagonLevel) { return 500 + InParagonLevel * 250; }

	/** 만렙 잉여 경험치 유입 (ROHProgressionComponent가 라우팅). 레벨업 시 포인트 +1 + 즉시 저장 */
	void GrantParagonXP(int32 Amount);

	/** 1포인트 투자 (분류당 상한 100). 성공 시 즉시 저장 — GE 재적용은 호출측(플레이어) 책임 */
	bool AllocateParagonPoint(FName Category, FString& OutError);

	/** 분류 목록 (표시/치트 검증용, 순서 고정) */
	static const TArray<FName>& GetParagonCategories();

	/** 분류별 보정: 어트리뷰트 + 포인트당 수치. 알 수 없는 분류면 false */
	static bool GetParagonCategoryBonus(FName Category, FGameplayAttribute& OutAttribute, float& OutPerPoint);

	// --- 스태시 ---
	const TArray<FROHItemInstance>& GetStashItems() const { return StashItems; }
	bool IsStashFull() const { return StashItems.Num() >= StashCapacity; }

	/** 보관 (용량 검사 + 즉시 저장). 미감정도 허용 — 감정은 셀바에서만 */
	bool StashDeposit(const FROHItemInstance& Item);

	/** 인출 (제거 + 즉시 저장). 성공 시 OutItem에 사본 */
	bool StashWithdrawAt(int32 StashIndex, FROHItemInstance& OutItem);

	/** 변경 버전 (UI 3차 — b31): 정복자 XP/투자/스태시 입출금 시 증가. 열린 창의 dirty 폴링용 */
	int32 GetChangeSerial() const { return ChangeSerial; }

private:
	// 런타임 사본 (세이브 객체는 저장/로드 시점에만 생성)
	int32 ParagonLevel = 0;
	int32 ParagonXP = 0;
	int32 ParagonPoints = 0;

	UPROPERTY()
	TMap<FName, int32> ParagonAllocations;

	UPROPERTY()
	TArray<FROHItemInstance> StashItems;

	/** 변경 버전 (b31 — dirty 폴링) */
	int32 ChangeSerial = 0;
};
