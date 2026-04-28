class EdgeCases {
    public static void main(String[] args) {
        double d;
        int a;
        boolean b;
        
        d = 1e-400; // Deve dar "Number out of bounds"
        a = 1 ^ 2;
        b = true ^ false; // Deve dar erro de operador ^ em boolean
        System.out.print(args); // Deve dar erro de impressão
    }
}