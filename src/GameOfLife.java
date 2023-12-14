
/*=============================================================================
            This branch shall henceforth be known as BitteEinBit
=============================================================================*/

public class GameOfLife {
    private static int width = 1000;
    private static int height = 1000;
    private static int seed = 10;
    private static double density = 0.5;
    private static int generations = 1000;

    public static void main(String[] args) {
        Runnable r = () -> {
            var g = TheGrid.BigBang(width, height, seed, density);
            g.simulateGenerations(generations);
            System.out.println("Survivors: " + g.numberOfSurvivors());
            if (g.isStatic()) {
                System.out.println("The grid is static");
            } else {
                System.out.println("The grid is still living and not static");
            }
        };
        Runnable r2 = () -> {
            var g = TheGridMultiVitamin.BigBang(width, height, seed, density);
            g.parallel().simulateGenerations(generations);
            System.out.println("Survivors: " + g.numberOfSurvivors());
            if (g.isStatic()) {
                System.out.println("The grid is static");
            } else {
                System.out.println("The grid is still living and not static");
            }
        };

        double executionTime = Stopwatch.startstop(r);
        System.out.println("Execution time: " + executionTime + " ms");
        double executionTime2 = Stopwatch.startstop(r2);
        System.out.println("Execution time: " + executionTime2 + " ms");

        System.out.println("Speedup: " + (executionTime / executionTime2) + "x");
    }
}