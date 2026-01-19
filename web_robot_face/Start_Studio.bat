@echo off
setlocal
title Robot Face Studio Server

REM 1. Find Python
python --version >nul 2>&1
if %errorlevel% equ 0 (
    set PYTHON_CMD=python
    goto :found_python
)

set "ESP_TOOLS_DIR=%USERPROFILE%\.espressif\tools\idf-python"
if exist "%ESP_TOOLS_DIR%" (
    for /d %%D in ("%ESP_TOOLS_DIR%\*") do (
        if exist "%%D\python.exe" (
            set "PYTHON_CMD=%%D\python.exe"
            goto :found_python
        )
    )
)

echo ❌ Error: Python not found!
pause
exit /b 1

:found_python
echo 🚀 Starting Robot Face Studio...
echo 🌍 Server: http://localhost:8000

REM 2. Open Browser (Wait 2s to ensure server acts first)
start "" "http://localhost:8000"

REM 3. Run Server (This blocks until closed)
"%PYTHON_CMD%" bridge_server.py
