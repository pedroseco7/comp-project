%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantics.h" 

int yylex(void);
void yyerror(char *);
struct node *ast;
int print_sym = 0;

struct node *newnode(enum category category, char *token);
void addchild(struct node *parent, struct node *child);
void show(struct node *node, int depth);
void free_list_and_node(struct node *n);
void codegen_program(struct node *program);

extern int cur_line, cur_col;
extern char *yytext;
extern int error_line, error_col;
extern char error_yytext[];
%}

%union {
    struct {
        char *id;
        int line;
        int col;
    } info;
    struct node *node;
}

%token <info> AND ASSIGN STAR COMMA DIV EQ GE GT LBRACE LE LPAR LSQ LT MINUS MOD NE NOT OR PLUS RBRACE RPAR RSQ SEMICOLON ARROW LSHIFT RSHIFT XOR BOOL CLASS DOTLENGTH DOUBLE ELSE IF INT PRINT PARSEINT PUBLIC RETURN STATIC STRING VOID WHILE RESERVED
%token <info> IDENTIFIER NATURAL DECIMAL STRLIT BOOLLIT

%type <node> Program ProgramDecls MethodDecl FieldDecl Type MethodHeader MethodParams FormalParams FormalParamsList MethodBody MethodBodyDecls VarDecl IdList Statement StatementList MethodInvocation ExprList Assignment ParseArgs Expr ExprOr ExprAnd ExprXor ExprEq ExprRel ExprShift ExprAdd ExprMult ExprUnary ExprPostfix

%nonassoc IF_NO_ELSE
%nonassoc ELSE

%%

Program: CLASS IDENTIFIER LBRACE ProgramDecls RBRACE { 
            $$ = newnode(Program, NULL);
            struct node *id = newnode(Identifier, $2.id);
            id->line = $2.line; id->col = $2.col;
            addchild($$, id);
            if ($4 != NULL) {
                struct node_list *child = $4->children;
                while (child != NULL) {
                    addchild($$, child->node);
                    child = child->next;
                }
                free_list_and_node($4);
            }
            ast = $$;
        } 
       ;

ProgramDecls: ProgramDecls MethodDecl { 
                $$ = $1;
                if ($$ == NULL) $$ = newnode(Program, NULL);
                if ($2 != NULL) addchild($$, $2);
            }
            | ProgramDecls FieldDecl { 
                $$ = $1;
                if ($$ == NULL) $$ = newnode(Program, NULL); 
                if ($2 != NULL) {
                    struct node_list *child = $2->children;
                    while (child != NULL) {
                        addchild($$, child->node);
                        child = child->next;
                    }
                    free_list_and_node($2);
                }
            }
            | ProgramDecls SEMICOLON { $$ = $1; }
            | { $$ = NULL; }
            ;

MethodDecl: PUBLIC STATIC MethodHeader MethodBody { 
                $$ = newnode(MethodDecl, NULL);
                addchild($$, $3);
                addchild($$, $4);
            } 
          ;

FieldDecl: PUBLIC STATIC Type IDENTIFIER IdList SEMICOLON { 
                $$ = newnode(Program, NULL);
                struct node *first = newnode(FieldDecl, NULL);
                addchild(first, newnode($3->category, NULL));
                struct node *id = newnode(Identifier, $4.id);
                id->line = $4.line;
                id->col = $4.col;
                addchild(first, id);
                addchild($$, first);
                if ($5 != NULL) {
                    struct node_list *child = $5->children;
                    while (child != NULL) {
                        struct node *next_field = newnode(FieldDecl, NULL);
                        addchild(next_field, newnode($3->category, NULL));
                        addchild(next_field, child->node);
                        addchild($$, next_field);
                        child = child->next;
                    }
                    free_list_and_node($5);
                }
            }
         | error SEMICOLON { $$ = NULL; }
         ;

