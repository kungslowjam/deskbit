@echo off
echo Installing Pillow...
pip install Pillow
echo Converting GIF to C...
python tools/gif_to_c.py gif_files/pomo.gif components/user_app/pomo_anim.c pomo_anim
echo Done! Please rebuild your project.
pause
