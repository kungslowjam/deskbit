@echo off
REM Quick Export - Robot Face Studio to ESP32 Project
REM Usage: Drag and drop JSON file onto this batch file

echo ========================================
echo  Robot Face Studio - Quick Export
echo ========================================
echo.

if "%~1"=="" (
    echo Error: No file specified!
    echo.
    echo Usage: Drag and drop a JSON file onto this batch file
    echo.
    pause
    exit /b 1
)

echo Processing: %~nx1
echo.

REM Find Python (try multiple locations)
set PYTHON_CMD=

REM 1. Try standard python command
where python >nul 2>&1
if %errorlevel% equ 0 (
    set PYTHON_CMD=python
    goto :found_python
)

REM 2. Try python3
where python3 >nul 2>&1
if %errorlevel% equ 0 (
    set PYTHON_CMD=python3
    goto :found_python
)

REM 3. Try ESP-IDF Python (common location)
if exist "%USERPROFILE%\.espressif\python_env\idf5.3_py3.11_env\Scripts\python.exe" (
    set PYTHON_CMD="%USERPROFILE%\.espressif\python_env\idf5.3_py3.11_env\Scripts\python.exe"
    goto :found_python
)

REM 4. Try ESP-IDF Python (alternative version)
if exist "%USERPROFILE%\.espressif\python_env\idf5.2_py3.11_env\Scripts\python.exe" (
    set PYTHON_CMD="%USERPROFILE%\.espressif\python_env\idf5.2_py3.11_env\Scripts\python.exe"
    goto :found_python
)

REM 5. Try to find any ESP-IDF Python
for /d %%i in ("%USERPROFILE%\.espressif\python_env\idf*_py*_env") do (
    if exist "%%i\Scripts\python.exe" (
        set PYTHON_CMD="%%i\Scripts\python.exe"
        goto :found_python
    )
)

REM 6. Try common Python installation paths
if exist "C:\Python311\python.exe" (
    set PYTHON_CMD="C:\Python311\python.exe"
    goto :found_python
)

if exist "C:\Python310\python.exe" (
    set PYTHON_CMD="C:\Python310\python.exe"
    goto :found_python
)

if exist "C:\Python39\python.exe" (
    set PYTHON_CMD="C:\Python39\python.exe"
    goto :found_python
)

REM Python not found
echo.
echo ========================================
echo  ERROR: Python not found!
echo ========================================
echo.
echo Please install Python or make sure ESP-IDF is installed.
echo.
echo Download Python: https://www.python.org/downloads/
echo.
pause
exit /b 1

:found_python
echo Using Python: %PYTHON_CMD%
echo.

%PYTHON_CMD% export_to_project.py "%~1"

echo.
pause