Type: BOOL { $$ = newnode(Bool, NULL); }
    | INT { $$ = newnode(Int, NULL); }
    | DOUBLE { $$ = newnode(Double, NULL); }
    ;

MethodHeader: Type IDENTIFIER LPAR MethodParams RPAR { 
                $$ = newnode(MethodHeader, NULL);
                addchild($$, $1);
                struct node *id = newnode(Identifier, $2.id);
                id->line = $2.line; id->col = $2.col;
                addchild($$, id);
                addchild($$, $4);
            }
            | VOID IDENTIFIER LPAR MethodParams RPAR { 
                $$ = newnode(MethodHeader, NULL);
                addchild($$, newnode(Void, NULL));
                struct node *id = newnode(Identifier, $2.id);
                id->line = $2.line; id->col = $2.col;
                addchild($$, id);
                addchild($$, $4);
            }
            ;

MethodParams: FormalParams { 
                $$ = newnode(MethodParams, NULL);
                if ($1 != NULL) {
                    struct node_list *child = $1->children;
                    while (child != NULL) {
                        addchild($$, child->node);
                        child = child->next;
                    }
                    free_list_and_node($1);
                }
            }
            | { $$ = newnode(MethodParams, NULL); }
            ;

FormalParams: Type IDENTIFIER FormalParamsList { 
                $$ = newnode(Program, NULL);
                struct node *p1 = newnode(ParamDecl, NULL);
                addchild(p1, $1);
                struct node *id = newnode(Identifier, $2.id);
                id->line = $2.line; id->col = $2.col;
                addchild(p1, id);
                addchild($$, p1);
                if ($3 != NULL) {
                    struct node_list *child = $3->children;
                    while (child != NULL) {
                        addchild($$, child->node);
                        child = child->next;
                    }
                    free_list_and_node($3);
                }
            }
            | STRING LSQ RSQ IDENTIFIER { 
                $$ = newnode(Program, NULL);
                struct node *p1 = newnode(ParamDecl, NULL);
                addchild(p1, newnode(StringArray, NULL));
                struct node *id = newnode(Identifier, $4.id);
                id->line = $4.line;
                id->col = $4.col;
                addchild(p1, id);
                addchild($$, p1);
            }
            ;

FormalParamsList: FormalParamsList COMMA Type IDENTIFIER { 
                    $$ = $1;
                    if ($$ == NULL) $$ = newnode(Program, NULL);
                    struct node *p = newnode(ParamDecl, NULL);
                    addchild(p, $3);
                    struct node *id = newnode(Identifier, $4.id);
                    id->line = $4.line; id->col = $4.col;
                    addchild(p, id);
                    addchild($$, p);
                }
                | { $$ = NULL; }
                ;

MethodBody: LBRACE MethodBodyDecls RBRACE { 
                $$ = newnode(MethodBody, NULL);
                if ($2 != NULL) {
                    struct node_list *child = $2->children;
                    while (child != NULL) {
                        addchild($$, child->node);
                        child = child->next;
                    }
                    free_list_and_node($2);
                }
            } 
          ;

MethodBodyDecls: MethodBodyDecls Statement { 
                    $$ = $1;
                    if ($$ == NULL) $$ = newnode(Program, NULL);
                    if ($2 != NULL) addchild($$, $2);
               }
               | MethodBodyDecls VarDecl { 
                    $$ = $1;
                    if ($$ == NULL) $$ = newnode(Program, NULL);
                    if ($2 != NULL) {
                        struct node_list *child = $2->children;
                        while (child != NULL) {
                            addchild($$, child->node);
                            child = child->next;
                        }
                        free_list_and_node($2);
                    }
                }
               | { $$ = NULL; }
               ;

