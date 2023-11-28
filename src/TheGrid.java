import java.io.PrintStream;
import java.util.Random;

public class TheGrid {

    public TheGrid ( int horizontal_dimension, int vertical_dimension ) {
        this.horizontal_dimension = horizontal_dimension;
        this.vertical_dimension = vertical_dimension;
        this.grid = new boolean[horizontal_dimension][vertical_dimension];
    }

    public void setCell ( int x, int y, boolean value ) {
        this.grid[x][y] = value;
    }

    public boolean getCell ( int x, int y ) {
        return this.grid[x][y];
    }

    public int countLivingNeighbors(int x, int y) {
        int count = 0;

        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                if (i == 0 && j == 0) {
                    // Skip the cell itself
                    continue;
                }

                int dx = x+i < 0 ? horizontal_dimension - 1 : (x+i) % horizontal_dimension;
                int dy = y+j < 0 ? vertical_dimension - 1 : (y+j) % vertical_dimension;

                if (grid[dx][dy]) {
                    count++;
                }
            }
        }
        return count;
    }

    public boolean nextCellState(int x, int y) {
        int livingNeighbors = countLivingNeighbors(x, y);
        boolean cellState = getCell(x, y);

        if (cellState) {
            if (livingNeighbors < 2) {
                return false;
            } else if (livingNeighbors > 3) {
                return false;
            } else {
                return true;
            }
        } else {
            if (livingNeighbors == 3) {
                return true;
            } else {
                return false;
            }
        }
    }

    public int getHorizontalDimension () {
        return horizontal_dimension;
    }

    public int getVerticalDimension () {
        return vertical_dimension;
    }

    public void print (PrintStream p) {
        for (int row = 0; row < vertical_dimension; row++) {
            for (int col = 0; col < horizontal_dimension; col++) {
                if (grid[col][row]) {
                    p.print("X");
                } else {
                    p.print(".");
                }
            }
            System.out.println();
        }
        System.out.println();
    }

    public void print () {
        print(System.out);
    }

    public TheGrid nextGeneration() {
        TheGrid next = new TheGrid(horizontal_dimension, vertical_dimension);
        for (int col = 0; col < horizontal_dimension; col++) {
            for (int row = 0; row < vertical_dimension; row++) {
                next.setCell(col, row, nextCellState(col, row));
            }
        }
        return next;
    }

    public void simulateGenerations(int iterations) {
        for (int i = 0; i < iterations; i++) {
            TheGrid next = nextGeneration();
            this.grid = next.grid;
        }
    }

    public int numberOfSurvivors () {
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

    public boolean isStatic () {
        TheGrid next = nextGeneration();
        for (int col = 0; col < horizontal_dimension; col++) {
            for (int row = 0; row < vertical_dimension; row++) {
                if (grid[col][row] != next.grid[col][row]) {
                    return false;
                }
            }
        }
        return true;
    }

    public static TheGrid BingBang(int horizontal_dimension, int vertical_dimension, int seed, double density ) {
        TheGrid g = new TheGrid(horizontal_dimension, vertical_dimension);
        Random rg = seed < 0 ? new Random() : new Random(seed);
        for (int col = 0; col < horizontal_dimension; col++) {
            for (int row = 0; row < vertical_dimension; row++) {
                g.setCell(col, row, rg.nextDouble() < density);
            }
        }
        return g;
    }

    private int horizontal_dimension;
    private int vertical_dimension;
    private boolean[][] grid;

}
