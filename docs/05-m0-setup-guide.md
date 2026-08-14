# 05. M0 셋업 가이드 — 로컬에서 프로젝트 열기

M0(기반 구축)의 C++ 골격이 저장소에 포함되어 있습니다. 이 문서는 로컬 PC에서 빌드하고
에디터 작업(맵/입력 애셋)을 마무리하는 절차입니다.

## 1. 사전 준비

- **Unreal Engine 5.8** (Epic Games Launcher에서 설치, 최신 패치 버전 권장)
  - 다른 5.x 버전을 쓰려면 `ROH.uproject`의 `EngineAssociation` 수정
- **Visual Studio 2022** + "C++를 사용한 게임 개발" 워크로드 (Windows 기준)

## 2. 빌드 & 열기

1. 저장소 클론 후 `ROH.uproject` 우클릭 → **Generate Visual Studio project files**
2. 생성된 `ROH.sln` 열기 → 구성 `Development Editor` / `Win64` → 빌드
3. `ROH.uproject` 더블클릭으로 에디터 실행
   - "모듈이 없습니다 / 다시 빌드하시겠습니까?" 팝업이 뜨면 **예** (자동 빌드)

## 3. 에디터에서 마무리할 작업 (최초 1회)

> **참고: 3.1~3.3은 이제 선택 사항입니다.** 입력 애셋/BP를 만들지 않아도
> C++이 기본 입력(좌클릭 이동, 우클릭 공격, Q/W/E 스킬)을 자동 생성하고,
> 프로젝트 기본 게임모드(ROHGameMode)가 바로 동작합니다.
> 키를 바꾸고 싶을 때만 아래 절차로 애셋을 만들어 오버라이드하세요.
> **3.4(테스트 맵 + NavMesh)는 여전히 필수입니다.**

### 3.1 입력 애셋
`Content/Input/` 폴더를 만들고:

1. **Input Action** 생성 → 이름 `IA_SetDestination`, Value Type = `Digital (bool)`
2. **Input Mapping Context** 생성 → 이름 `IMC_Default`
   - Mappings에 `IA_SetDestination` 추가, 키 = **마우스 왼쪽 버튼**

### 3.2 플레이어 컨트롤러 BP
1. `Content/Blueprints/` 에서 `ROHPlayerController` 를 부모로 BP 생성 → `BP_ROHPlayerController`
2. 클래스 디폴트에서:
   - `Default Mapping Context` = `IMC_Default`
   - `Set Destination Action` = `IA_SetDestination`

### 3.3 게임모드 BP
1. `ROHGameMode` 를 부모로 BP 생성 → `BP_ROHGameMode`
2. `Player Controller Class` = `BP_ROHPlayerController`
   (Pawn은 기본값 `ROHPlayerCharacter` 유지 — 외형은 M1에서)

### 3.4 테스트 맵
1. 새 레벨(Basic) 생성 → `Content/Maps/L_Sandbox` 로 저장
2. 바닥이 충분히 넓게 (그레이박스), **NavMeshBoundsVolume** 을 바닥 전체를 덮게 배치
   - `P` 키로 초록색 내비메시가 보이면 정상
3. World Settings → GameMode Override = `BP_ROHGameMode`
4. Project Settings → Maps & Modes → Editor Startup Map / Game Default Map = `L_Sandbox`

## 4. M0 완료 기준(DoD) 검증

PIE(Play In Editor) 실행 후:

1. **클릭 이동**: 바닥 클릭 → 캐릭터(캡슐)가 그 지점으로 이동, 꾹 누르면 커서를 따라옴
2. **치트 콘솔**: `` ` `` 키로 콘솔 열고:
   ```
   ROHDumpAttrs          ← 전체 스탯 출력 (Health 100, Strength 10 등)
   ROHSetAttr Strength 50
   ROHDumpAttrs          ← Strength base=50 확인
   ```

두 가지가 동작하면 M0 완료입니다. → 다음: M1 (전투 수직 슬라이스, docs/04)

## 5. 현재 코드 구조

```
Source/ROH/
├── ROH.h/.cpp                        # 모듈, LogROH 로그 카테고리
├── Core/
│   ├── ROHGameMode                   # 기본 폰/컨트롤러 지정
│   ├── ROHGameInstance               # (M3에서 세이브 시스템 연결)
│   ├── ROHPlayerController           # 클릭/홀드 이동, Enhanced Input
│   └── ROHCheatManager               # ROHSetAttr / ROHDumpAttrs
├── Character/
│   ├── ROHCharacterBase              # ASC + AttributeSet 보유 (플레이어/몬스터 공통)
│   ├── ROHPlayerCharacter            # 쿼터뷰 카메라, 이동 방향 회전
│   └── ROHAttributeSet               # HP/마나/힘/민첩/활력/에너지/이동속도
└── Abilities/
    ├── ROHAbilitySystemComponent     # ASC 서브클래스 (확장 지점)
    └── ROHGameplayAbility            # 스킬 베이스 (M1에서 비용/쿨다운/시너지)
```

## 6. 알려진 제약

- 이 저장소는 코드/설정만 버전 관리합니다. `Content/` 의 `.uasset` 은 위 절차로 로컬 생성
  후 커밋하세요 (바이너리이므로 필요 시 Git LFS 도입 검토).
- 코드는 UE 5.4 API 기준으로 작성되었으며 **CI 없는 환경에서 작성되어 로컬 첫 빌드 시
  컴파일 오류가 있으면 리포트해 주세요** — 다음 세션에서 바로 수정합니다.
