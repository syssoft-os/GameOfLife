public class Hashing {
    public static double hash31Double(int x, int y, int z) {
        int x2 = ((x >>> 8) ^ y) * 1103515245;
        int y2 = ((y >>> 8) ^ z) * 1103515245;
        int z2 = ((z >>> 8) ^ x) * 1103515245;
        x = ((x2 >>> 8) ^ y2) * 1103515245;
        y = ((y2 >>> 8) ^ z2) * 1103515245;
        x2 = ((x >>> 8) ^ y) * 1103515245;
        return (double)(x2 & Integer.MAX_VALUE) / Integer.MAX_VALUE;
    }
}
