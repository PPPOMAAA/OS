chcp 65001 >nul

@echo off
if not exist "build" mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
if exist hello_world.exe (
    echo Запуск программы...
    cls
    hello_world.exe
) else (
    echo Ошибка: hello_world.exe не найден.
)
cmd /k