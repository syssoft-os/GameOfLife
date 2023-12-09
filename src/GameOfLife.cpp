#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>

class Stopwatch {
public:
    template <typename Function>
    static double measureTime(Function function) {
        auto startTime = std::chrono::high_resolution_clock::now();
        function();
        auto endTime = std::chrono::high_resolution_clock::now();

        return std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count() / 1000.0; // Convert microseconds to milliseconds
    }

    template <typename Function>
    static double measureParallelTime(Function parallelFunction) {
        auto startTime = std::chrono::high_resolution_clock::now();
        parallelFunction();
        auto endTime = std::chrono::high_resolution_clock::now();

        return std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count() / 1000.0; // Convert microseconds to milliseconds
    }
};

class TheGrid {
public:
    TheGrid(int horizontal_dimension, int vertical_dimension)
            : horizontal_dimension(horizontal_dimension),
              vertical_dimension(vertical_dimension),
              grid(horizontal_dimension, std::vector<bool>(vertical_dimension, false)) {}

    void setCell(int x, int y, bool value) {
        grid[x][y] = value;
    }

    bool getCell(int x, int y) const {
        return grid[x][y];
    }

    int countLivingNeighbors(int x, int y) const {
        int count = 0;

        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                if (i == 0 && j == 0) {
                    continue;
                }

                int dx = (x + i + horizontal_dimension) % horizontal_dimension;
                int dy = (y + j + vertical_dimension) % vertical_dimension;

                if (grid[dx][dy]) {
                    count++;
                }
            }
        }
        return count;
    }

    bool nextCellState(int x, int y) const {
        int livingNeighbors = countLivingNeighbors(x, y);
        bool cellState = getCell(x, y);

        if (cellState) {
            return (livingNeighbors == 2 || livingNeighbors == 3);
        } else {
            return (livingNeighbors == 3);
        }
    }

    void simulateGenerations(int iterations) {
        for (int iter = 0; iter < iterations; ++iter) {
            std::vector<std::vector<bool>> nextGrid = grid;

            for (int col = 0; col < horizontal_dimension; ++col) {
                for (int row = 0; row < vertical_dimension; ++row) {
                    nextGrid[col][row] = nextCellState(col, row);
                }
            }

            grid = nextGrid;
        }
    }

    void simulateGenerationsParallel(int iterations, int numThreads) {
        for (int iter = 0; iter < iterations; ++iter) {
            std::vector<std::thread> threads;
            std::vector<std::vector<bool>> nextGrid = grid;

            for (int t = 0; t < numThreads; ++t) {
                threads.emplace_back([&, t]() {
                    for (int col = t; col < horizontal_dimension; col += numThreads) {
                        for (int row = 0; row < vertical_dimension; ++row) {
                            nextGrid[col][row] = nextCellState(col, row);
                        }
                    }
                });
            }

            for (auto& thread : threads) {
                thread.join();
            }

            grid = nextGrid;
        }
    }

    static TheGrid BingBang(int horizontal_dimension, int vertical_dimension, int seed, double density) {
        TheGrid g(horizontal_dimension, vertical_dimension);
        std::default_random_engine rg(seed < 0 ? std::random_device{}() : seed);
        std::uniform_real_distribution<double> distribution(0.0, 1.0);

        for (int col = 0; col < horizontal_dimension; col++) {
            for (int row = 0; row < vertical_dimension; row++) {
                g.setCell(col, row, distribution(rg) < density);
            }
        }
        return g;
    }

    int getHorizontalDimension() const {
        return horizontal_dimension;
    }

    int getVerticalDimension() const {
        return vertical_dimension;
    }

    int numberOfSurvivors() const {
        int count = 0;
        for (int col = 0; col < horizontal_dimension; col++) {
            for (int row = 0; row < vertical_dimension; row++) {
                if (grid[col][row]) {
                    count++;
                }
            }
        }
        return count;
    }

    bool isStatic() const {
        TheGrid next = *this;

        for (int col = 0; col < horizontal_dimension; col++) {
            for (int row = 0; row < vertical_dimension; row++) {
                if (grid[col][row] != next.grid[col][row]) {
                    return false;
                }
            }
        }

        return true;
    }

private:
    int horizontal_dimension;
    int vertical_dimension;
    std::vector<std::vector<bool>> grid;
};

int main() {
    TheGrid grid(1000, 1000);

    double nonParallelTime = Stopwatch::measureTime([&]() {
        grid.simulateGenerations(100);
        std::cout << "Survivors: " << grid.numberOfSurvivors() << std::endl;
        std::cout << (grid.isStatic() ? "The grid is static" : "The grid is still living and not static") << std::endl;
    });

    std::cout << "Non-parallel execution time: " << nonParallelTime << " ms" << std::endl;

    double parallelTime = Stopwatch::measureParallelTime([&]() {
        grid.simulateGenerationsParallel(100, std::thread::hardware_concurrency());
        std::cout << "Survivors: " << grid.numberOfSurvivors() << std::endl;
        std::cout << (grid.isStatic() ? "The grid is static" : "The grid is still living and not static") << std::endl;
    });

    std::cout << "Parallel execution time: " << parallelTime << " ms" << std::endl;

    return 0;
}
