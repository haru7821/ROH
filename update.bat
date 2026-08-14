@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"

REM 엔진 설치 경로 - 다르면 이 줄만 수정
set "UE_PATH=D:\ROH\UE_5.8"

echo ============================================
echo  ROH 업데이트 스크립트: Pull - 빌드 - 실행
echo ============================================
echo.

echo [1/4] 최신 코드 받는 중...
set "GIT_EXE=git"
where git >nul 2>nul
if not errorlevel 1 goto do_pull
for /d %%i in ("%LOCALAPPDATA%\GitHubDesktop\app-*") do set "GIT_EXE=%%i\resources\app\git\cmd\git.exe"

:do_pull
"%GIT_EXE%" pull
if errorlevel 1 goto pull_failed

echo.
echo [2/4] 이전 빌드 정리 중...
if exist Binaries rmdir /s /q Binaries
if exist Intermediate\Build rmdir /s /q Intermediate\Build

echo [3/4] 컴파일 중... 수 분 소요, 창을 닫지 마세요
call "%UE_PATH%\Engine\Build\BatchFiles\Build.bat" ROHEditor Win64 Development -project="%~dp0ROH.uproject" -WaitMutex
if errorlevel 1 goto build_failed

echo.
echo [4/4] 빌드 성공. 에디터 실행 중...
start "" "%UE_PATH%\Engine\Binaries\Win64\UnrealEditor.exe" "%~dp0ROH.uproject"
exit /b 0

:pull_failed
echo.
echo ===== Pull 실패 =====
echo GitHub Desktop에서 Pull origin을 먼저 실행한 뒤 다시 켜세요.
echo 충돌 메시지가 보이면 캡처해서 Claude에게 보내주세요.
pause
exit /b 1

:build_failed
echo.
echo ===== 빌드 실패 =====
echo 위 오류 내용을 복사해서 Claude에게 보내주세요.
pause
exit /b 1
