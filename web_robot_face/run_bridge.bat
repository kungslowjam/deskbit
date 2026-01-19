@echo off
setlocal

REM Try standard python first
python --version >nul 2>&1
if %errorlevel% equ 0 (
    echo Found global Python...
    python bridge_server.py
    goto :end
)

REM Try finding ESP-IDF Python
echo Global Python not found. Searching for ESP-IDF Python...

set "ESP_TOOLS_DIR=%USERPROFILE%\.espressif\tools\idf-python"
if exist "%ESP_TOOLS_DIR%" (
    for /d %%D in ("%ESP_TOOLS_DIR%\*") do (
        if exist "%%D\python.exe" (
            echo Found Python at: %%D\python.exe
            "%%D\python.exe" bridge_server.py
            goto :end
        )
    )
)

echo.
echo ❌ Error: Could not find Python!
echo Please make sure you have installed ESP-IDF or Python.
echo.
pause
exit /b 1

:end
pause
