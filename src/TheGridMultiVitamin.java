import java.io.PrintStream;
import java.util.Random;
import java.util.stream.IntStream;

public class TheGridMultiVitamin {

    public TheGridMultiVitamin(int width, int height) {
        this.width = width;
        this.height = height;
        heightPadded = height + 2;
        wordsPerRow = (width >> 4) + 2;
        rightPadding = (((wordsPerRow - 1) * 16 - width) & 15) << 2;
        lastWordCol = rightPadding != 0 ? wordsPerRow - 1 : wordsPerRow - 2;
        curGen = new long[wordsPerRow * heightPadded];
        nextGen = new long[wordsPerRow * heightPadded];
    }

    public int getHorizontalDimension () {
        return width;
    }
    public int getVerticalDimension () {
        return height;
    }

    public void setCell(int x, int y, boolean value) {
        y += 1;
        int arrayX = (x >> 4) + 1;
        int wordShift = (x & 15) << 2;
        curGen[arrayX + y * wordsPerRow] = (curGen[arrayX + y * wordsPerRow] & ~(1L << wordShift))
                | ((value ? 1L : 0L) << wordShift);
    }

    public boolean getCell(int x, int y) {
        y += 1;
        int arrayX = (x >> 4) + 1;
        int wordShift = (x & 15) << 2;
        return (curGen[arrayX + y * wordsPerRow] & (1L << wordShift)) != 0;
    }

    public int getCellI(int x, int y) { return getCell(x, y) ? 1 : 0; }

    public void nextGeneration() {
        if (useParallel)
            nextGenerationParallelImpl();
        else
            nextGenerationImpl();
    }

    public void simulateGenerations(int iterations) {
        if (useParallel)
            for (int i = 0; i < iterations; ++i)
                nextGenerationParallelImpl();
        else
            for (int i = 0; i < iterations; ++i)
                nextGenerationImpl();
    }

    public int countLivingNeighbors(int x, int y) {
        int left = x > 0 ? x - 1 : width - 1;
        int right = x + 1 < width ? x + 1 : 0;
        int bottom = y > 0 ? y - 1 : height - 1;
        int top = y + 1 < height ? y + 1 : 0;
        return getCellI(left, bottom) + getCellI(x, bottom) + getCellI(right, bottom)
                + getCellI(left, y) + getCellI(right, y)
                + getCellI(left, top) + getCellI(x, top) + getCellI(right, top);
    }

    public int numberOfSurvivors() {
        int count = 0;
        for (int i = 1; i + 1 < heightPadded; ++i)
        {
            for (int j = 1; j <= lastWordCol; ++j)
            {
                long cells = curGen[j + i * wordsPerRow];
                if (j >= lastWordCol)
                    cells = (cells << rightPadding) >> rightPadding;
                count += Long.bitCount(cells);
            }
        }
        return count;
    }

    public boolean isStatic() {
        nextGeneration();
        boolean result = true;
        for (int i = 1; i + 1 < heightPadded; ++i)
        {
            for (int j = 1; j <= lastWordCol; ++j)
            {
                long cells1 = curGen[j + i * wordsPerRow];
                long cells2 = nextGen[j + i * wordsPerRow];
                if (j >= lastWordCol)
                {
                    cells1 = (cells1 << rightPadding) >> rightPadding;
                    cells2 = (cells2 << rightPadding) >> rightPadding;
                }
                if (cells1 != cells2)
                {
                    result = false;
                    break;
                }
            }
        }

        swap();
        return result;
    }

    public void print(PrintStream p) {
        for (int row = 0; row < height; row++) {
            for (int col = 0; col < width; col++) {
                    p.print(getCell(col, row) ? 'X' : '.');
            }
            System.out.println();
        }
        System.out.println();
    }

    public void print() { print(System.out); }

    public TheGridMultiVitamin sequential() { useParallel = false; return this; }
    public TheGridMultiVitamin parallel() { useParallel = true; return this; }

    public static TheGridMultiVitamin BigBang(int width, int height, int seed, double density) {
        var g = new TheGridMultiVitamin(width, height);
        seed = seed < 0 ? new Random().nextInt(Integer.MAX_VALUE) : seed;
        for (int col = 0; col < width; col++) {
            for (int row = 0; row < height; row++) {
                g.setCell(col, row, Hashing.hash31Double(col, row, seed) < density);
            }
        }
        return g;
    }

