#pragma once

#include "CoreMinimal.h"

class UStaticMesh;
class UMaterialInterface;

/**
 * 애셋 카탈로그 (M6 그래픽 패스 1차 — b32, docs/13).
 * 키 → "/Game/ROH/Art/<키>.<키>" 명명 규약의 단일 소스: 사용자가 Fab 애셋을 규약 경로로
 * 임포트만 하면 코드 수정 없이 그레이박스가 실물 메시로 교체된다.
 * 애셋이 없으면 nullptr — 호출측은 기존 그레이박스 폴백을 그대로 탄다
 * (애셋 0개 상태 = 종전 동작 100% 유지, 실패 경고 스팸 없음 — 패키지 존재를 먼저 조용히 확인).
 *
 * 키 전체 목록 (스켈레탈 이관은 M6 2차 — 현 단계는 전부 스태틱 메시):
 *  [캐릭터]  SM_Player_Warrior, SM_Player_Elementalist,
 *            SM_Monster_Grunt, SM_Monster_Archer, SM_Monster_Charger,
 *            SM_Monster_Brute, SM_Monster_Hexer, SM_Monster_Stalker,
 *            SM_Boss_Baltar, SM_Boss_Morgath
 *  [월드]    SM_Waypoint, SM_StashChest,
 *            SM_Npc_General, SM_Npc_Blacksmith, SM_Npc_Jeweler,
 *            SM_Npc_PotionVendor, SM_Npc_Gambler, SM_Npc_Identifier
 *  [투사체]  SM_Projectile (공용 폴백),
 *            SM_Projectile_Fire, SM_Projectile_Ice, SM_Projectile_Lightning, SM_Projectile_Shadow
 *  [머티리얼] 위 모든 메시 키에 선택적으로 "M_<키 몸통>" (예: M_Waypoint) —
 *            존재하면 슬롯 0 오버라이드 (메시 적용 성공 시에만 조회)
 */
namespace ROHAssetCatalog
{
	/** 규약 경로의 스태틱 메시 로드. 없으면 nullptr (경고 스팸 없음) */
	ROH_API UStaticMesh* LoadMesh(FName Key);

	/** "SM_..." 메시 키의 짝 머티리얼("M_...") 로드. 없으면 nullptr */
	ROH_API UMaterialInterface* LoadMaterialForMeshKey(FName MeshKey);
}
