chcp 65001 >nul

@echo off
if not exist "build" mkdir build
cd build

cmake .. -G "MinGW Makefiles"
cmake --build .

if exist prog.exe (
    echo Запуск основной программы...
    cls
    prog.exe
) else (
    echo Ошибка: prog.exe не найден.
)

if exist emulated_device.exe (
    echo Запуск emulated_device...
    cls
    emulated_device.exe
) else (
    echo Ошибка: emulated_device.exe не найден.
)

cmd /k