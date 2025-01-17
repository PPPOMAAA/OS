#!/bin/bash
mkdir -p build
cd build
cmake ..
cmake --build .
if [ -f ./prog ]; then
    echo "Запуск программы..."
    clear
    ./prog
else
    echo "Ошибка: prog не найден."
fi