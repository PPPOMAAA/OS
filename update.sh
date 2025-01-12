#!/bin/bash
if [ -d .git ]; then
    git pull
else
    echo "Клонирование репозитория..."
    rm -rf * .[!.]*
    git clone https://github.com/PPPOMAAA/OS .
fi
clear
echo "Проект успешно обновлён."
sleep 3