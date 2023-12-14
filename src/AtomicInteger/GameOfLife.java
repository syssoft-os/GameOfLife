package AtomicInteger;

import java.util.LinkedList;

public class GameOfLife {
    public static void main(String[] args) {
        LinkedList<Thread> threads = new LinkedList<Thread>();
        Runnable r = new Runnable() {
            @Override
            public void run() {
                TheGrid g = TheGrid.BingBang(1000, 1000, -1, 0.5);
                int numThreads = Runtime.getRuntime().availableProcessors();
                for(int i = 0; i < numThreads; i++ ){
                    Thread t = new Thread(new Runnable() {
                        @Override
                        public void run() {
                            g.simulateGenerations(1000 / numThreads);
                        }
                    });
                    t.start();
                    threads.add(t);
                }
                for(Thread thread : threads){
                    try {
                        thread.join();
                    } catch (InterruptedException e) {
                        throw new RuntimeException(e);
                    }
                }
                System.out.println("Survivors: " + g.numberOfSurvivors());
                if (g.isStatic()) {
                    System.out.println("The grid is static");
                } else {
                    System.out.println("The grid is still living and not static");
                }
            }
        };

        double executionTime = Stopwatch.startstop(r);
        System.out.println("Execution time parallel: " + executionTime + " ms");
    }
}