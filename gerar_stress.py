# gerar_stress.py
import sys

def gerar_stress_fields():
    # Testa o Limite de Memória (MLE) - 50.000 variáveis declaradas
    with open("stress_fields.java", "w") as f:
        f.write("class StressFields {\n")
        f.write("    public static int " + ", ".join(f"f{i}" for i in range(50000)) + ";\n")
        f.write("}\n")

def gerar_stress_methods():
    # Testa o Limite de Tempo (TLE) - Hash Collisions com 10.000 métodos iguais
    with open("stress_methods.java", "w") as f:
        f.write("class StressMethods {\n")
        for i in range(10000):
            # Assinaturas ligeiramente diferentes para forçar o compilador a procurar
            params = ", ".join(["int a"] * (i % 50))
            f.write(f"    public static void m{i}({params}) {{}}\n")
        f.write("}\n")

def gerar_edge_cases_expr():
    # Testa o "Wrong Answer" nas Expressions and Statements
    with open("edge_cases_expr.java", "w") as f:
        f.write("""class EdgeCases {
    public static void main(String[] args) {
        double d = 1e-400; // Underflow absoluto
        int a = 1 ^ 2;     // XOR só funciona com ints e booleans
        boolean b = true ^ false; 
        double c = 2.0 << 1; // Shifts em doubles dão erro no Juc!
        System.out.print(args); // Print de String[] dá erro
    }
}
""")

gerar_stress_fields()
gerar_stress_methods()
gerar_edge_cases_expr()
print("Testes gerados com sucesso!")