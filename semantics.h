#ifndef _SEMANTICS_H
#define _SEMANTICS_H

/* Tipos semânticos da linguagem Juc [cite: 301, 305, 602] */
typedef enum {
    TypeInt, 
    TypeDouble, 
    TypeBoolean, 
    TypeStringArray, 
    TypeVoid, 
    TypeUndef, 
    TypeNone,
    TypeString
} ExprType;

/* Categorias da AST (migradas do seu .y) */
enum category {
    Program, FieldDecl, VarDecl, MethodDecl, MethodHeader, 
    MethodParams, ParamDecl, MethodBody, Block, If, While, 
    Return, Call, Print, ParseArgs, Assign, Or, And, Eq, 
    Ne, Lt, Gt, Le, Ge, Add, Sub, Mul, Div, Mod, 
    Lshift, Rshift, Xor, Not, Minus, Plus, Length, Bool, 
    BoolLit, Double, Decimal, Identifier, Int, Natural, 
    StrLit, StringArray, Void
};

/* Estrutura do Nó da Árvore com campos de anotação [cite: 646] */
struct node {
    enum category category;
    char *token;
    ExprType type;
    int is_expr;
    int is_duplicate;
    char *func_sig;
    int line, col;
    struct node_list *children;
    struct node_list *tail; /* NOVO: Ponteiro O(1) para a árvore */
};

struct node_list {
    struct node *node;
    struct node_list *next;
};

/* Estrutura de um Símbolo na Tabela [cite: 604] */
struct symbol {
    char *name;
    ExprType type;
    ExprType *param_types;
    int num_params;
    int is_param;
    int is_method; 
    int line, col;
    struct symbol *next;
};

struct symbol_table {
    char *table_name;
    char *table_type;
    struct symbol *first_symbol; 
    struct symbol *last_symbol; /* NOVO: Ponteiro O(1) para os símbolos */
    struct symbol_table *next;
};

extern struct symbol_table *sym_tables;
extern int semantic_errors;

/* Funções de gestão */
void build_symbol_tables(struct node *ast_root);
void print_symbol_tables();
void annotate_ast(struct node *node, struct symbol_table *method_table);
const char* get_type_name(ExprType type);
struct symbol *lookup_symbol(struct symbol_table *local, struct symbol_table *global, char *name, int line, int col);
void print_errors();

#endif