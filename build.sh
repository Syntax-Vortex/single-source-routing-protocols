#!/bin/sh
# macOS / Linux equivalent of build.bat.
set -e
cd "$(dirname "$0")"
mkdir -p build
CXX="${CXX:-c++}"
FLAGS="-std=c++11 -O2 -Wall -Wextra -Wpedantic -Iinclude"
$CXX $FLAGS src/main.cpp src/input.cpp src/etx.cpp src/case1.cpp src/case2.cpp src/case3.cpp src/case4.cpp -o build/etx_routing
echo "Built build/etx_routing"
$CXX $FLAGS src/simulation.cpp src/ran2.cpp src/input.cpp src/etx.cpp src/case2.cpp src/case3.cpp src/case4.cpp -o build/simulation
echo "Built build/simulation"
