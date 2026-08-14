@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"

REM 엔진 설치 경로 (다르면 이 줄만 수정)
set UE_PATH=D:\ROH\UE_5.8

echo ============================================
echo  ROH 업데이트 빌드 스크립트
echo  (실행 전: GitHub Desktop에서 Pull 완료할 것)
echo ============================================
echo.

echo [1/3] 이전 빌드 정리 중...
if exist Binaries rmdir /s /q Binaries
if exist Intermediate\Build rmdir /s /q Intermediate\Build

echo [2/3] 컴파일 중... (수 분 소요, 창을 닫지 마세요)
call "%UE_PATH%\Engine\Build\BatchFiles\Build.bat" ROHEditor Win64 Development -project="%~dp0ROH.uproject" -WaitMutex
if errorlevel 1 (
  echo.
  echo ############################################
  echo  빌드 실패! 위 오류 내용을 Claude에게 보내주세요.
  echo ############################################
  pause
  exit /b 1
)

echo [3/3] 빌드 성공! 에디터 실행 중...
start "" "%UE_PATH%\Engine\Binaries\Win64\UnrealEditor.exe" "%~dp0ROH.uproject"
exit /b 0
