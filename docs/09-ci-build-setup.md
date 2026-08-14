# 09. 자동 빌드(CI) 셋업 — Self-hosted Runner

푸시할 때마다 회원님 PC에서 자동으로 컴파일되고, Claude가 GitHub에서 빌드 로그를
읽어 오류를 직접 수정하는 구조입니다. 클라우드에는 UE 엔진을 올릴 수 없으므로
**빌드는 회원님 PC가 수행**하고, 결과만 GitHub Actions에 기록됩니다.

```
Claude 푸시 → GitHub Actions → 회원님 PC(runner)가 컴파일 → 성공/실패 로그 업로드
→ Claude가 로그 확인 → 오류 수정 → 다시 푸시 (반복)
```

## 사전 조건

- UE 5.8과 Visual Studio 2022(C++ 게임 개발 워크로드)가 설치된 PC (docs/05 §1)
- PC가 켜져 있고 인터넷에 연결되어 있을 때만 빌드가 실행됩니다

## Runner 등록 (최초 1회, 약 10분)

1. 브라우저에서 **https://github.com/haru7821/ROH/settings/actions/runners** 접속
2. **New self-hosted runner** 클릭 → OS: **Windows**, Architecture: **x64** 선택
3. 페이지에 표시되는 명령어들을 **PowerShell**에 순서대로 복사-실행:
   - Download 섹션 (폴더 생성 + 다운로드 + 압축 해제)
   - Configure 섹션 (`./config.cmd --url ... --token ...`)
   - 설정 질문은 전부 Enter(기본값)로 진행해도 됩니다
4. 상시 실행을 원하면 서비스로 설치:
   ```powershell
   ./svc.cmd install
   ./svc.cmd start
   ```
   (또는 빌드가 필요할 때만 `./run.cmd` 실행)
5. 엔진을 기본 경로(`C:\Program Files\Epic Games\UE_5.8`)가 아닌 곳에 설치했다면:
   저장소 **Settings → Secrets and variables → Actions → Variables** 에
   `UE_PATH` 변수를 추가 (예: `D:\Epic\UE_5.8`)

## 동작 확인

1. runner 등록 후 https://github.com/haru7821/ROH/actions 접속
2. **Build** 워크플로 → **Run workflow** (수동 실행) 또는 다음 푸시를 기다림
3. 초록 체크 = 컴파일 성공. 빨간 X = 실패 (Claude에게 "빌드 실패 확인해줘"라고 하면
   로그를 읽고 수정합니다)

## 참고

- 첫 빌드는 전체 컴파일이라 오래 걸립니다(10~30분). 이후는 증분 빌드입니다
  (워크플로가 Intermediate를 유지하도록 설정됨)
- 이 CI는 **C++ 컴파일 검증**입니다. 에디터 전용 작업(맵/입력 애셋)과 플레이 검증은
  여전히 docs/05, 07 절차를 따릅니다
- runner가 꺼져 있으면 워크플로는 대기 상태로 남고, PC가 켜지면 이어서 실행됩니다
