#!/bin/bash

mkdir -p build
cd build

cmake ..
cmake --build .

if [ -f ./prog ]; then
    echo "Запуск основной программы..."
    clear
    ./prog
else
    echo "Ошибка: prog не найден."
fi

if [ -f ./emulated_device ]; then
    echo "Запуск emulated_device..."
    clear
    ./emulated_device
else
    echo "Ошибка: emulated_device не найден."
fi
