%{

#include <stdio.h>
#include <ast.h>

int yylex(void);
void yyerror(char *);

struct node *ast;

%}

%token BOOLLIT AND ASSIGN STAR COMMA DIV EQ GE GT LBRACE LE LPAR LSQ LT MINUS MOD NE NOT OR PLUS RBRACE RPAR RSQ SEMICOLON ARROW LSHIFT RSHIFT XOR BOOL CLASS DOTLENGTH DOUBLE ELSE IF INT PRINT PARSEINT PUBLIC RETURN STATIC STRING VOID WHILE RESERVED

%union{
    char *lexeme;
    struct node *node;
}

/* START grammar rules section -- BNF grammar */

%%








%%


const char *category_names[] = {
    "Program",
    "MethodDecl",
    "FieldDecl",
    "Type",
    "MethodHeader",
    "FormalParams",
    "MethodBody",
    "VarDecl",
    "Statement",
    "MethodInvocation",
    "Assigment",
    "ParseArgs",
    "Expr"
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