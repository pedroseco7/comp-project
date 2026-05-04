#!/bin/sh
set -e
# Limpar ficheiros antigos
rm -f jucompiler lex.yy.c y.tab.c y.tab.h y.output y.gv

# Gerar o Parser e o Lexer
yacc -d -v -t jucompiler.y
lex jucompiler.l

# Compilar incluindo o semantics.c e o codegen.c (Meta 4)
cc -o jucompiler lex.yy.c y.tab.c semantics.c codegen.c -Wall -Wno-unused-function

# Copiar para a pasta de testes
cp jucompiler ../tests/comp/java/jucompiler
echo "Compilação concluída com sucesso e copiado para ../tests/comp/java/jucompiler!"