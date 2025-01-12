@echo off
if not exist "OS" mkdir OS
cd OS
if exist .git (
    git pull
) else (
    git clone https://github.com/PPPOMAAA/OS .
)
cd ..