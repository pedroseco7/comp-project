#!/bin/sh
set -e
rm -f jucompiler lex.yy.c y.tab.c y.tab.h y.output y.gv
yacc -d -v -t jucompiler.y
lex jucompiler.l
cc -o jucompiler lex.yy.c y.tab.c -Wall -Wno-unused-function
cp jucompiler ../tests/comp/java