    private void nextGenerationImpl() {
        {
            int notRightPaddingBoolInt = rightPadding != 0 ? 0 : 1;
            int leftPadding = (64 - rightPadding) & 63;
            int yIndex = wordsPerRow;
            for (int y = 1; y + 1 < heightPadded; ++y, yIndex += wordsPerRow)
            {

                curGen[yIndex] = curGen[lastWordCol + yIndex] << rightPadding;
                curGen[lastWordCol + notRightPaddingBoolInt + yIndex] =
                        ((curGen[lastWordCol + notRightPaddingBoolInt + yIndex] << rightPadding) >> rightPadding) * (1 - notRightPaddingBoolInt)
                                | curGen[1 + yIndex] << leftPadding;
            }

            System.arraycopy(curGen, (heightPadded - 2) * wordsPerRow, curGen, 0, wordsPerRow);
            System.arraycopy(curGen, wordsPerRow, curGen, (heightPadded - 1) * wordsPerRow, wordsPerRow);
        }

        for (int i = 1; i + 1 < heightPadded; ++i)
        {
            for (int j = 1; j <= lastWordCol; ++j)
            {
                long cellsDown = curGen[j + (i - 1) * wordsPerRow];
                long cells = curGen[j + i * wordsPerRow];
                long cellsUp = curGen[j + (i + 1) * wordsPerRow];
                long counts = (cellsDown << 4) + cellsDown + (cellsDown >> 4) + (cells << 4) + (cells >> 4) + (cellsUp << 4) + cellsUp + (cellsUp >> 4);
                counts += (curGen[j - 1 + (i - 1) * wordsPerRow] + curGen[j - 1 + i * wordsPerRow] + curGen[j - 1 + (i + 1) * wordsPerRow]) >> 60;
                if (j + 1 < wordsPerRow)
                    counts += (curGen[j + 1 + (i - 1) * wordsPerRow] + curGen[j + 1 + i * wordsPerRow] + curGen[j + 1 + (i + 1) * wordsPerRow]) << 60;
                long nextCells = counts | cells;
                nextGen[j + i * wordsPerRow] = nextCells & (nextCells >> 1) & (~((nextCells >> 2) | (nextCells >> 3))) & 0x1111111111111111L;
            }
        }
        swap();
    }

    private void nextGenerationParallelImpl() {
        {
            int notRightPaddingBoolInt = rightPadding != 0 ? 0 : 1;
            int leftPadding = (64 - rightPadding) & 63;
            int yIndex = wordsPerRow;
            for (int y = 1; y + 1 < heightPadded; ++y, yIndex += wordsPerRow)
            {

                curGen[yIndex] = curGen[lastWordCol + yIndex] << rightPadding;
                curGen[lastWordCol + notRightPaddingBoolInt + yIndex] =
                        ((curGen[lastWordCol + notRightPaddingBoolInt + yIndex] << rightPadding) >> rightPadding) * (1 - notRightPaddingBoolInt)
                                | curGen[1 + yIndex] << leftPadding;
            }

            System.arraycopy(curGen, (heightPadded - 2) * wordsPerRow, curGen, 0, wordsPerRow);
            System.arraycopy(curGen, wordsPerRow, curGen, (heightPadded - 1) * wordsPerRow, wordsPerRow);
        }

        IntStream.range(1, heightPadded - 1).parallel().forEach(i -> {
            for (int j = 1; j <= lastWordCol; ++j)
            {
                long cellsDown = curGen[j + (i - 1) * wordsPerRow];
                long cells = curGen[j + i * wordsPerRow];
                long cellsTop = curGen[j + (i + 1) * wordsPerRow];
                long counts = (cellsDown << 4) + cellsDown + (cellsDown >> 4) + (cells << 4) + (cells >> 4) + (cellsTop << 4) + cellsTop + (cellsTop >> 4);
                counts += (curGen[j - 1 + (i - 1) * wordsPerRow]
                        + curGen[j - 1 + i * wordsPerRow]
                        + curGen[j - 1 + (i + 1) * wordsPerRow]) >> 60;
                if (j + 1 < wordsPerRow)
                    counts += (curGen[j + 1 + (i - 1) * wordsPerRow] + curGen[j + 1 + i * wordsPerRow] + curGen[j + 1 + (i + 1) * wordsPerRow]) << 60;
                long nextCells = counts | cells;
                nextGen[j + i * wordsPerRow] = nextCells & (nextCells >> 1) & (~((nextCells >> 2) | (nextCells >> 3))) & 0x1111111111111111L;
            }
        });
        swap();
    }

    private void swap() {
        var tmp = curGen;
        curGen = nextGen;
        nextGen = tmp;
    }

    private final int width;
    private final int height;
    private final int heightPadded;
    private final int wordsPerRow;
    private final int rightPadding;
    private final int lastWordCol;
    private long[] curGen;
    private long[] nextGen;
    private boolean useParallel = false;

}
