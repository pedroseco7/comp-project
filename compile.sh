#!/bin/sh
# Para a execucao caso algum comando falhe
set -e

echo "Limpar ficheiros antigos..."
rm -f jucompiler lex.yy.c y.tab.c y.tab.h y.output y.gv

echo "Executar Yacc..."
yacc -d -v -t jucompiler.y

echo "Executar Lex..."
lex jucompiler.l

echo "Compilar executavel..."
cc -o jucompiler lex.yy.c y.tab.c ast.c -Wall -Wno-unused-function

echo "Copiar para a pasta de testes..."
cp jucompiler ../tests/comp/java

echo "Compilacao terminada com sucesso!"