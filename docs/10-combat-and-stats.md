# 10. 전투 및 스탯 알고리즘 (정식 사양)

> 소유자 제공 설계 문서 (2026-08). 기존 docs/02·03의 전투 수식과 충돌 시 **이 문서가 우선**한다.
> 구현 현황: 1단계(§3.2, §4.2, §4.3, §5 전체) — b18. 2단계(§2 HP/MP/재생, §3.4 속도→쿨다운, §4.1) — b20.
> 미적용: §1 3단 구조의 % 배율 어트리뷰트(아이템 % 옵션 도입 시), §3.1 AP/SP 완전형.
> 참고: 코드의 `Energy` 어트리뷰트가 본 문서의 INT 역할을 승계한다 (세이브 호환 유지, 명칭 이관은 추후).

## 1. 스탯 아키텍처

모든 엔티티는 기본(Base) / 추가(Flat) / 배율(Multiplier) 3단계 구조로 최종 스탯을 산출한다.

```
FinalStat = (BaseStat + AddedFlat) × (1 + TotalPercentMultiplier/100)
```

- AddedFlat: 아이템·패시브로 추가되는 고정 수치
- TotalPercentMultiplier: 퍼센트 증가치의 합 (10% + 10% = 20%)

### 1.2 주 스탯

| 스탯 | 역할 |
|------|------|
| 힘 (STR) | 물리 공격력, 물리 피해 감소, 장비 무게 |
| 민첩 (DEX) | 공격 속도, 치명타 확률, 적중률, 회피율 |
| 지능 (INT) | 주문 공격력, 최대 마나, 마나 재생, 저항력 |
| 체력 (VIT) | 최대 체력, 체력 재생, 물리 방어력 |

## 2. 생존/리소스 (2단계 예정)

```
HPmax   = (HPbase + (레벨−1)×HPPerLevel + VIT×5 + FlatHP) × (1 + %HP/100)   [레벨 1 = 기본치]
MPmax   = (MPbase + (레벨−1)×MPPerLevel + INT×2 + FlatMP) × (1 + %MP/100)
HPRegen = (HPRegenBase + VIT×0.05 + Flat) × (1 + %/100)   [초당]
MPRegen = (MPRegenBase + INT×0.1  + Flat) × (1 + %/100)   [초당]
```

- HPbase: 전사 100 / 원소술사 60, HPPerLevel: 10 / 5
- MPbase: 전사 20 / 원소술사 50, MPPerLevel: 2 / 5

## 3. 오펜스

### 3.1 공격력 (2단계에서 완전 적용)

```
FinalAP = (WeaponDamageBase + FlatAP) × (1 + STR/100 + Physical%Inc/100) × (1 + Global%Inc/100)
FinalSP = (SkillSpellDamageBase + FlatSP) × (1 + INT/100 + Elemental%Inc/100) × (1 + Global%Inc/100)
```

### 3.2 치명타 (1단계 적용)

```
Crit%    = CritBase%(5) + DEX/50 + FlatCrit%          (캡 95%)
CritDmg% = CritBaseDmg%(150) + FlatCritDmg%
```

### 3.3 적중률 (기존 구현 유지)

```
Hit% = 2 × AR/(AR+DR) × Lv공격/(Lv공격+Lv방어) × 100   (캡 5~95%)
AR = DEX×5 + FlatAR
```

### 3.4 공격/시전 속도 (2단계 예정)

```
AS = WeaponBaseSpeed × (1 + DEX/100 + %AS/100 − ArmorPenalty%/100)
FinalCastTime = SkillBaseCastTime / (1 + INT/200 + %CS/100)
```

## 4. 디펜스

### 4.1 방어 등급 (DR) — 적중률 판정에 사용

```
DR = (ArmorBase + FlatDR) × (1 + VIT/100) × (1 + %DR/100)
```

### 4.2 저항력 (1단계 적용)

속성: 화염, 냉기, 번개, 독, 그림자. 퍼센트 감쇄.

```
RESfinal% = (FlatRES + INT×0.1) − DifficultyPenalty   (캡 75%, 아이템/스킬 확장 시 최대 95%, 하한 −100%)
```

- DifficultyPenalty: 노말 0 / 악몽 25 / 지옥 50

### 4.3 물리 피해 감소 (PDR, 1단계 적용)

저항과 별개로 물리 최종 피해를 퍼센트 감소. 캡 90%.

```
PDRfinal% = FlatPDR% + VIT/100 + Total%PDR   (캡 90%)
```

- 코드의 `PhysicalResistance` 어트리뷰트가 FlatPDR% 역할을 담당한다.

## 5. 최종 피해 계산 및 룬 (1단계 적용)

### 5.1 최종 피해량

```
FinalDamage = ModifiedBaseDamage × IsCrit × DamageDispersion
              × (1 − TargetRESfinal) × (1 − TargetDRfinal) × RuneMultiplier
```

- IsCrit: 치명타 성공 시 CritDmg%/100, 실패 시 1.0
- DamageDispersion: 0.95 ~ 1.05 난수
- RuneMultiplier: §5.2

### 5.2 룬 시스템

```
RuneMultiplier = 1 + Σ RuneEffectValue
```

- 룬은 최종 피해에 **곱연산** — Global%Inc보다 강력
- 1단계 구현: `RunePower` 어트리뷰트(%)를 합산원으로 사용, "룬각인" 접사가 공급.
  룬/소켓/룬워드 본체(M5)가 이 위에 RuneEffectValue를 얹는다.

### 5.3 피해 유형 및 적용 순서

속성 6종: 물리(Physical), 화염(Fire), 냉기(Cold), 번개(Lightning), 독(Poison), 그림자(Shadow)

1. 기술 발동: 기본 AP/SP 계산
2. 스탯/퍼센트 보정
3. 치명타 판단 (성공 시 치명 피해 적용)
4. 적중 판단 (AR vs DR, 물리 공격만)
5. 대상 방어: 속성 저항 → 물리 피해 감소(PDR)
6. 룬 배율 적용
7. 최종 피해 발생

## 6. 구현 노트 (UE/GAS)

- 스탯은 `UROHAttributeSet`에 정의, 변경은 GameplayEffect로만 (CLAUDE.md 원칙)
- 피해 파이프라인은 `UROHCombatStatics::ApplyDamage` 일원화
- 피해 유형별 태그(Damage.Physical 등)는 필요 시점(상태이상/발동조건 도입)에 추가
- 수치는 테스트/피드백으로 계속 조정한다