VarDecl: Type IDENTIFIER IdList SEMICOLON { 
            $$ = newnode(Program, NULL);
            struct node *first = newnode(VarDecl, NULL);
            addchild(first, newnode($1->category, NULL));
            struct node *id = newnode(Identifier, $2.id);
            id->line = $2.line;
            id->col = $2.col;
            addchild(first, id);
            addchild($$, first);
            if ($3 != NULL) {
                struct node_list *child = $3->children;
                while (child != NULL) {
                    struct node *next_var = newnode(VarDecl, NULL);
                    addchild(next_var, newnode($1->category, NULL));
                    addchild(next_var, child->node);
                    addchild($$, next_var);
                    child = child->next;
                }
                free_list_and_node($3);
            }
        } 
       ;

IdList: IdList COMMA IDENTIFIER { 
            $$ = $1;
            if ($$ == NULL) $$ = newnode(Program, NULL);
            struct node *id = newnode(Identifier, $3.id);
            id->line = $3.line; id->col = $3.col;
            addchild($$, id);
        }
      | { $$ = NULL; }
      ;

Statement: LBRACE StatementList RBRACE 
         { 
             int count = 0;
             if ($2 != NULL) {
                 struct node_list *child = $2->children;
                 while (child != NULL) {
                     count++;
                     child = child->next;
                 }
             }

             if (count == 0) {
                 $$ = NULL;
                 if ($2) free_list_and_node($2);
             } else if (count == 1) {
                 $$ = $2->children->node;
                 free_list_and_node($2);
             } else {
                 $$ = newnode(Block, NULL);
                 struct node_list *child = $2->children;
                 while (child != NULL) {
                     addchild($$, child->node);
                     child = child->next;
                 }
                 free_list_and_node($2);
             }
         }
         | IF LPAR Expr RPAR Statement %prec IF_NO_ELSE 
         { 
             $$ = newnode(If, NULL);
             addchild($$, $3);
             addchild($$, $5 != NULL ? $5 : newnode(Block, NULL));
             addchild($$, newnode(Block, NULL));
         }
         | IF LPAR Expr RPAR Statement ELSE Statement 
         { 
             $$ = newnode(If, NULL);
             addchild($$, $3);
             addchild($$, $5 != NULL ? $5 : newnode(Block, NULL));
             addchild($$, $7 != NULL ? $7 : newnode(Block, NULL));
         }
         | WHILE LPAR Expr RPAR Statement 
         { 
             $$ = newnode(While, NULL);
             addchild($$, $3);
             addchild($$, $5 != NULL ? $5 : newnode(Block, NULL));
         }
         | RETURN SEMICOLON 
         { 
             $$ = newnode(Return, NULL);
             $$->line = $1.line; $$->col = $1.col;
         }
         | RETURN Expr SEMICOLON 
         { 
             $$ = newnode(Return, NULL);
             $$->line = $1.line; $$->col = $1.col;
             addchild($$, $2);
         }
         | MethodInvocation SEMICOLON { $$ = $1; }
         | Assignment SEMICOLON { $$ = $1; }
         | ParseArgs SEMICOLON { $$ = $1; }
         | SEMICOLON { $$ = NULL; }
         | PRINT LPAR Expr RPAR SEMICOLON 
         { 
             $$ = newnode(Print, NULL);
             addchild($$, $3);
         }
         | PRINT LPAR STRLIT RPAR SEMICOLON 
        { 
            $$ = newnode(Print, NULL);
            struct node *str = newnode(StrLit, $3.id);
            str->line = $3.line; str->col = $3.col;
            addchild($$, str);
        }
         | error SEMICOLON { $$ = NULL; }
         ;

StatementList: StatementList Statement 
             { 
                 $$ = $1;
                 if ($$ == NULL) $$ = newnode(Program, NULL); 
                 if ($2 != NULL) addchild($$, $2);
             }
             | { $$ = newnode(Program, NULL); }
             ;

