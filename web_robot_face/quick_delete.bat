@echo off
setlocal

REM Try standard python first
python --version >nul 2>&1
if %errorlevel% equ 0 (
    python delete_anim.py
    pause
    exit /b 0
)

REM Try finding ESP-IDF Python
echo Global Python not found. Searching for ESP-IDF Python...
set "ESP_TOOLS_DIR=%USERPROFILE%\.espressif\tools\idf-python"
if exist "%ESP_TOOLS_DIR%" (
    for /d %%D in ("%ESP_TOOLS_DIR%\*") do (
        if exist "%%D\python.exe" (
            "%%D\python.exe" delete_anim.py
            pause
            exit /b 0
        )
    )
)

echo ❌ Error: Could not find Python!
pause
