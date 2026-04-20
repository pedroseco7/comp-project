%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum category {
    Program, FieldDecl, VarDecl, MethodDecl, MethodHeader, 
    MethodParams, ParamDecl, MethodBody, Block, If, While, 
    Return, Call, Print, ParseArgs, Assign, Or, And, Eq, 
    Ne, Lt, Gt, Le, Ge, Add, Sub, Mul, Div, Mod, 
    Lshift, Rshift, Xor, Not, Minus, Plus, Length, Bool, 
    BoolLit, Double, Decimal, Identifier, Int, Natural, 
    StrLit, StringArray, Void
};

struct node {
    enum category category;
    char *token;
    struct node_list *children;
};

struct node_list {
    struct node *node;
    struct node_list *next;
};

struct node *newnode(enum category category, char *token);
void addchild(struct node *parent, struct node *child);
void show(struct node *node, int depth);

int yylex(void);
void yyerror(char *);

struct node *ast;

extern int cur_line, cur_col;
extern char *yytext;

extern int error_line, error_col;
extern char error_yytext[];
%}

%token AND ASSIGN STAR COMMA DIV EQ GE GT LBRACE LE LPAR LSQ LT MINUS MOD NE NOT OR PLUS RBRACE RPAR RSQ SEMICOLON ARROW LSHIFT RSHIFT XOR BOOL CLASS DOTLENGTH DOUBLE ELSE IF INT PRINT PARSEINT PUBLIC RETURN STATIC STRING VOID WHILE RESERVED
%token <lexeme> IDENTIFIER NATURAL DECIMAL STRLIT BOOLLIT

%type <node> Program ProgramDecls MethodDecl FieldDecl Type MethodHeader MethodParams FormalParams FormalParamsList MethodBody MethodBodyDecls VarDecl IdList Statement StatementList MethodInvocation ExprList Assignment ParseArgs Expr ExprOr ExprAnd ExprXor ExprEq ExprRel ExprShift ExprAdd ExprMult ExprUnary ExprPostfix

%union{
    char *lexeme;
    struct node *node;
}

%nonassoc IF_NO_ELSE
%nonassoc ELSE

%%

Program: CLASS IDENTIFIER LBRACE ProgramDecls RBRACE { 
            $$ = newnode(Program, NULL); 
            addchild($$, newnode(Identifier, $2));
            if ($4 != NULL) {
                struct node_list *child = $4->children;
                while (child != NULL) {
                    addchild($$, child->node);
                    child = child->next;
                }
                free($4);
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
                    free($2);
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
                addchild(first, newnode(Identifier, $4));
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
                    free($5);
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
                addchild($$, newnode(Identifier, $2));
                addchild($$, $4);
            }
            | VOID IDENTIFIER LPAR MethodParams RPAR { 
                $$ = newnode(MethodHeader, NULL);
                addchild($$, newnode(Void, NULL));
                addchild($$, newnode(Identifier, $2));
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
                    free($1);
                }
            }
            | { $$ = newnode(MethodParams, NULL); }
            ;

FormalParams: Type IDENTIFIER FormalParamsList { 
                $$ = newnode(Program, NULL);
                struct node *p1 = newnode(ParamDecl, NULL);
                addchild(p1, $1);
                addchild(p1, newnode(Identifier, $2));
                addchild($$, p1);
                if ($3 != NULL) {
                    struct node_list *child = $3->children;
                    while (child != NULL) {
                        addchild($$, child->node);
                        child = child->next;
                    }
                    free($3);
                }
            }
            | STRING LSQ RSQ IDENTIFIER { 
                $$ = newnode(Program, NULL); 
                struct node *p1 = newnode(ParamDecl, NULL);
                addchild(p1, newnode(StringArray, NULL));
                addchild(p1, newnode(Identifier, $4));
                addchild($$, p1);
            }
            ;

FormalParamsList: FormalParamsList COMMA Type IDENTIFIER { 
                    $$ = $1;
                    if ($$ == NULL) $$ = newnode(Program, NULL);
                    struct node *p = newnode(ParamDecl, NULL);
                    addchild(p, $3);
                    addchild(p, newnode(Identifier, $4));
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
                    free($2);
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
                        free($2);
                    }
                }
               | { $$ = NULL; }
               ;

VarDecl: Type IDENTIFIER IdList SEMICOLON { 
            $$ = newnode(Program, NULL);
            struct node *first = newnode(VarDecl, NULL);
            addchild(first, newnode($1->category, NULL));
            addchild(first, newnode(Identifier, $2));
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
                free($3);
            }
        } 
       ;

