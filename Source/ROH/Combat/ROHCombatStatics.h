#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ROHCombatStatics.generated.h"

class AROHCharacterBase;

/** 피해 유형 6종 (docs/10 §5.3) */
UENUM(BlueprintType)
enum class EROHDamageType : uint8
{
	Physical,
	Fire,
	Cold,
	Lightning,
	Poison,
	Shadow
};

/** 유형별 피해량 묶음. 무기/스킬은 여러 유형의 피해를 동시에 가질 수 있다 (디아블로2 방식). */
USTRUCT(BlueprintType)
struct FROHDamageParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float PhysicalDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float FireDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float ColdDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float LightningDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float PoisonDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float ShadowDamage = 0.f;

	/** true면 명중 굴림(AR vs Defense) 적용. 근접/원거리 '공격'은 true, 주문은 false (디아블로2 방식) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	bool bUseAttackRoll = true;
};

/**
 * 전투 계산 파이프라인 (docs/03 §3.1).
 * M1: 명중 굴림 → 저항 완화 → 피해 GE 적용을 여기서 일원화.
 * M2에서 장비 접사가 붙으면 ExecutionCalculation으로 이관 예정.
 */
UCLASS()
class ROH_API UROHCombatStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Source가 Target에게 피해를 가한다 (docs/10 §5 파이프라인). 명중 실패 시에만 false.
	 * 명중(§3.3): 2*AR/(AR+DEF) * alvl/(alvl+dlvl), 5%~95% 클램프
	 * 치명타(§3.2) → 원소 저항/PDR(§4.2~4.3) → 산포 → 치명 배율 → 룬 배율(§5.2)
	 */
	UFUNCTION(BlueprintCallable, Category = "ROH|Combat")
	static bool ApplyDamage(AROHCharacterBase* Source, AROHCharacterBase* Target, const FROHDamageParams& Params);

	/** Source 전방 원뿔 내의 적대적 생존 캐릭터 목록. HalfAngleDegrees >= 180이면 전방위. */
	UFUNCTION(BlueprintCallable, Category = "ROH|Combat")
	static TArray<AROHCharacterBase*> GetHostileTargetsInCone(AROHCharacterBase* Source, float Range, float HalfAngleDegrees);

	/** 지정 위치 기준 반경 내의 적대적 생존 캐릭터 목록. */
	UFUNCTION(BlueprintCallable, Category = "ROH|Combat")
	static TArray<AROHCharacterBase*> GetHostileTargetsInRadius(AROHCharacterBase* Source, const FVector& Origin, float Radius);

	/** Target을 Source 반대 방향으로 밀어낸다. */
	UFUNCTION(BlueprintCallable, Category = "ROH|Combat")
	static void ApplyKnockback(AROHCharacterBase* Source, AROHCharacterBase* Target, float Strength);

	/** 타격감용 히트스톱: 두 캐릭터의 시간을 잠깐 멈춘다. */
	UFUNCTION(BlueprintCallable, Category = "ROH|Combat")
	static void ApplyHitStop(AROHCharacterBase* Source, AROHCharacterBase* Target, float DurationSeconds = 0.06f);
};
