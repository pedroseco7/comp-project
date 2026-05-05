class StressTest {
    public static void main(String[] args) {
        int i;
        int count;
        i = 0;
        count = 0;

        System.out.print("A iniciar o teste de stress...\n");
        System.out.print("\"");

        // Loop que itera 10 milhões de vezes
        while (i < 10000000) {
            // Duas subcondições com && (Testa o Short-Circuit)
            if (i % 2 == 0 && i % 5 == 0) {
                count = count + 1;
            }
            i = i + 1;
        }

        System.out.print("Resultado: ");
        System.out.print(count); // Deve imprimir 1000000
        System.out.print("\n");
    }
}