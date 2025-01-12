#!/bin/bash
mkdir -p build
cd build
cmake ..
cmake --build .
if [ -f ./hello_world ]; then
    echo "Запуск программы..."
    ./hello_world
else
    echo "Ошибка: hello_world не найден."
fi