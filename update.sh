#!/bin/bash
mkdir -p OS
cd OS
if [ -d .git ]; then
    git pull
else
    git clone https://github.com/PPPOMAAA/OS .
fi
cd ..