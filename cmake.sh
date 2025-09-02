#!/bin/bash

cd /home/davidson/Documents/Studiumdokumente/lab/project
rm -rf build
mkdir build
cd build
cmake ..
make -j4
