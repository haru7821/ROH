# 13. M6 그래픽 패스 가이드 (Fab 애셋 적용)

> 대상: 소유자(에디터 작업). 코드는 b32부터 **애셋 카탈로그** 방식 —
> 정해진 경로·이름으로 애셋을 임포트만 하면 코드 수정 없이 그레이박스가
> 실물로 교체된다. 애셋이 없으면 지금처럼 그레이박스로 표시된다 (폴백 유지).

## 0. 원칙

- 코드/블루프린트 수정 없음. **임포트 → 이름 변경 → 폴더 이동**만 하면 된다.
- 한 개씩 적용해도 된다. 예: 웨이포인트 메시 하나만 넣어도 그것만 교체된다.
- 임포트 위치는 항상 콘텐츠 브라우저의 `Content/ROH/Art` 폴더
  (경로 `/Game/ROH/Art`). 없으면 우클릭 → 새 폴더로 만든다.
- 애셋 이름은 아래 키 표와 **정확히 일치**해야 한다 (대소문자 포함).

## 1. 단계 로드맵

| 단계 | 내용 | 비고 |
|---|---|---|
| M6-1 (지금) | 소품/월드/투사체 정적 메시 교체 | 이 문서 §3 |
| M6-2 | 캐릭터 스켈레탈 메시 + 애니메이션 | 코드 지원 후 별도 안내 |
| M6-3 | 나이아가라 이펙트 (스킬/고대 잭팟 연출) | 코드 지원 후 별도 안내 |
| M6-4 | 환경 꾸미기 (지형/건물/조명) | docs/11 방식 — 에디터 자유 배치 |

## 2. Fab에서 애셋 받기 (에픽게임즈 런처)

1. 에픽게임즈 런처 → **Fab** (또는 브라우저 fab.com, 에픽 계정 로그인).
2. 검색 시 **무료(Free)** 필터 + **Unreal Engine** 형식 필터.
3. "내 라이브러리에 추가" 후, 런처의 라이브러리 → 해당 애셋 →
   **프로젝트에 추가** → ROH 프로젝트 선택.
4. 추천 검색어 (무료 위주, 시기에 따라 목록은 달라질 수 있음):
   - 환경: `Infinity Blade: Grass Lands`, `Infinity Blade: Ice Lands`,
     `Stylized Fantasy Environment`, `Medieval Village`
   - 소품: `Infinity Blade: Props`, `Treasure Chest`, `Fantasy Props`
   - 캐릭터(M6-2용): `Paragon` (에픽 무료 영웅들 — 애니메이션 포함)
   - 이펙트(M6-3용): `Niagara VFX`, `FX Variety Pack`
5. 프로젝트에 추가된 애셋은 `Content/<팩 이름>` 폴더에 들어온다.
   거기서 마음에 드는 스태틱 메시를 찾아 **복제(Ctrl+W) → 이름 변경 →
   `Content/ROH/Art`로 이동**한다 (원본 팩은 그대로 두기).

## 3. 애셋 키 표 (M6-1: 스태틱 메시)

`Content/ROH/Art`에 아래 이름으로 두면 자동 적용된다.
같은 이름에 `M_` 접두 머티리얼을 함께 두면 슬롯 0에 덧입혀진다
(예: `SM_Waypoint` + `M_Waypoint`).

### 월드 소품

| 키 (파일 이름) | 대상 |
|---|---|
| `SM_Waypoint` | 웨이포인트 기둥 |
| `SM_StashChest` | 계정 보관함 상자 |

### 마을 NPC (6인)

| 키 | 대상 |
|---|---|
| `SM_Npc_General` | 잡화상 로사 |
| `SM_Npc_Blacksmith` | 대장장이 브란 |
| `SM_Npc_Jeweler` | 보석상 일렌 |
| `SM_Npc_PotionVendor` | 포션상인 미로 |
| `SM_Npc_Gambler` | 도박사 카론 |
| `SM_Npc_Identifier` | 감정사 셀바 |

### 캐릭터 (M6-1은 임시 정적 메시 — M6-2에서 스켈레탈로 대체 예정)

| 키 | 대상 |
|---|---|
| `SM_Player_Warrior` | 전사 |
| `SM_Player_Elementalist` | 원소술사 |
| `SM_Monster_Grunt` | 졸개 |
| `SM_Monster_Archer` | 궁수 |
| `SM_Monster_Charger` | 돌격병 |
| `SM_Monster_Brute` | 덩치 |
| `SM_Monster_Hexer` | 주술사 |
| `SM_Monster_Stalker` | 추적자 |
| `SM_Boss_Baltar` | 보스 발타르 |
| `SM_Boss_Morgath` | 보스 모르가스 |

### 투사체 (속성별 — 없으면 공용 `SM_Projectile`로 폴백)

| 키 | 대상 |
|---|---|
| `SM_Projectile` | 공용 (속성별 키가 없을 때) |
| `SM_Projectile_Fire` | 화염 계열 (화염구/화염탄 등) |
| `SM_Projectile_Ice` | 냉기 계열 (얼음화살 등) |
| `SM_Projectile_Lightning` | 번개 계열 |
| `SM_Projectile_Shadow` | 그림자 계열 (주술사 투사체) |

## 4. 절차 예시 (웨이포인트 교체)

1. Fab에서 소품 팩을 프로젝트에 추가.
2. 콘텐츠 브라우저에서 기둥/비석 느낌의 스태틱 메시 선택 → Ctrl+W 복제.
3. 이름을 `SM_Waypoint`로 변경.
4. `Content/ROH/Art` 폴더로 드래그해 이동 (이동 선택).
5. 저장(Ctrl+Shift+S) → 플레이 → 웨이포인트가 새 메시로 표시되는지 확인.
   - 크기는 코드가 캡슐/기존 치수에 맞춰 자동 정규화한다.
   - 안 바뀌면: 이름 오타/폴더 위치 확인 → 출력 로그에서 `LogROH` 검색.

## 5. 환경 꾸미기 (M6-4, 자유 작업)

- 바닥/벽/건물은 액터 배치라 코드와 무관 — 팩의 메시를 레벨에 직접 배치.
- docs/11의 존 좌표(마을 0 / 들판 6000 / 지하묘지 12000 / 성소 18000, +X 방향)
  범위 안에 배치하면 된다. 내비메시 볼륨 범위 유지 필수.
- 조명/포스트프로세스는 마지막에 (그레이박스 시인성 우선).

## 6. (예약) 패키징 시 주의

카탈로그 애셋은 코드가 문자열 경로로만 참조하므로, 나중에 게임을
패키징(쿡)할 때는 프로젝트 세팅 → Packaging →
**Additional Asset Directories to Cook**에 `/Game/ROH/Art`를 추가해야
누락되지 않는다. (에디터 실행 워크플로에서는 해당 없음)