MethodInvocation: IDENTIFIER LPAR RPAR 
                { 
                    $$ = newnode(Call, NULL);
                    $$->line = $1.line; $$->col = $1.col;
                    struct node *id = newnode(Identifier, $1.id);
                    id->line = $1.line; id->col = $1.col;
                    id->is_expr = 1;
                    addchild($$, id);
                }
                | IDENTIFIER LPAR ExprList RPAR 
                { 
                    $$ = newnode(Call, NULL);
                    $$->line = $1.line; $$->col = $1.col;
                    struct node *id = newnode(Identifier, $1.id);
                    id->line = $1.line; id->col = $1.col;
                    id->is_expr = 1;
                    addchild($$, id);
                    struct node_list *child = $3->children;
                    while (child != NULL) {
                        addchild($$, child->node);
                        child = child->next;
                    }
                    free_list_and_node($3);
                }
                | IDENTIFIER LPAR error RPAR { $$ = NULL; }
                ;

Assignment: IDENTIFIER ASSIGN Expr 
          { 
              $$ = newnode(Assign, NULL);
              $$->line = $2.line; $$->col = $2.col;
              struct node *id = newnode(Identifier, $1.id);
              id->line = $1.line; id->col = $1.col;
              id->is_expr = 1;
              addchild($$, id); addchild($$, $3);
          }
          ;

ParseArgs: PARSEINT LPAR IDENTIFIER LSQ Expr RSQ RPAR 
         { 
             $$ = newnode(ParseArgs, NULL);
             $$->line = $1.line; $$->col = $1.col;
             struct node *id = newnode(Identifier, $3.id);
             id->line = $3.line; id->col = $3.col;
             id->is_expr = 1;
             addchild($$, id);
             addchild($$, $5);
         }
         | PARSEINT LPAR error RPAR { $$ = NULL; }
         ;

ExprList: Expr 
        { 
            $$ = newnode(Program, NULL);
            addchild($$, $1);
        }
        | ExprList COMMA Expr 
        { 
            $$ = $1;
            addchild($$, $3);
        }
        ;

Expr: Assignment { $$ = $1; }
    | ExprOr     { $$ = $1; }
    ;

ExprOr: ExprOr OR ExprAnd 
        { $$ = newnode(Or, NULL); $$->line = $2.line; $$->col = $2.col; addchild($$, $1); addchild($$, $3); }
      | ExprAnd { $$ = $1; }
      ;

ExprAnd: ExprAnd AND ExprXor 
        { $$ = newnode(And, NULL); $$->line = $2.line; $$->col = $2.col; addchild($$, $1); addchild($$, $3); }
       | ExprXor { $$ = $1; }
       ;

ExprXor: ExprXor XOR ExprEq 
        { $$ = newnode(Xor, NULL); $$->line = $2.line; $$->col = $2.col; addchild($$, $1); addchild($$, $3); }
       | ExprEq { $$ = $1; }
       ;

ExprEq: ExprEq EQ ExprRel 
        { $$ = newnode(Eq, NULL); $$->line = $2.line; $$->col = $2.col; addchild($$, $1); addchild($$, $3); }
      | ExprEq NE ExprRel 
        { $$ = newnode(Ne, NULL); $$->line = $2.line; $$->col = $2.col; addchild($$, $1); addchild($$, $3); }
      | ExprRel { $$ = $1; }
      ;

ExprRel: ExprRel LT ExprShift 
        { $$ = newnode(Lt, NULL); $$->line = $2.line; $$->col = $2.col; addchild($$, $1); addchild($$, $3); }
       | ExprRel GT ExprShift 
        { $$ = newnode(Gt, NULL); $$->line = $2.line; $$->col = $2.col; addchild($$, $1); addchild($$, $3); }
       | ExprRel LE ExprShift 
        { $$ = newnode(Le, NULL); $$->line = $2.line; $$->col = $2.col; addchild($$, $1); addchild($$, $3); }
       | ExprRel GE ExprShift 
        { $$ = newnode(Ge, NULL); $$->line = $2.line; $$->col = $2.col; addchild($$, $1); addchild($$, $3); }
       | ExprShift { $$ = $1; }
       ;

