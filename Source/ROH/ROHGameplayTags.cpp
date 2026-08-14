#include "ROHGameplayTags.h"

namespace ROHGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage, "Data.Damage", "SetByCaller: 최종 피해량");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Cooldown, "Data.Cooldown", "SetByCaller: 쿨다운 시간(초)");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "사망 상태. 어빌리티 발동 차단");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_BasicAttack, "Cooldown.BasicAttack", "기본 공격 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_Bash, "Cooldown.Skill.Bash", "강타 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_Whirlwind, "Cooldown.Skill.Whirlwind", "회전베기 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_Leap, "Cooldown.Skill.Leap", "도약공격 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Monster_Attack, "Cooldown.Monster.Attack", "몬스터 공격 쿨다운");
}
