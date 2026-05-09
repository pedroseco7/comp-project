
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
    - **Pesquisa Eficiente**: Esta estrutura transforma

### 3. Geração de Código

A última fase do compilador percorre a AST anotada e emite diretamente código LLVM IR equivalente ao programa fonte, sem passar por uma forma intermédia própria. A escolha do LLVM como alvo dispensou-nos da alocação manual de registos físicos, simplificando significativamente esta meta.

- **Estratégia para Variáveis e Parâmetros**: Cada variável local e parâmetro recebe espaço próprio na pilha através da instrução `alloca`, sendo as leituras e escritas traduzidas em `load` e `store`. Esta abordagem foi adotada por simplificar o tratamento de atribuições repetidas à mesma variável: como cada `store` substitui o valor anterior, não é preciso reconciliar versões diferentes de uma variável após um `if` ou no fim de um ciclo. Os parâmetros são copiados para os respetivos slots no início da função para uniformizar o acesso.

- **Hoisting das Alocações**: Todas as alocações são empurradas para o bloco de entrada da função através de uma pré-passagem (`print_allocas`) que percorre o corpo do método e recolhe todos os `VarDecl`, mesmo aqueles declarados em blocos. Esta decisão evita um problema concreto observado em testes iniciais: declarar variáveis dentro de um `while` faria com que `alloca` consumisse pilha a cada iteração, causando *stack overflow* em ciclos longos. Centralizar as alocações no prólogo garante que o tamanho do *frame* é fixo.

- **Tradução do Fluxo de Controlo**: As construções `if-else` e `while` são partidas em blocos identificados por etiquetas sequenciais (`L0`, `L1`, ...), ligadas por `br i1` (condicional) ou `br label` (incondicional). Os operadores `&&` e `||` são implementados com avaliação em curto-circuito recorrendo a um slot auxiliar (`%sc_temp`) onde se guarda o resultado parcial, em coerência com a abordagem geral baseada em memória. Uma *flag* (`block_terminated`) impede a emissão de instruções após um `return` ou `br`, garantindo IR aceite pelo verificador do LLVM.

- **Seleção de Instruções e Conversões**: Os tipos `int`, `boolean` e `double` mapeiam para `i32`, `i1` e `double`. O gerador escolhe entre instruções inteiras e de vírgula flutuante consoante os operandos: `add`/`icmp` para inteiros e booleanos, `fadd`/`fcmp` para reais. Quando uma operação mistura `int` com `double`, o operando inteiro é promovido com `sitofp`. A mesma promoção é aplicada em atribuições, `return` e passagem de argumentos.

- **Mangling e Caso Especial do main**: Para suportar a sobrecarga de métodos validada na meta anterior, os nomes recebem um sufixo que codifica os tipos dos parâmetros formais (`m_factorial_i`, `m_foo_d_b`). O `main` é a única excepção: mantém o nome original e a sua assinatura `String[] args` é traduzida para o par `(i32 %argc, i8** %argv)` da convenção C, com `args.length` reduzido a `sub i32 %argc, 1` e `Integer.parseInt(args[i])` a `getelementptr` seguido de `call @atoi`.

- **Literais e Saída**: As `String`s do programa são emitidas como constantes globais (`@.str.N`) e endereçadas com `getelementptr`. `System.out.print` é traduzido em chamadas a `printf` com `%d`, `%.16e` e `%s` consoante o tipo; para booleanos, a string `"true"` ou `"false"` é seleccionada em runtime através de `select i1`.