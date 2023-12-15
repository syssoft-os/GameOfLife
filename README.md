# GameOfLive
Conway's "Game of Life" as an exercise in program parallelization.

## C++ build
- For **POSIX** simply use command `gcc GameOfLifeCpp.cpp -o GameOfLifeCpp -std=c++17 -march=native -lstdc++ -pthread -O3`.
- For **Windows** use the included Visual Studio Solution (VS2022 or later).