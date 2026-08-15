#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AttributeSet.h"
#include "ROHGameplayAbility.generated.h"

class AROHCharacterBase;
class UNiagaraSystem;
class USoundBase;
class UCameraShakeBase;

/**
 * 모든 스킬의 베이스 (docs/03 §1).
 * - 자원 비용: CostAttribute에서 CostAmount 차감 (전사=Rage, 원소술사=Mana)
 * - 쿨다운: 공용 UROHCooldownEffect + CooldownTags (SetByCaller 지속시간)
 * - 타격 피드백: 이펙트/사운드/카메라셰이크/히트스톱 훅
 * M3에서 스킬트리 하드 포인트/시너지 계수 조회가 여기에 얹힌다.
 */
UCLASS()
class ROH_API UROHGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UROHGameplayAbility();

	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	/** 커밋 성공 시 자원 차감을, 실패 시 사유(쿨다운/자원 부족)를 화면에 표시 (플레이어 한정) */
	virtual bool CommitAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	AROHCharacterBase* GetROHCharacter() const;

	/** 플레이어 전용: 마우스 커서 아래 지면 좌표. 실패 시 전방 기본 지점. */
	FVector GetCursorLocation() const;

	/** 캐릭터를 지정 좌표 방향(Yaw만)으로 즉시 회전 */
	void FaceLocation(const FVector& Location) const;

	/** 타격 성공 시 공통 피드백 (이펙트/사운드/셰이크/히트스톱 + 디버그 표시) */
	void PlayHitFeedback(AROHCharacterBase* Target) const;

	/** 그레이박스용: 스킬 발동 범위를 잠깐 표시 (아트 이펙트가 붙기 전 시각 피드백) */
	void DebugDrawSwing(const FVector& Center, float Radius) const;

	/**
	 * 스킬트리 연동: 랭크 미투자 시 발동 차단 + 랭크/시너지 피해 배수.
	 * SkillId가 None이면 트리와 무관 (기본 공격 등) → 배수 1.
	 */
	bool CheckSkillInvested() const;
	float GetSkillDamageMultiplier() const;

	/** 스킬트리 상의 식별자 (None = 트리 미연동) */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Skill")
	FName SkillId;

	// --- 비용 ---
	/** 소모 자원 어트리뷰트 (예: Rage, Mana). 비워두면 비용 없음 */
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Cost")
	FGameplayAttribute CostAttribute;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Cost")
	float CostAmount = 0.f;

	// --- 쿨다운 ---
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Cooldown")
	float CooldownDuration = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Cooldown")
	FGameplayTagContainer CooldownTags;

	// --- 타격 피드백 ---
	UPROPERTY(EditDefaultsOnly, Category = "ROH|Feedback")
	TObjectPtr<UNiagaraSystem> HitEffect;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Feedback")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Feedback")
	TSubclassOf<UCameraShakeBase> HitCameraShake;

	UPROPERTY(EditDefaultsOnly, Category = "ROH|Feedback")
	float HitStopSeconds = 0.05f;

private:
	/** GetCooldownTags 반환용 임시 컨테이너 (엔진 관례 패턴) */
	UPROPERTY(Transient)
	FGameplayTagContainer TempCooldownTags;
};
