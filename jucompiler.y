%{

#include <stdio.h>
#include "ast.h"
int yylex(void);
void yyerror(char *);

struct node *ast;

extern int cur_line, cur_col;
extern char *yytext;

%}

%token AND ASSIGN STAR COMMA DIV EQ GE GT LBRACE LE LPAR LSQ LT MINUS MOD NE NOT OR PLUS RBRACE RPAR RSQ SEMICOLON ARROW LSHIFT RSHIFT XOR BOOL CLASS DOTLENGTH DOUBLE ELSE IF INT PRINT PARSEINT PUBLIC RETURN STATIC STRING VOID WHILE RESERVED
%token <lexeme> IDENTIFIER NATURAL DECIMAL STRLIT BOOLLIT
%type <node> Program MethodDecl FieldDecl Type MethodHeader MethodParams ParamDecl MethodBody VarDecl Statement Expr MethodInvocation Assignment ParseArgs

%union{
    char *lexeme;
    struct node *node;
}

%left OR
%left AND
%left XOR
%left EQ NE
%left LT GT LE GE
%left LSHIFT RSHIFT
%left PLUS MINUS
%left STAR DIV MOD
%right NOT PLUS PLUS MINUS MINUS
%left LPAR RPAR LSQ RSQ DOTLENGTH
%nonassoc ELSE

/* START grammar rules section -- BNF grammar */

%%








%%


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
        printf("%s (%s)\n", category_names[node->category], node->token);
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
            while (yylex() != 0); // Corre só o Lexer e imprime tokens
            return 0;
        } 
        else if (strcmp(argv[1], "-e1") == 0) {
            verbose = 0;
            while (yylex() != 0); // Corre só o Lexer (erros saem automaticamente)
            return 0;
        } 
        else if (strcmp(argv[1], "-e2") == 0) {
            yyparse(); // Corre o sintático, imprime só erros
            return 0;
        } 
        else if (strcmp(argv[1], "-t") == 0) {
            print_ast = 1;
        }
    }

    // Comportamento para "-t" ou sem flags
    yyparse();

    // A árvore só é impressa se foi pedida e se não houver erros de sintaxe!
    if (print_ast && syntax_errors == 0) {
        show(ast, 0);
    }

    return 0;
}

void yyerror(char *error) {
    syntax_errors++;
    printf("Line %d, col %d: %s: %s\n", cur_line, cur_col - (int)strlen(yytext), error, yytext);
}