ExprShift: ExprShift LSHIFT ExprAdd 
           { $$ = newnode(Lshift, NULL); $$->line = $2.line; $$->col = $2.col; addchild($$, $1); addchild($$, $3); }
         | ExprShift RSHIFT ExprAdd 
           { $$ = newnode(Rshift, NULL); $$->line = $2.line; $$->col = $2.col; addchild($$, $1); addchild($$, $3); }
         | ExprAdd { $$ = $1; }
         ;

ExprAdd: ExprAdd PLUS ExprMult  
         { $$ = newnode(Add, NULL); $$->line = $2.line; $$->col = $2.col; addchild($$, $1); addchild($$, $3); }
       | ExprAdd MINUS ExprMult 
         { $$ = newnode(Sub, NULL); $$->line = $2.line; $$->col = $2.col; addchild($$, $1); addchild($$, $3); }
       | ExprMult { $$ = $1; }
       ;

ExprMult: ExprMult STAR ExprUnary 
          { $$ = newnode(Mul, NULL); $$->line = $2.line; $$->col = $2.col; addchild($$, $1); addchild($$, $3); }
        | ExprMult DIV ExprUnary  
          { $$ = newnode(Div, NULL); $$->line = $2.line; $$->col = $2.col; addchild($$, $1); addchild($$, $3); }
        | ExprMult MOD ExprUnary  
          { $$ = newnode(Mod, NULL); $$->line = $2.line; $$->col = $2.col; addchild($$, $1); addchild($$, $3); }
        | ExprUnary { $$ = $1; }
        ;

ExprUnary: PLUS ExprUnary  { $$ = newnode(Plus, NULL); $$->line = $1.line; $$->col = $1.col; addchild($$, $2); }
         | MINUS ExprUnary { $$ = newnode(Minus, NULL); $$->line = $1.line; $$->col = $1.col; addchild($$, $2); }
         | NOT ExprUnary   { $$ = newnode(Not, NULL); $$->line = $1.line; $$->col = $1.col; addchild($$, $2); }
         | ExprPostfix     { $$ = $1; }
         ;

ExprPostfix: MethodInvocation { $$ = $1; }
           | ParseArgs { $$ = $1; }
           | IDENTIFIER DOTLENGTH  
             { 
                 $$ = newnode(Length, NULL);
                 $$->line = $2.line; $$->col = $2.col;
                 struct node *id = newnode(Identifier, $1.id);
                 id->line = $1.line; id->col = $1.col;
                 id->is_expr = 1;
                 addchild($$, id); 
             }
           | IDENTIFIER { $$ = newnode(Identifier, $1.id); $$->line = $1.line; $$->col = $1.col; $$->is_expr = 1; }
           | NATURAL { $$ = newnode(Natural, $1.id); $$->line = $1.line; $$->col = $1.col; $$->is_expr = 1; }
           | DECIMAL { $$ = newnode(Decimal, $1.id); $$->line = $1.line; $$->col = $1.col; $$->is_expr = 1; }
           | BOOLLIT { $$ = newnode(BoolLit, $1.id); $$->line = $1.line; $$->col = $1.col; $$->is_expr = 1; }
           | LPAR Expr RPAR { $$ = $2; }
           | LPAR error RPAR { $$ = NULL; }
           ;
%%

struct node *newnode(enum category category, char *token) {
    struct node *new = malloc(sizeof(struct node));
    new->category = category;
    new->token = token;
    new->type = TypeNone;
    new->is_expr = 0;
    new->is_duplicate = 0;
    new->func_sig = NULL;
    new->line = 0;
    new->col = 0;
    new->children = NULL;
    new->tail = NULL;
    return new;
}

