@echo off
echo Running png_to_c...
python tools/png_to_c.py image/paint.png components/user_app/paint_icon.c paint_icon
if %errorlevel% neq 0 (
    echo Python failed with error %errorlevel%
    python --version
    where python
) else (
    echo Conversion successful!
)
