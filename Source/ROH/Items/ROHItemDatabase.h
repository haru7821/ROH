#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Items/ROHItemTypes.h"
#include "ROHItemDatabase.generated.h"

/**
 * 아이템/접사/트레저클래스 데이터 저장소 + 생성기 (docs/03 §3.2).
 * M2: 기본 데이터를 C++로 등록. 추후 DataTable 애셋이 있으면 그것으로 대체 가능한 구조.
 * 순수 로직(월드 불필요)이라 자동화 테스트에서 NewObject로 직접 사용 가능.
 */
UCLASS()
class ROH_API UROHItemDatabase : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** 테스트에서 서브시스템 초기화 없이 데이터를 구축할 수 있도록 공개 */
	void BuildDefaultData();

	const FROHItemBaseDef* FindBase(FName BaseId) const;
	const FROHAffixDef* FindAffix(FName AffixId) const;
	const FROHTreasureClassDef* FindTreasureClass(FName TCId) const;

	/** 등급 판정: 매직파인드(MF)는 체감 곡선 적용 (docs/02 §3.4) */
	EROHItemQuality RollQuality(int32 ItemLevel, float MagicFind, FRandomStream& Rng) const;

	/** 아이템 인스턴스 생성: 등급에 따라 접사 굴림. Seed 0이면 무작위 시드 */
	FROHItemInstance GenerateItem(FName BaseId, int32 ItemLevel, EROHItemQuality Quality, int32 Seed = 0) const;

	/** 계층형 TC 굴림: NoDrop/골드/베이스/하위TC → 등급 판정 → 접사 굴림 */
	FROHDropResult RollTreasureClass(FName TCId, int32 ItemLevel, float MagicFind) const;

	/** 아이템 표시명 (등급 반영: "강철의 단검" 등) */
	FText GetItemDisplayName(const FROHItemInstance& Instance) const;

	static FColor GetQualityColor(EROHItemQuality Quality);

private:
	void RollAffixes(FROHItemInstance& Instance, const FROHItemBaseDef& Base, FRandomStream& Rng) const;
	TArray<const FROHAffixDef*> GetEligibleAffixes(const FROHItemBaseDef& Base, int32 ItemLevel, bool bPrefix) const;

	TMap<FName, FROHItemBaseDef> Bases;
	TMap<FName, FROHAffixDef> Affixes;
	TMap<FName, FROHTreasureClassDef> TreasureClasses;
};
