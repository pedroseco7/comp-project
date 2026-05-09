
# Relatório 
### 1. Gramática Re-escrita
A conversão da gramática original EBNF da linguagem Juc para LALR(1) exigiu reestruturações arquiteturais para garantir a compatibilidade com a ferramenta Yacc e a especificação do subconjunto Java.

- **Eliminação de EBNF e Recursividade**: As construções de repetição {...} e opcionalidade [...] foram convertidas em regras recursivas. Optou-se estritamente pela recursividade à esquerda em produções como *ProgramDecls*, *MethodBodyDecls*, *StatementList* e *IdList*. Esta decisão técnica otimiza o uso da memória, minimizando o consumo da stack do parser LALR e prevenindo *overflows* na análise de ficheiros de códigos extensos, algo que a recursividade à direita não cumpre.

- **Resolução de Ambiguidades (Dangling-Else)**: O conflito de shift/reduce gerado pela instrução if-else foi erradicado através da manipulação direta das precedências do Yacc. Foram introduzidos os tokens virtuais %nonassoc IF_NO_ELSE e %nonassoc ELSE. Ao associar a regra do if simples à menor precedência, forçámos o parser a executar um shift do token ELSE em vez de um reduce prematuro.

- **Hierarquia e Precedência de Expressões**: Em vez de depender exclusivamente das declarações %left e %right no cabeçalho Yacc, a precedência e associatividade dos operadores foram codificadas na própria estrutura geométrica das produções. A regra Expr foi estratificada numa hierarquia que desce de ExprOr até ExprPostfix (passando por ExprAnd, ExprEq, ExprRel, ExprAdd, etc...). Isto assegura a prioridade correta das operações aritméticas e lógicas, eliminando ambiguidades estruturais de forma nativa.

- **Propagação de Contexto Lexical**: Para suportar uma emissão de erros cirúrgica na fase semântica, o tipo %union transporta não apenas o identificador, mas os metadados de localização (line, col). Durante as reduções das regras, estas coordenadas espaciais são imediatamente cravadas nos nós terminais e transitadas pelas expressões.

- **Simplificação Dinâmica da Árvore**: A gramática otimiza a árvore em tempo de análise sintática. Na produção de blocos de instruções (Statement -> LBRACE StatementList RBRACE), a ação semântica avalia a cardinalidade da lista: blocos vazios retornam NULL e blocos com apenas uma instrução promovem esse filho diretamente. Isto suprime a alocação de nós Block supérfluos, minimizando a pegada de memória da AST. 

- **Recuperação de Erros**: A gramática implementa pontos de sincronização utilizando a palavra-chave error em produções estratégicas (e.g., Statement -> error SEMICOLON e MethodInvocation -> IDENTIFIER LPAR error RPAR). isto permite o resgate local da análise sintática, descartando tokens inválidos até encontrar um delimitador seguro como o ; ou ), evitando a interrupção prematura da compilação.


### 2. Algoritmos e Estruturas de Dados
Esta secção descreve as estruturas fundamentais que suportam a análise semântica e a geração de código, focando-se na eficiência algorítmica para processar programas mais complexas.

- **Árvore de Sintaxe Abstrata (AST)**: A AST é implementada através de uma estrutura de dados `node`, composta por uma categoria (enum), metadados de localização (`line`, `col`) e anotações de tipo (`ExprType`).
    - **Otimização de Inserção O(1)**: Ao contrário de implementações que percorrem a lista de filhos para adicionar um novo nó, a nossa estrutura inclui um ponteiro `tail`. Isto permite que a função `addchild` execute a ligação de novos ramos em tempo constante, independentemente da dimensão da árvore.
    - **Anotação e Travessia**: O algoritmo de anotação (`annotate_ast`) utiliza uma travessia *post-order* recursiva. Isto garante que os tipos das folhas (literais e identificadores) sejam determinados antes da validação dos nós pais (operadores), permitindo a propagação correta de tipos e a deteção de imcompatibilidades semânticas

- **Tabelas de Símbolos e Gestão de Escopo**: A tabela de símbolos segue uma hierarquia de escopos, onde uma tabela global (`Class`) contém referências para tabelas locais (`Method`). Cada entrada é representada pela estrutura `symbol`, que armazena assinaturas de métodos, tipos de retorno e parâmetros.

- **Cache Hash Maps**: Para garantir a viabilidade do compilador perante testes massivos do Mooshak, implementámos uma camada de cache sobre as tabelas de símbolos.
    - **Indexação Global O(1)**: Através da função de dispersão `hash_str`, criámos os mapas `sys_cache` e `table_cache_map` com um tamanho de 65537 entradas. 
    - **Pesquisa Eficiente**: Esta estrutura transforma pesquisas que seriam linearmente lentas (O(n)) em acessos praticamente instantâneos (O(1)). A função `lookup_symbol` utiliza esta cache para resolver identificadores, verificando primeiro o escopo local e depois o global, respeitando as regras de *shadowing* e a ordem de declaração.
    - **Sobrecarga**: O sistema de cache lida nativamente com a sobrecarga de métodos (*method overloading*), armazenando as assinaturas completas (`mangled names`) para distinguir funções com o mesmo nome mas parâmetros distintos.