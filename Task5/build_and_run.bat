chcp 65001 >nul

@echo off
if not exist "build" mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
if exist prog.exe (
    echo Запуск программы...
    cls
    prog.exe
) else (
    echo Ошибка: prog.exe не найден.
)
cmd /k