#include "ROHGameplayTags.h"

namespace ROHGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage, "Data.Damage", "SetByCaller: 최종 피해량");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Cooldown, "Data.Cooldown", "SetByCaller: 쿨다운 시간(초)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_BuffAttackPower, "Data.Buff.AttackPower", "SetByCaller: 버프 공격력 증가량");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_BuffDefense, "Data.Buff.Defense", "SetByCaller: 버프 방어 증가량");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_BuffAttackSpeed, "Data.Buff.AttackSpeed", "SetByCaller: 버프 공격 속도 % 증가량 (광란)");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "사망 상태. 어빌리티 발동 차단");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_BasicAttack, "Cooldown.BasicAttack", "기본 공격 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_Bash, "Cooldown.Skill.Bash", "강타 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_Whirlwind, "Cooldown.Skill.Whirlwind", "회전베기 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_Leap, "Cooldown.Skill.Leap", "도약공격 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_Execute, "Cooldown.Skill.Execute", "처형 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_BattleShout, "Cooldown.Skill.BattleShout", "전투의 함성 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_Fireball, "Cooldown.Skill.Fireball", "화염구 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_FrostNova, "Cooldown.Skill.FrostNova", "서리 신성 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_Teleport, "Cooldown.Skill.Teleport", "텔레포트 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_IceBolt, "Cooldown.Skill.IceBolt", "얼음화살 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_Meteor, "Cooldown.Skill.Meteor", "운석 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_Blizzard, "Cooldown.Skill.Blizzard", "눈보라 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_StaticField, "Cooldown.Skill.StaticField", "정전기장 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_Charge, "Cooldown.Skill.Charge", "돌진 쿨다운 (b30)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_Frenzy, "Cooldown.Skill.Frenzy", "광란 쿨다운 (b30)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_FlameWave, "Cooldown.Skill.FlameWave", "불꽃 파도 쿨다운 (b30)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_ChainLightning, "Cooldown.Skill.ChainLightning", "연쇄 번개 쿨다운 (b30)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Monster_Attack, "Cooldown.Monster.Attack", "몬스터 공격 쿨다운");
}
