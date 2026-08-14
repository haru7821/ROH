#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Combat/ROHCombatStatics.h"
#include "ROHProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;
class AROHCharacterBase;

/**
 * 범용 투사체. 발사자의 적대 팀에게 명중 시 피해를 적용하고 소멸한다.
 * M3에서 원소술사 스킬(화염구 등)도 이 클래스를 재사용한다.
 */
UCLASS()
class ROH_API AROHProjectile : public AActor
{
	GENERATED_BODY()

public:
	AROHProjectile();

	/** 발사 직후 호출: 발사자/피해/속도 설정 */
	void InitProjectile(AROHCharacterBase* InSource, const FROHDamageParams& InDamage, float Speed);

protected:
	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, Category = "ROH|Projectile")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, Category = "ROH|Projectile")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, Category = "ROH|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

private:
	TWeakObjectPtr<AROHCharacterBase> Source;
	FROHDamageParams DamageParams;
};
