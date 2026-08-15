#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayEffectTypes.h" // FActiveGameplayEffectHandle (b29 % 배율 GE)
#include "ROHCharacterBase.generated.h"

class UROHAbilitySystemComponent;
class UROHAttributeSet;
class UROHGameplayAbility;
class UStaticMeshComponent;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FROHDeathSignature, AROHCharacterBase*, DeadCharacter);

/**
 * 플레이어/몬스터 공통 베이스.
 * 싱글플레이 전제로 ASC를 캐릭터에 직접 부착한다 (Action RPG 샘플 방식).
 * 멀티 확장 시 플레이어만 PlayerState로 옮기면 되도록 InitAbilityActorInfo를 분리해 둔다.
 */
UCLASS(Abstract)
class ROH_API AROHCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AROHCharacterBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UROHAttributeSet* GetAttributeSet() const { return AttributeSet; }

	/** 0 = 플레이어 진영, 1 = 몬스터 진영 */
	uint8 GetTeamId() const { return TeamId; }

	bool IsAlive() const;

	/** 레벨업/세이브 복원 시 프로그레션 컴포넌트가 참조 (docs/10 §2) */
	float GetHealthPerLevel() const { return HealthPerLevel; }
	float GetManaPerLevel() const { return ManaPerLevel; }

	/** 피해 반영 직후 호출 (피격 연출 훅). AttributeSet에서 호출한다. */
	virtual void HandleDamageTaken(float Damage, AActor* InstigatorActor);

	/** 생명력 0 도달 시 호출. AttributeSet에서 호출한다. */
	virtual void HandleDeath(AActor* Killer);

	UPROPERTY(BlueprintAssignable, Category = "ROH")
	FROHDeathSignature OnDeath;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	void InitAbilityActorInfo();

	/** 기본 스탯 프로퍼티를 어트리뷰트에 반영. 파생 공식 포함 (docs/10 §2) */
	virtual void InitializeAttributes();

	void GrantDefaultAbilities();
	FGameplayAbilitySpecHandle GrantAbility(TSubclassOf<UROHGameplayAbility> AbilityClass);

	/** MoveSpeed 어트리뷰트 변경을 CharacterMovement에 반영 */
	void OnMoveSpeedChanged(const FOnAttributeChangeData& Data);

	/** HealthPct/ManaPct 변경 → % 배율 GE 재적용 (b29 — docs/10 §1/§2) */
	void OnVitalPercentChanged(const FOnAttributeChangeData& Data);

	/**
	 * 최대 HP/MP % 배율 반영 (docs/10 §2 완전형 — b29):
	 * HPmax = (베이스 + Flat가산) × (1 + HealthPct/100) — MultiplyAdditive 무한 GE 1개로 적용.
	 * % 가 전부 0이면 GE를 걸지 않는다 (기존 수치와 완전 동일 보장). 장비 착탈로 % 변경 시 재적용.
	 */
	void RefreshVitalPercentScaling();

	UPROPERTY(VisibleAnywhere, Category = "ROH|Abilities")
	TObjectPtr<UROHAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UROHAttributeSet> AttributeSet;

	/** 그레이박스 시각화용 캡슐 메시 (아트 반영 전 임시) */
	UPROPERTY(VisibleAnywhere, Category = "ROH|Visual")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	/** 스폰/빙의 시 부여할 어빌리티 */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Abilities")
	TArray<TSubclassOf<UROHGameplayAbility>> DefaultAbilities;

	// --- 기본 스탯 (InitializeAttributes에서 어트리뷰트로 반영) ---
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Stats")
	float BaseStrength = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Stats")
	float BaseDexterity = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Stats")
	float BaseVitality = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Stats")
	float BaseEnergy = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Stats")
	float BaseMaxHealth = 50.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Stats")
	float BaseMaxMana = 20.f;

	/** 레벨당 최대 생명력 성장 (docs/10 §2 — 몬스터는 0으로 기존 고정 HP 유지) */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Stats")
	float HealthPerLevel = 0.f;

	/** 레벨당 최대 마나 성장 (docs/10 §2) */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Stats")
	float ManaPerLevel = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Stats")
	float BaseMoveSpeed = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Stats")
	float BaseLevel = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Team")
	uint8 TeamId = 0;

private:
	bool bAbilitiesGranted = false;
	bool bAttributeDelegatesBound = false;

	/** % 배율 GE 핸들 (b29 — 재적용 시 제거용) */
	FActiveGameplayEffectHandle VitalPctEffectHandle;

	// 상태 표시용 몬스터 수 캐시 (0.25초 간격 갱신)
	float MonsterCountTimer = 0.f;
	int32 CachedAliveMonsters = 0;
};
