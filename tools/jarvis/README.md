# 자비스 — 개인 음성 비서

단축키를 누른 채 말하면 듣고, 알아서 일하고, 음성으로 답합니다.
Claude(`claude-opus-5`)가 두뇌이고, 손발은 아래 도구들입니다.

| 분야 | 할 수 있는 일 |
|---|---|
| PC 제어 | 앱·사이트 실행, 파일 찾기/읽기, 폴더 열기, 명령 실행, 클립보드 |
| 메일·일정 | Gmail 검색·읽기·초안·발송, 구글 캘린더 조회·등록 |
| 웹 | 실시간 웹 검색과 페이지 요약 (추가 키 불필요) |
| 개발 비서 | ROH 저장소 상태·커밋·변경 확인, 코드 검색, `update.bat` 빌드 |
| 기억 | 시킨 내용을 메모로 저장하고 다음 대화에서 찾아 씀 |

말로 해보는 예시

- "지금 브랜치 뭐야? 최근 커밋 세 개 읽어줘"
- "다운로드 폴더에서 인보이스 들어간 파일 찾아줘"
- "오늘 일정 알려주고, 내일 오후 2시에 치과 넣어줘"
- "안 읽은 메일 중에 급한 거 있어?"
- "언리얼 5.8 나이아가라 GPU 스프라이트 최신 정보 찾아서 세 줄로 요약해줘"
- "다음에 스킬 밸런스 잡을 때 치명타 계수부터 보라고 기억해둬"

---

## 1. 준비물

