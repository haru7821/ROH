# 08. M2 셋업 가이드 — 아이템 & 드랍

M1 셋업(docs/07) 위에 추가 에디터 작업은 **없습니다**. 코드만으로 동작합니다.

## 1. 플레이 루프

몬스터를 처치하면 트레저 클래스(`TC_Default`)가 굴려져 시체 주위로 드랍이 흩어집니다:
- **이름표 색상**: 흰색(일반) / 파란색(매직) / 노란색(레어) / 금색 골드
- 드랍 위로 걸어가면 자동 습득 (인벤토리 40칸, 가득 차면 바닥에 남음)
- 골드는 즉시 잔액에 합산

장비/확인은 콘솔(`)로:
```
ROHDumpInventory          ← 골드/장비창/인벤토리 목록
ROHEquip 0                ← 인벤토리 0번 장착 (같은 슬롯 기존 장비는 인벤토리로)
ROHDumpAttrs              ← 장착 후 AttackPower/Defense/접사 스탯 반영 확인
ROHUsePotion              ← 치유물약 사용
ROHAddGold 500 / ROHBuyPotion   ← 상점 최소 구현 (NPC UI는 M4)
ROHGiveItem BattleAxe 6 rare    ← 임의 아이템 생성
ROHSimulateDrops TC_Default 10000   ← 드랍 통계 (등급 분포/골드 평균)
```

## 2. 구현된 시스템 (docs/02 §3 대응)

| 시스템 | 내용 |
|--------|------|
| 아이템 구조 | Base 정의(10종) + Instance(시드 저장 → 굴림 재현 가능) |
| 접사 생성기 | 접두 6종 + 접미 9종, ilvl 게이트, 슬롯 제한, 매직 1~2 / 레어 3~6 |
| 등급 판정 | 매직 20%/레어 5% 기본, **MF 체감 곡선**(MF×250/(MF+250)) 반영 |
| 트레저 클래스 | 계층형(TC_Default → TC_Weapons/TC_Armor), NoDrop/골드/물약, 보스용 TC_Boss(3회 추첨) |
| 장비 장착 | 무기→AttackPower, 방어구→Defense, 접사 전부 **런타임 GameplayEffect**로 적용/제거 |
| 피해 연동 | 전사 스킬 공식 = (기본 피해 + AttackPower) × (1 + 힘×1%) |
| 물약/골드 | 즉시 회복 물약, 골드 습득/소비, 콘솔 상점 |
| 드랍 시뮬레이터 | 콘솔 통계 명령 + **자동화 테스트** (Session Frontend > Automation > "ROH.Loot") |

## 3. M2 완료 기준(DoD) 검증

1. 몬스터 처치 → 색상별 드랍 → 습득 → 장착 → `ROHDumpAttrs`에서 스탯 상승 확인
2. 좋은 무기 장착 후 몬스터가 눈에 띄게 빨리 죽는다 (강해짐 체감)
3. `ROHSimulateDrops TC_Default 10000` 결과가 설계 수치와 일치:
   - NoDrop ≈ 35%, 등급 분포 매직 ≈ 20%/레어 ≈ 5% (장비 기준)
4. 자동화 테스트 "ROH.Loot.DropSimulator" 통과 (데이터 정합성/접사 규칙/시드 재현성/통계/MF 효과)

## 4. 의도적 단순화 (후속 마일스톤)

- 인벤토리는 40칸 목록형 — 격자 배치·아이템 크기 UI는 M6 (데이터에 GridSize 준비됨)
- 습득은 접촉 방식 — 클릭 습득 + 이름표 UMG는 UI 패스에서
- 아이템/접사 데이터는 C++ 등록 — DataTable/CSV 이관 구조 준비됨 (FTableRowBase 상속)
- 상점은 콘솔 명령 — NPC/UI는 M4 마을 구현과 함께
- 유니크/소켓/룬은 M5
