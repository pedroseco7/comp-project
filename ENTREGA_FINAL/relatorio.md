
# Relatório 
### 1. Gramática Re-escrita
A conversão da gramática original EBNF da linguagem Juc para LALR(1) exigiu reestruturações arquiteturais para garantir a compatibilidade com a ferramenta Yacc e a especificação do subconjunto Java.

- **Eliminação de EBNF e Recursividade**: As construções de repetição {...} e opcionalidade [...] foram convertidas em regras recursivas. Optou-se estritamente pela recursividade à esquerda em produções como *ProgramDecls*, *MethodBodyDecls*, *StatementList* e *IdList*. Esta decisão técnica otimiza o uso da memória, minimizando o consumo da stack do parser LALR e prevenindo *overflows* na análise de ficheiros de códigos extensos, algo que a recursividade à direita não cumpre.

- **Resolução de Ambiguidades (Dangling-Else)**: O conflito de shift/reduce gerado pela instrução if-else foi erradicado através da manipulação direta das precedências do Yacc. Foram introduzidos os tokens virtuais %nonassoc IF_NO_ELSE e %nonassoc ELSE. Ao associar a regra do if simples à menor precedência, forçámos o parser a executar um shift do token ELSE em vez de um reduce prematuro.

- **Hierarquia e Precedência de Expressões**: Em vez de depender exclusivamente das declarações %left e %right no cabeçalho Yacc, a precedência e associatividade dos operadores foram codificadas na própria estrutura geométrica das produções. A regra Expr foi estratificada numa hierarquia que desce de ExprOr até ExprPostfix (passando por ExprAnd, ExprEq, ExprRel, ExprAdd, etc...). Isto assegura a prioridade correta das operações aritméticas e lógicas, eliminando ambiguidades estruturais de forma nativa.

- **Propagação de Contexto Lexical**: Para suportar uma emissão de erros cirúrgica na fase semântica, o tipo %union transporta não apenas o identificador, mas os metadados de localização (line, col). Durante as reduções das regras, estas coordenadas espaciais são imediatamente cravadas nos nós terminais e transitadas pelas expressões.

- **Simplificação Dinâmica da Árvore**: A gramática otimiza a árvore em tempo de análise sintática. Na produção de blocos de instruções (Statement -> LBRACE StatementList RBRACE), a ação semântica avalia a cardinalidade da lista: blocos vazios retornam NULL e blocos com apenas uma instrução promovem esse filho diretamente. Isto suprime a alocação de nós Block supérfluos, minimizando a pegada de memória da AST. 

- **Recuperação de Erros**: A gramática implementa pontos de sincronização utilizando a palavra-chave error em produções estratégicas (e.g., Statement -> error SEMICOLON e MethodInvocation -> IDENTIFIER LPAR error RPAR). isto permite o resgate local da análise sintática, descartando tokens inválidos até encontrar um delimitador seguro como o ; ou ), evitando a interrupção prematura da compilação.
