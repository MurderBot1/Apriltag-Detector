@echo off
REM AprilTag Detector System Launch Script for Windows
REM This script starts all components of the AprilTag detection system

setlocal enabledelayedexpansion

REM Get script directory
set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%~dp0.."

REM Create logs directory with timestamp
for /f "tokens=1-6 delims=/:. " %%a in ('echo %date% %time%') do (
    set "YY=%%a"
    set "MM=%%b"
    set "DD=%%c"
    set "HH=%%d"
    set "MI=%%e"
    set "SS=%%f"
)
set "LOG_DIR=%PROJECT_ROOT%\logs\%YY%-%MM%-%DD%_%HH%-%MI%-%SS%"
mkdir "%LOG_DIR%\system" 2>nul

REM Configuration directory
set "CONFIG_DIR=%PROJECT_ROOT%\config"

REM Binary directory
set "BIN_DIR=%PROJECT_ROOT%\bin"

REM Function to check if a process is running
:is_process_running
set "process_name=%~1"
tasklist /FI "IMAGENAME eq %process_name%" /FO CSV 2>nul | find /I "%process_name%" >nul
if %errorlevel% equ 0 (
    exit /b 0
) else (
    exit /b 1
)

REM Function to start a process
:start_process
set "name=%~1"
set "cmd=%~2"

call :is_process_running "%name%"
if %errorlevel% equ 0 (
    echo [%name%] Already running
    exit /b 0
)

echo [%name%] Starting...
start "" /B cmd /c "%cmd% > %LOG_DIR%\system\%name%.log 2>&1"

REM Wait a bit to check if it started
timeout /t 1 >nul

call :is_process_running "%name%"
if %errorlevel% equ 0 (
    echo [%name%] Started successfully
    exit /b 0
) else (
    echo [%name%] Failed to start
    exit /b 1
)

REM Function to stop a process
:stop_process
set "name=%~1"

call :is_process_running "%name%"
if %errorlevel% neq 0 (
    echo [%name%] Not running
    exit /b 0
)

echo [%name%] Stopping...
taskkill /F /IM "%name%.exe" >nul 2>&1

REM Wait for it to stop
set "count=0"
:wait_stop
call :is_process_running "%name%"
if %errorlevel% equ 0 (
    timeout /t 1 >nul
    set /a "count+=1"
    if !%count! geq 10 (
        goto :wait_stop
    )
)

call :is_process_running "%name%"
if %errorlevel% neq 0 (
    echo [%name%] Stopped
    exit /b 0
) else (
    echo [%name%] Failed to stop
    exit /b 1
)

REM Function to show status
:show_status
set "processes=launcher web_ui pose_finder"

REM Add camera detectors
if exist "%CONFIG_DIR%\cameras" (
    for %%f in ("%CONFIG_DIR%\cameras\camera_*.json") do (
        set "config=%%f"
        set "camera_id=%%~nf"
        set "camera_id=!camera_id:camera_=!"
        set "processes=!processes! camera_detector_!camera_id!"
    )
)

echo === AprilTag Detector System Status === 
echo.

for %%p in (%processes%) do (
    call :is_process_running "%%p"
    if %errorlevel% equ 0 (
        for /f "tokens=2" %%i in ('tasklist /FI "IMAGENAME eq %%p.exe" /FO CSV /NH') do (
            set "pid=%%i"
            set "pid=!pid:"=!"
            echo   %%p: Running (PID: !pid!)
        )
    ) else (
        echo   %%p: Stopped
    )
)

echo.
echo === Logs ===
echo   Log directory: %LOG_DIR%
echo.

goto :eof

REM Main command handling
if "%~1"=="" goto :start
if "%~1"=="start" goto :start
if "%~1"=="stop" goto :stop
if "%~1"=="restart" goto :restart
if "%~1"=="status" goto :status
if "%~1"=="logs" goto :logs
if "%~1"=="build" goto :build

:start
echo === Starting AprilTag Detector System === 
echo.

REM Check if binaries exist
if not exist "%BIN_DIR%\launcher.exe" (
    echo Building project...
    cd "%PROJECT_ROOT%"
    if not exist "build" mkdir build
    cd build
    cmake .. -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release
    cmake --build . --config Release --parallel %NUMBER_OF_PROCESSORS%
    
    if not exist "%BIN_DIR%\launcher.exe" (
        echo Build failed!
        exit /b 1
    )
)

REM Start launcher (which starts everything else)
call :start_process "launcher" "%BIN_DIR%\launcher.exe"

echo.
echo System started. Access web UI at http://localhost:8080
echo Logs are in: %LOG_DIR%

goto :eof

:stop
echo === Stopping AprilTag Detector System === 
echo.

REM Stop all processes
call :stop_process "camera_detector_"
call :stop_process "pose_finder"
call :stop_process "web_ui"
call :stop_process "launcher"

echo.
echo All processes stopped

goto :eof

:restart
echo === Restarting AprilTag Detector System === 
echo.

call %~0 stop
timeout /t 2 >nul
call %~0 start

goto :eof

:status
call :show_status

goto :eof

:logs
if exist "%LOG_DIR%" (
    type "%LOG_DIR%\system\*.log"
) else (
    echo No log directory found: %LOG_DIR%
)

goto :eof

:build
echo === Building AprilTag Detector System === 
echo.

cd "%PROJECT_ROOT%"
if not exist "build" mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --parallel %NUMBER_OF_PROCESSORS%

if exist "%BIN_DIR%\launcher.exe" (
    echo.
    echo Build successful!
) else (
    echo.
    echo Build failed!
    exit /b 1
)

goto :eof

:usage
echo Usage: %~nx0 {start|stop|restart|status|logs|build}
echo.
echo Commands:
echo   start    - Start all processes
echo   stop     - Stop all processes
echo   restart  - Restart all processes
echo   status   - Show process status
echo   logs     - Show logs
echo   build    - Build the project
exit /b 1
