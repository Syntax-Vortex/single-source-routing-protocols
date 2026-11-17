@echo off
setlocal
cd /d "%~dp0"
if not exist build mkdir build
g++ -std=c++11 -static -O2 -Wall -Wextra -Wpedantic -Iinclude src/main.cpp src/input.cpp src/etx.cpp src/case1.cpp src/case2.cpp src/case3.cpp src/case4.cpp -o build/etx_routing.exe
if errorlevel 1 exit /b 1
echo Built build\etx_routing.exe
g++ -std=c++11 -static -O2 -Wall -Wextra -Wpedantic -Iinclude src/simulation.cpp src/ran2.cpp src/input.cpp src/etx.cpp src/case2.cpp src/case3.cpp src/case4.cpp -o build/simulation.exe
if errorlevel 1 exit /b 1
echo Built build\simulation.exe
