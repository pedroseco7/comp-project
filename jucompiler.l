%{
    struct node;
    #include <string.h>
    #include "y.tab.h"
    
    int cur_line = 1;
    int cur_col = 1;
    int start_line = 1;
    int start_col = 1;
    
    int verbose = 0;
    int string_with_error = 0;
    
    char string_buf[65536];
    int error_line = 1;
    int error_col = 1;
    char error_yytext[70000] = "";

    #define YY_USER_ACTION { \
        error_line = cur_line; \
        error_col = cur_col; \
        cur_col += yyleng; \
        strncpy(error_yytext, yytext, 69999); \
        error_yytext[69999] = '\0'; \
        yylval.info.id = NULL; \
        yylval.info.line = error_line; \
        yylval.info.col = error_col; \
    }

    #define PASS_TOKEN() ;
%}

%option noyywrap

IGNORE                          [ \t\f]
NEWLINE                         "\r\n"|"\n"|"\r" 

digit                           [0-9]
DigitSeq                        [0-9]([0-9_]*[0-9])?
EXPONENT                        [eE][+-]?{DigitSeq}+

letter                          [a-zA-Z_$]
escape                          [fnrt\"\\]

BOOLLIT                         "true"|"false"
AND                             "&&"
ASSIGN                          "="
STAR                            "*"
COMMA                           ","
DIV                             "/"
EQ                              "=="
GE                              ">="
GT                              ">"
LBRACE                          "{"
LE                              "<="
LPAR                            "("
LSQ                             "["  
LT                              "<"
MINUS                           "-"
MOD                             "%"
NE                              "!="
NOT                             "!"   
OR                              "||"
PLUS                            "+"
RBRACE                          "}"
RPAR                            ")"
RSQ                             "]"
SEMICOLON                       ";"
ARROW                           "->"
LSHIFT                          "<<"
RSHIFT                          ">>"
XOR                             "^"
BOOL                            "boolean"
CLASS                           "class"
DOTLENGTH                       ".length"
DOUBLE                          "double"
ELSE                            "else"
IF                              "if"
INT                             "int"
PRINT                           "System.out.print"
PARSEINT                        "Integer.parseInt"
PUBLIC                          "public"
RETURN                          "return"
STATIC                          "static"
STRING                          "String"
VOID                            "void"
WHILE                           "while"
RESERVED                        "++"|"--"|"null"|"Integer"|"System"|"abstract"|"assert"|"break"|"byte"|"case"|"catch"|"char"|"const"|"continue"|"default"|"do"|"enum"|"extends"|"final"|"finally"|"float"|"for"|"goto"|"implements"|"import"|"instanceof"|"interface"|"long"|"native"|"new"|"private"|"protected"|"short"|"strictfp"|"super"|"switch"|"synchronized"|"this"|"throw"|"throws"|"transient"|"try"|"volatile"|"package"

%x MULTI_COMMENT
%x SINGLE_COMMENT
%x STR_STATE
%%

{IGNORE}                        { }

{BOOLLIT}                       { if(verbose) printf("BOOLLIT(%s)\n", yytext); yylval.info.id = strdup(yytext); PASS_TOKEN(); return BOOLLIT; }
{AND}                           { if(verbose) printf("AND\n"); return AND; }
{ASSIGN}                        { if(verbose) printf("ASSIGN\n"); return ASSIGN; }
{STAR}                          { if(verbose) printf("STAR\n"); return STAR; }
{COMMA}                         { if(verbose) printf("COMMA\n"); return COMMA; }
{DIV}                           { if(verbose) printf("DIV\n"); return DIV; }
{EQ}                            { if(verbose) printf("EQ\n"); return EQ; }
{GE}                            { if(verbose) printf("GE\n"); return GE; }
{GT}                            { if(verbose) printf("GT\n"); return GT; }
{LBRACE}                        { if(verbose) printf("LBRACE\n"); return LBRACE; }
{LE}                            { if(verbose) printf("LE\n"); return LE; }
{LPAR}                          { if(verbose) printf("LPAR\n"); return LPAR; }
{LSQ}                           { if(verbose) printf("LSQ\n"); return LSQ; }
{LT}                            { if(verbose) printf("LT\n"); return LT; }
{MINUS}                         { if(verbose) printf("MINUS\n"); return MINUS; }
{MOD}                           { if(verbose) printf("MOD\n"); return MOD; }
{NE}                            { if(verbose) printf("NE\n"); return NE; }
{NOT}                           { if(verbose) printf("NOT\n"); return NOT;}
{OR}                            { if(verbose) printf("OR\n"); return OR; }
{PLUS}                          { if(verbose) printf("PLUS\n"); return PLUS; }
{RBRACE}                        { if(verbose) printf("RBRACE\n"); return RBRACE; }
{RPAR}                          { if(verbose) printf("RPAR\n"); return RPAR; }
{RSQ}                           { if(verbose) printf("RSQ\n"); return RSQ; }
{SEMICOLON}                     { if(verbose) printf("SEMICOLON\n"); return SEMICOLON; }
{ARROW}                         { if(verbose) printf("ARROW\n"); return ARROW; }
{LSHIFT}                        { if(verbose) printf("LSHIFT\n"); return LSHIFT; }
{RSHIFT}                        { if(verbose) printf("RSHIFT\n"); return RSHIFT; }
{XOR}                           { if(verbose) printf("XOR\n"); return XOR;}
{BOOL}                          { if(verbose) printf("BOOL\n"); return BOOL; }
{CLASS}                         { if(verbose) printf("CLASS\n"); return CLASS; }
{DOTLENGTH}                     { if(verbose) printf("DOTLENGTH\n"); return DOTLENGTH; }
{DOUBLE}                        { if(verbose) printf("DOUBLE\n"); return DOUBLE; }
{ELSE}                          { if(verbose) printf("ELSE\n"); return ELSE; }
{IF}                            { if(verbose) printf("IF\n"); return IF; }
{INT}                           { if(verbose) printf("INT\n"); return INT; }
{PRINT}                         { if(verbose) printf("PRINT\n"); return PRINT; }
{PARSEINT}                      { if(verbose) printf("PARSEINT\n"); return PARSEINT; }
{PUBLIC}                        { if(verbose) printf("PUBLIC\n"); return PUBLIC; }
{RETURN}                        { if(verbose) printf("RETURN\n"); return RETURN; }
{STATIC}                        { if(verbose) printf("STATIC\n"); return STATIC; }
{STRING}                        { if(verbose) printf("STRING\n"); return STRING; }
{VOID}                          { if(verbose) printf("VOID\n"); return VOID; }
{WHILE}                         { if(verbose) printf("WHILE\n"); return WHILE;}
{RESERVED}                      { if(verbose) printf("RESERVED(%s)\n", yytext); PASS_TOKEN(); return RESERVED;}

0|[1-9]([0-9_]*[0-9])?          { if(verbose) printf("NATURAL(%s)\n", yytext); yylval.info.id = strdup(yytext); PASS_TOKEN(); return NATURAL; }
{DigitSeq}"."({DigitSeq})?({EXPONENT})? { if(verbose) printf("DECIMAL(%s)\n", yytext); yylval.info.id = strdup(yytext); PASS_TOKEN(); return DECIMAL; }
"."{DigitSeq}({EXPONENT})?      { if(verbose) printf("DECIMAL(%s)\n", yytext); yylval.info.id = strdup(yytext); PASS_TOKEN(); return DECIMAL; }
{DigitSeq}{EXPONENT}            { if(verbose) printf("DECIMAL(%s)\n", yytext); yylval.info.id = strdup(yytext); PASS_TOKEN(); return DECIMAL; }

{letter}({digit}|{letter})* { if(verbose) printf("IDENTIFIER(%s)\n", yytext); yylval.info.id = strdup(yytext); PASS_TOKEN(); return IDENTIFIER; }

{NEWLINE}                       { cur_line++; cur_col = 1; }

"//"                            { start_line = cur_line; start_col = cur_col - yyleng; BEGIN(SINGLE_COMMENT); }
<SINGLE_COMMENT>{NEWLINE}       { cur_line++; cur_col = 1; BEGIN(INITIAL); }
<SINGLE_COMMENT>.               { ; }

"/*"                            { start_line = cur_line; start_col = cur_col - yyleng; BEGIN(MULTI_COMMENT); }
<MULTI_COMMENT>"*/"             { BEGIN(INITIAL); }
<MULTI_COMMENT>{NEWLINE}        { cur_line++; cur_col = 1; }
<MULTI_COMMENT><<EOF>> { 
    printf("Line %d, col %d: unterminated comment\n", start_line, start_col); 
    error_line = cur_line;
    error_col = cur_col;
    strcpy(error_yytext, "");
    return 0; 
}
<MULTI_COMMENT>.                { }

"\""                            { start_line = cur_line; start_col = cur_col - yyleng; string_buf[0] = '\0'; string_with_error = 0; BEGIN(STR_STATE); }
<STR_STATE>"\\"{escape}         { 
    /* Proteção de Buffer na concatenação */
    if (strlen(string_buf) + yyleng < 65535) strcat(string_buf, yytext); 
}
<STR_STATE>"\\"[^\n\r]          { printf("Line %d, col %d: invalid escape sequence (%s)\n", cur_line, cur_col - (int)yyleng, yytext); string_with_error = 1; }
<STR_STATE>"\\"                 { printf("Line %d, col %d: invalid escape sequence (\\)\n", cur_line, cur_col - (int)yyleng); string_with_error = 1; }
<STR_STATE>{NEWLINE}            { printf("Line %d, col %d: unterminated string literal\n", start_line, start_col); cur_line++; cur_col = 1; BEGIN(INITIAL); }
<STR_STATE>\" { 
    BEGIN(INITIAL); 
    if(!string_with_error) {
        if(verbose) printf("STRLIT(\"%s\")\n", string_buf);
        yylval.info.id = strdup(string_buf);
        yylval.info.line = start_line;
        yylval.info.col = start_col;
        error_line = start_line;
        error_col = start_col;
        sprintf(error_yytext, "\"%s\"", string_buf); 
        
        return STRLIT;
    }
}
<STR_STATE><<EOF>>              { printf("Line %d, col %d: unterminated string literal\n", start_line, start_col); return 0; }
<STR_STATE>.                    { 
    /* Proteção de Buffer na concatenação */
    if (strlen(string_buf) + yyleng < 65535) strcat(string_buf, yytext); 
}

.                               { printf("Line %d, col %d: illegal character (%s)\n", cur_line, cur_col - (int)yyleng, yytext); }
<<EOF>> { 
    error_line = cur_line;
    error_col = cur_col;
    strcpy(error_yytext, "");
    return 0; 
}
%%