IdList: IdList COMMA IDENTIFIER { 
            $$ = $1;
            if ($$ == NULL) $$ = newnode(Program, NULL);
            addchild($$, newnode(Identifier, $3));
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
                 // Tem 0 statements: Elimina o Block completamente (retorna NULL)
                 $$ = NULL;
                 free($2);
             } else if (count == 1) {
                 // Tem apenas 1 statement: Omite o Block e passa o único filho para cima
                 $$ = $2->children->node;
                 free($2);
             } else {
                 // Tem >1 statements: Cria o Block e despeja o saco
                 $$ = newnode(Block, NULL);
                 if ($2 != NULL) {
                     struct node_list *child = $2->children;
                     while (child != NULL) {
                         addchild($$, child->node);
                         child = child->next;
                     }
                     free($2);
                 }
             }
         }
         | IF LPAR Expr RPAR Statement %prec IF_NO_ELSE 
         { 
             $$ = newnode(If, NULL);
             addchild($$, $3);
             addchild($$, $5 != NULL ? $5 : newnode(Block, NULL));
             addchild($$, newnode(Block, NULL)); // Else vazio obrigatório
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
         }
         | RETURN Expr SEMICOLON 
         { 
             $$ = newnode(Return, NULL);
             addchild($$, $2);
         }
         | MethodInvocation SEMICOLON 
         { 
             $$ = $1;
         }
         | Assignment SEMICOLON 
         { 
             $$ = $1;
         }
         | ParseArgs SEMICOLON 
         { 
             $$ = $1;
         }
         | SEMICOLON 
         { 
             $$ = NULL; // Statement vazio
         }
         | PRINT LPAR Expr RPAR SEMICOLON 
         { 
             $$ = newnode(Print, NULL);
             addchild($$, $3);
         }
         | PRINT LPAR STRLIT RPAR SEMICOLON 
         { 
             $$ = newnode(Print, NULL);
             addchild($$, newnode(StrLit, $3));
         }
         | error SEMICOLON 
         { 
             $$ = NULL;
         }
         ;

StatementList: StatementList Statement 
             { 
                 $$ = $1;
                 if ($$ == NULL) $$ = newnode(Program, NULL); // Nó saco
                 if ($2 != NULL) addchild($$, $2);
             }
             | 
             { 
                 $$ = newnode(Program, NULL); // Nó saco
             }
             ;

MethodInvocation: IDENTIFIER LPAR RPAR 
                { 
                    $$ = newnode(Call, NULL);
                    addchild($$, newnode(Identifier, $1));
                }
                | IDENTIFIER LPAR ExprList RPAR 
                { 
                    $$ = newnode(Call, NULL);
                    addchild($$, newnode(Identifier, $1));
                    if ($3 != NULL) {
                        struct node_list *child = $3->children;
                        while (child != NULL) {
                            addchild($$, child->node);
                            child = child->next;
                        }
                        free($3);
                    }
                }
                ;

Assignment: IDENTIFIER ASSIGN Expr 
          { 
              $$ = newnode(Assign, NULL);
              addchild($$, newnode(Identifier, $1));
              addchild($$, $3);
          }
          ;

ParseArgs: PARSEINT LPAR IDENTIFIER LSQ Expr RSQ RPAR 
         { 
             $$ = newnode(ParseArgs, NULL);
             addchild($$, newnode(Identifier, $3));
             addchild($$, $5);
         }
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

ExprOr: ExprOr OR ExprAnd { $$ = newnode(Or, NULL); addchild($$, $1); addchild($$, $3); }
      | ExprAnd           { $$ = $1; }
      ;

ExprAnd: ExprAnd AND ExprXor { $$ = newnode(And, NULL); addchild($$, $1); addchild($$, $3); }
       | ExprXor             { $$ = $1; }
       ;

ExprXor: ExprXor XOR ExprEq { $$ = newnode(Xor, NULL); addchild($$, $1); addchild($$, $3); }
       | ExprEq             { $$ = $1; }
       ;

ExprEq: ExprEq EQ ExprRel { $$ = newnode(Eq, NULL); addchild($$, $1); addchild($$, $3); }
      | ExprEq NE ExprRel { $$ = newnode(Ne, NULL); addchild($$, $1); addchild($$, $3); }
      | ExprRel           { $$ = $1; }
      ;

