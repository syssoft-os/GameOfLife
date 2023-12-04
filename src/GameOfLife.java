public class GameOfLife {
    public static void main(String[] args) {
        Runnable r = new Runnable() {
            @Override
            public void run() {
                TheGrid g = TheGrid.BingBang(10000, 10000, -1, 0.5);
                g.simulateGenerations(1000);
                System.out.println("Survivors: " + g.numberOfSurvivors());
                if (g.isStatic()) {
                    System.out.println("The grid is static");
                } else {
                    System.out.println("The grid is still living and not static");
                }
            }
        };

        double executionTime = Stopwatch.startstop(r);
        System.out.println("Execution time: " + executionTime + " ms");
    }
}