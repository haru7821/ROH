@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"

echo === Jarvis 설치 ===

where py >nul 2>nul
if %errorlevel%==0 (set PY=py -3) else (set PY=python)

if not exist ".venv" (
    echo [1/4] 가상환경 생성...
    %PY% -m venv .venv || goto :fail
) else (
    echo [1/4] 가상환경 확인됨.
)

echo [2/4] pip 최신화...
call .venv\Scripts\python.exe -m pip install --upgrade pip || goto :fail

echo [3/4] 패키지 설치... (처음에는 몇 분 걸립니다)
call .venv\Scripts\python.exe -m pip install -r requirements.txt || goto :fail

if not exist "config.toml" (
    echo [4/4] config.toml 생성...
    copy /y config.example.toml config.toml >nul
    echo.
    echo  * config.toml 을 열어 dev.repo_path 와 앱 별칭을 본인 환경에 맞게 고치세요.
) else (
    echo [4/4] config.toml 이미 있음 - 건드리지 않습니다.
)

echo.
echo 설치 완료. 환경변수 ANTHROPIC_API_KEY 를 설정한 뒤 jarvis.bat 을 실행하세요.
echo   setx ANTHROPIC_API_KEY "sk-ant-..."
pause
exit /b 0

:fail
echo.
echo [X] 설치 중 오류가 발생했습니다. 위 메시지를 확인하세요.
pause
exit /b 1
