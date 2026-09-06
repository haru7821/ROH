@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"

if not exist ".venv\Scripts\python.exe" (
    echo 가상환경이 없습니다. 먼저 setup.bat 을 실행하세요.
    pause
    exit /b 1
)

.venv\Scripts\python.exe -m jarvis %*
if %errorlevel% neq 0 pause
