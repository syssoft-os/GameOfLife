import java.util.Random;
import java.util.concurrent.Callable;

class Stopwatch {

    public static double startstop ( Runnable function ) {
        long startTime = System.nanoTime();
        try {
            function.run();
        } catch (Exception e) {
            System.out.println("Exception while calling function in startstop(): " + e);
        }
        long endTime = System.nanoTime();
        return (endTime - startTime) / 1e6; // Convert nanoseconds to milliseconds
    }

}