void addchild(struct node *parent, struct node *child) {
    if (parent == NULL || child == NULL) return;
    struct node_list *new = malloc(sizeof(struct node_list));
    new->node = child;
    new->next = NULL;
    if (parent->children == NULL) {
        parent->children = new;
        parent->tail = new;
    } else {
        if (parent->tail != NULL) {
            parent->tail->next = new;
            parent->tail = new;
        } else {
            struct node_list *curr = parent->children;
            while(curr->next != NULL) curr = curr->next;
            curr->next = new;
            parent->tail = new;
        }
    }
}

void free_list_and_node(struct node *n) {
    if (!n) return;
    struct node_list *curr = n->children;
    while (curr) {
        struct node_list *tmp = curr;
        curr = curr->next;
        free(tmp);
    }
    free(n);
}

const char *category_names[] = {
    "Program", "FieldDecl", "VarDecl", "MethodDecl", "MethodHeader", 
    "MethodParams", "ParamDecl", "MethodBody", "Block", "If", "While", 
    "Return", "Call", "Print", "ParseArgs", "Assign", "Or", "And", "Eq", 
    "Ne", "Lt", "Gt", "Le", "Ge", "Add", "Sub", "Mul", "Div", "Mod", 
    "Lshift", "Rshift", "Xor", "Not", "Minus", "Plus", "Length", "Bool", 
    "BoolLit", "Double", "Decimal", "Identifier", "Int", "Natural", 
    "StrLit", "StringArray", "Void"
};

void show(struct node *node, int depth) {
    if (node == NULL) return;
    for (int i = 0; i < depth; i++) printf("..");
    if (node->token != NULL) {
        if (node->category == StrLit) {
            printf("%s(\"%s\")", category_names[node->category], node->token);
        } else {
            printf("%s(%s)", category_names[node->category], node->token);
        }
    } else {
        printf("%s", category_names[node->category]);
    }

    if (print_sym && node->is_expr) {
        if (node->func_sig != NULL) {
            printf(" - %s\n", node->func_sig);
        } else if (node->type != TypeNone) {
            printf(" - %s\n", get_type_name(node->type));
        } else {
            printf("\n");
        }
    } else {
        printf("\n");
    }

    struct node_list *curr = node->children;
    while (curr) { show(curr->node, depth + 1); curr = curr->next; }
}

extern int verbose;
int syntax_errors = 0; 

extern int lexical_errors; 

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IOFBF, 65536);
    
    int print_ast = 0;
    int run_semantics = 1;
    int generate_code = (argc == 1); 

    if (argc > 1) {
        if (strcmp(argv[1], "-l") == 0 || strcmp(argv[1], "-1") == 0) {
            verbose = 1;
            while (yylex() != 0); 
            return 0;
        } 
        else if (strcmp(argv[1], "-e1") == 0) {
            verbose = 0;
            while (yylex() != 0); 
            return 0;
        } 
        else if (strcmp(argv[1], "-e2") == 0) {
            yyparse(); 
            return 0;
        } 
        else if (strcmp(argv[1], "-t") == 0) {
            print_ast = 1;
            run_semantics = 0;
        }
        else if (strcmp(argv[1], "-s") == 0) {
            print_sym = 1;
            run_semantics = 1;
        }
        else if (strcmp(argv[1], "-e3") == 0) {
            run_semantics = 1;
        }
    }

    yyparse();

    if (syntax_errors == 0) {
        if (print_ast) {
            show(ast, 0);
        }
        
        if (run_semantics) {
            build_symbol_tables(ast);
            annotate_ast(ast, NULL);
            
            if (print_sym) {
                print_symbol_tables();
                show(ast, 0);
            }
            
            if (generate_code && semantic_errors == 0 && lexical_errors == 0) {
                codegen_program(ast);
            }
        }
    }
    return 0;
}

void yyerror(char *error) {
    syntax_errors++;
    printf("Line %d, col %d: %s: %s\n", error_line, error_col, error, error_yytext);
}