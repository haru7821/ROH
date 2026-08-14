#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/**
 * 프로젝트 공용 네이티브 게임플레이 태그.
 * 피해 유형/상태/쿨다운의 공통 언어 (docs/03 §1).
 */
namespace ROHGameplayTags
{
	// SetByCaller 데이터
	ROH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage);
	ROH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Cooldown);

	// 상태
	ROH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);

	// 쿨다운
	ROH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_BasicAttack);
	ROH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Skill_Bash);
	ROH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Skill_Whirlwind);
	ROH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Skill_Leap);
	ROH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Monster_Attack);
}
