package DivideGrid;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.*;

/**
 * Dividing the grid in several parts, calculating the new parts of the grid and wait until all
 * parts are calculated. Then starting on with a new generation.
 *
 * This approach needs < 1/3 of the sequential approach.
 */

public class GameOfLife {
    public static void main(String[] args) throws InterruptedException, ExecutionException {
        long startTime = System.nanoTime();

        int numThreads = Runtime.getRuntime().availableProcessors();
        ExecutorService executor = Executors.newFixedThreadPool(numThreads);

        TheGrid g = TheGrid.BingBang(1000, 1000, -1, 0.5);
        int partSize = g.getHorizontalDimension() / numThreads;

        for (int generation = 0; generation < 1000; generation++) {
            List<Future<TheGrid>> futures = new ArrayList<>();

            for (int i = 0; i < numThreads; i++) {
                final int start = i * partSize;
                final int end = (i + 1) * partSize;

                TheGrid finalG = g;
                futures.add(executor.submit(() -> finalG.nextGeneration(start, end)));
            }

            TheGrid next = new TheGrid(g.getHorizontalDimension(), g.getVerticalDimension());
            for (int i = 0; i < numThreads; i++) {
                final int start = i * partSize;
                final int end = (i + 1) * partSize;

                TheGrid part = futures.get(i).get();
                for (int x = start; x < end; x++) {
                    for (int y = 0; y < g.getVerticalDimension(); y++) {
                        next.setCell(x, y, part.getCell(x - start, y));
                    }
                }
            }

            g = next;
        }

        executor.shutdown();

        System.out.println("Survivors: " + g.numberOfSurvivors());
        if (g.isStatic()) {
            System.out.println("The grid is static");
        } else {
            System.out.println("The grid is still living and not static");
        }

        long endTime = System.nanoTime();
        double executionTime = (endTime - startTime) / 1e6; // convert to milliseconds
        System.out.println("Execution time: " + executionTime + " ms");
    }
}