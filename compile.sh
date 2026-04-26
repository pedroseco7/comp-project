#!/bin/sh
set -e
# Limpar ficheiros antigos
rm -f jucompiler lex.yy.c y.tab.c y.tab.h y.output y.gv

# Gerar o Parser e o Lexer
yacc -d -v -t jucompiler.y
lex jucompiler.l

# Compilar incluindo o semantics.c
# Adicionámos o semantics.c à lista de ficheiros a compilar
cc -o jucompiler lex.yy.c y.tab.c semantics.c -Wall -Wno-unused-function

# Copiar para a pasta de testes
cp jucompiler ../tests/comp/java