ExprRel: ExprRel LT ExprShift { $$ = newnode(Lt, NULL); addchild($$, $1); addchild($$, $3); }
       | ExprRel GT ExprShift { $$ = newnode(Gt, NULL); addchild($$, $1); addchild($$, $3); }
       | ExprRel LE ExprShift { $$ = newnode(Le, NULL); addchild($$, $1); addchild($$, $3); }
       | ExprRel GE ExprShift { $$ = newnode(Ge, NULL); addchild($$, $1); addchild($$, $3); }
       | ExprShift            { $$ = $1; }
       ;

ExprShift: ExprShift LSHIFT ExprAdd { $$ = newnode(Lshift, NULL); addchild($$, $1); addchild($$, $3); }
         | ExprShift RSHIFT ExprAdd { $$ = newnode(Rshift, NULL); addchild($$, $1); addchild($$, $3); }
         | ExprAdd                  { $$ = $1; }
         ;

ExprAdd: ExprAdd PLUS ExprMult  { $$ = newnode(Add, NULL); addchild($$, $1); addchild($$, $3); }
       | ExprAdd MINUS ExprMult { $$ = newnode(Sub, NULL); addchild($$, $1); addchild($$, $3); }
       | ExprMult               { $$ = $1; }
       ;

ExprMult: ExprMult STAR ExprUnary { $$ = newnode(Mul, NULL); addchild($$, $1); addchild($$, $3); }
        | ExprMult DIV ExprUnary  { $$ = newnode(Div, NULL); addchild($$, $1); addchild($$, $3); }
        | ExprMult MOD ExprUnary  { $$ = newnode(Mod, NULL); addchild($$, $1); addchild($$, $3); }
        | ExprUnary               { $$ = $1; }
        ;

ExprUnary: PLUS ExprUnary  { $$ = newnode(Plus, NULL); addchild($$, $2); }
         | MINUS ExprUnary { $$ = newnode(Minus, NULL); addchild($$, $2); }
         | NOT ExprUnary   { $$ = newnode(Not, NULL); addchild($$, $2); }
         | ExprPostfix     { $$ = $1; }
         ;

ExprPostfix: MethodInvocation      { $$ = $1; }
           | ParseArgs             { $$ = $1; }
           | IDENTIFIER DOTLENGTH  { $$ = newnode(Length, NULL); addchild($$, newnode(Identifier, $1)); }
           | IDENTIFIER            { $$ = newnode(Identifier, $1); }
           | NATURAL               { $$ = newnode(Natural, $1); }
           | DECIMAL               { $$ = newnode(Decimal, $1); }
           | BOOLLIT               { $$ = newnode(BoolLit, $1); }
           | LPAR Expr RPAR        { $$ = $2; }
           | LPAR error RPAR       { $$ = NULL; }
           ;

%%

struct node *newnode(enum category category, char *token) {
    struct node *new = malloc(sizeof(struct node));
    new->category = category;
    new->token = token;
    new->children = malloc(sizeof(struct node_list));
    new->children = NULL;
    return new;
}

void addchild(struct node *parent, struct node *child) {
    struct node_list *new = malloc(sizeof(struct node_list));
    new->node = child;
    new->next = NULL;

    if (parent->children == NULL) {
        parent->children = new;
    } else {
        struct node_list *children = parent->children;
        while(children->next != NULL)
            children = children->next;
        children->next = new;
    }
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

void show(struct node *node, int depth){
    if (node == NULL){
        return;
    }
    for (int i = 0; i < depth; i++){
        printf("..");
    }
    if (node->token != NULL) {
        if (node->category == StrLit) {
            printf("%s(\"%s\")\n", category_names[node->category], node->token);
        } else {
            printf("%s(%s)\n", category_names[node->category], node->token);
        }
    } else {
        printf("%s\n", category_names[node->category]);
    }
    struct node_list *current_child = node->children;
    while (current_child != NULL) {
        show(current_child->node, depth + 1);
        current_child = current_child->next;
    }
}

extern int verbose;
int syntax_errors = 0; 

int main(int argc, char *argv[]) {
    int print_ast = 0;
    if (argc > 1) {
        if (strcmp(argv[1], "-l") == 0) {
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
        }
    }
    yyparse();
    if (print_ast && syntax_errors == 0) {
        show(ast, 0);
    }
    return 0;
}

void yyerror(char *error) {
    syntax_errors++;
    printf("Line %d, col %d: %s: %s\n", error_line, error_col, error, error_yytext);
}