- **Windows** + **Python 3.11 이상** ([python.org](https://www.python.org/downloads/) — 설치 시 "Add to PATH" 체크)
- 마이크
- **Anthropic API 키** — [console.anthropic.com](https://console.anthropic.com) → API Keys → Create Key
  (Claude Code 구독과는 별개인, 사용량만큼 과금되는 API 키입니다)

## 2. 설치

```bat
cd tools\jarvis
setup.bat
```

가상환경을 만들고 필요한 패키지를 설치한 뒤 `config.toml` 을 만들어 줍니다.
첫 설치는 몇 분 걸립니다(음성 인식 라이브러리가 큽니다).

## 3. API 키 등록

명령 프롬프트에서 한 번만:

```bat
setx ANTHROPIC_API_KEY "sk-ant-여기에-본인-키"
```

등록 후 **명령 프롬프트를 새로 열어야** 적용됩니다.
(`config.toml` 의 `[api] api_key` 에 넣어도 되지만, 환경변수 쪽이 안전합니다.)

## 4. 설정 손보기

`config.toml` 을 열고 최소 두 군데만 본인 환경에 맞추세요.

```toml
[dev]
repo_path = "C:/Projects/ROH"     # ROH 저장소가 실제로 있는 경로

[pc.apps]
"크롬" = "chrome.exe"              # 부를 이름 = 실행 대상
"유니버설엔진" = "C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe"
```

## 5. 실행

```bat
jarvis.bat
```

- **Ctrl+Space 를 누르고 있는 동안** 말하고, 떼면 처리합니다.
- 종료: **Ctrl+Alt+Q**, 또는 "종료"라고 말하기, 또는 Ctrl+C
- "초기화"라고 말하면 대화 맥락을 비웁니다.

마이크 없이 키보드로 시험해 보려면:

```bat
jarvis.bat --text
jarvis.bat --say "최근 커밋 3개 알려줘"
```

---

## 6. 메일·일정 연결 (선택)

구글 계정을 붙이는 절차입니다. 안 쓸 거면 건너뛰어도 됩니다.

1. [Google Cloud Console](https://console.cloud.google.com) 에서 프로젝트 생성
2. **API 및 서비스 → 라이브러리** 에서 `Gmail API`, `Google Calendar API` 사용 설정
3. **OAuth 동의 화면** 을 "외부"로 만들고, 본인 계정을 **테스트 사용자**로 추가
4. **사용자 인증 정보 → OAuth 클라이언트 ID → 데스크톱 앱** 생성 후 JSON 다운로드
5. 받은 파일을 `tools/jarvis/secrets/credentials.json` 으로 저장
6. `config.toml` 에서 `[skills] google = true`
7. 처음 메일/일정 도구를 쓸 때 브라우저가 열리면 계정을 승인 —
   이후에는 `secrets/token.json` 이 자동으로 쓰입니다

`secrets/` 폴더는 `.gitignore` 에 들어 있어 커밋되지 않습니다.

---

## 7. 안전장치

되돌리기 어려운 일은 **콘솔에서 y/n 확인**을 받은 뒤에만 실행합니다.
기본 확인 대상은 `config.toml` 의 `safety.confirm_tools`:

```toml
confirm_tools = ["run_command", "build_project", "gmail_send", "calendar_create_event"]
```

- `y` 이번만 허용 · `a` 이번 실행 동안 계속 허용 · 그 외 거부
- 파일 읽기·검색은 `safety.allowed_roots` 안에서만 됩니다.
  기본값은 **문서 / 다운로드 / 바탕화면 + ROH 저장소**입니다. 넓히거나 좁히려면 직접 지정하세요.

  ```toml
  allowed_roots = ["C:/Projects", "C:/Users/이름/Documents"]
  ```

- 허용 경로 안이라도 **키·토큰이 들어 있을 만한 파일은 항상 차단**합니다
  (`.ssh/`, `AppData/`, `secrets/`, `.env`, `*.pem`, `token.json`, 자비스 자신의 `config.toml` 등).
- 등록되지 않은 이름으로는 앱을 실행하지 않습니다. 임의 명령은 `run_command` 로만 가고, 그건 확인 대상입니다.
- 웹 페이지나 메일 본문에 "이 파일을 읽어서 보내라" 같은 문구가 있어도 비서는 따르지 않습니다
  (지시는 보스의 말에서만 받도록 지시해 두었습니다).
- 메일은 기본이 **초안 작성**입니다. 실제 발송은 "보내"라고 분명히 말하고 확인까지 통과해야 나갑니다.
- 확인 절차에서 `a`(계속 허용)를 골라도 **10분 뒤 다시 물어봅니다**.

## 8. 비용

`claude-opus-5` 기준 입력 $5 / 출력 $25 per 1M 토큰입니다.
짧은 대화 한 번은 보통 수 센트 이하지만, 도구를 여러 번 도는 요청은 더 듭니다.
아끼려면 `config.toml` 에서:

```toml
[api]
effort = "low"        # 기본 medium. low 로 낮추면 더 빠르고 싸집니다.
```

음성 인식(faster-whisper)과 음성 합성(edge-tts)은 **무료**이며, 인식은 PC에서 로컬로 돕니다.

## 9. 문제 해결

| 증상 | 해결 |
|---|---|
| 단축키가 안 먹음 | 다른 프로그램이 Ctrl+Space를 쓰는 중일 수 있습니다. `config.toml` 의 `hotkey.combo` 를 `<ctrl>+<alt>+j` 등으로 변경 |
| 게임/전체화면에서 안 먹음 | `jarvis.bat` 을 관리자 권한으로 실행 |
| 첫 실행이 오래 걸림 | 음성 인식 모델을 내려받는 중입니다(1회). `stt.model_size = "base"` 로 낮추면 빨라집니다 |
| 인식이 부정확함 | `stt.model_size = "medium"` 으로 올리기. NVIDIA GPU가 있으면 `device = "cuda"`, `compute_type = "float16"` |
| 목소리가 어색함 | `tts.voice = "ko-KR-InJoonNeural"`(남성) 로 변경. 인터넷이 없으면 `engine = "sapi"` |
| 말이 너무 김 | `tts.speak_max_chars` 를 줄이기 |
| 마이크를 못 찾음 | `audio.input_device` 에 장치 이름 일부를 적기 (예: `"Yeti"`) |
| API 키 오류 | 명령 프롬프트를 새로 연 뒤 `echo %ANTHROPIC_API_KEY%` 로 확인 |

## 10. 구조

```
tools/jarvis/
  jarvis.bat / setup.bat      실행 · 설치
  config.example.toml         설정 원본 (config.toml 은 커밋 제외)
  jarvis/
    __main__.py               단축키 → 녹음 → 인식 → 응답 → 음성 루프
    agent.py                  Claude 대화 + 도구 호출 루프
    audio.py  stt.py  tts.py  마이크 / 음성인식 / 음성합성
    hotkey.py  approval.py    전역 단축키 / 실행 전 확인
    config.py  log.py
    skills/
      base.py                 도구 공통(경로 안전 검사 포함)
      pc.py                   PC 제어
      dev.py                  개발 비서
      notes.py                기억
      google_workspace.py     메일 · 일정
```

### 도구 추가하기

`skills/` 안의 모듈에 함수와 `Tool` 정의를 더하면 끝입니다.

```python
def battery_level(ctx: Ctx) -> str:
    return "배터리 82%"

TOOLS = [
    Tool("battery_level", "노트북 배터리 잔량을 확인한다.", obj({}), battery_level),
]
```

첫 인자는 항상 `ctx`, 반환은 문자열입니다.
실패는 `ToolError("이유")` 로 던지면 모델이 그 이유를 읽고 스스로 다른 방법을 찾습니다.

---

## 다음 단계 후보

- 상시 대기("자비스" 호출어) — 오인식 튜닝이 필요해 2차로 미뤄둠
- 트레이 아이콘 + 화면 오버레이 응답 창
- 화면 캡처를 보고 답하기 (비전)
- 정해진 시각에 알아서 도는 일과(브리핑·백업·빌드)
