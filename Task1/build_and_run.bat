chcp 65001 >nul

@echo off
if not exist "build" mkdir build
cd build
cmake ..
cmake --build .
if exist hello_world.exe (
    echo Запуск программы...
    hello_world.exe
) else (
    echo Ошибка: hello_world.exe не найден.
)