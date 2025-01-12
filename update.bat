chcp 65001 >nul

@echo off
if exist .git (
    git pull
) else (
    echo Клонирование репозитория...
    rd /s /q . >nul 2>&1
    git clone https://github.com/PPPOMAAA/OS .
)
cls
echo Проект успешно обновлён.
timeout /t 3