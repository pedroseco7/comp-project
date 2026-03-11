%{
    /* place here any C variables and definitions */
    #include <string.h>
    int cur_line = 1;
    int cur_col = 1;

    int verbose = 0;

    char string_buf[4096];
    int string_buf_len;

    int start_line, start_col = 1;

    #define YY_USER_ACTION \
        for(int i=0; i < yyleng; i++){ \
            if(yytext[i] == '\n'){ \
                cur_line++; \
                cur_col = 0; \
            }else{ \
                cur_col++; \
            }\
        }
%}

%option noyywrap

BOOLLIT                          "true"|"false"
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
RESERVED                        "++"|"--"|"null"|"Integer"|"System"|"abstract"|"assert"|"break"|"byte"|"case"|"catch"|"char"|"const"|"continue"|"default"|"do"|"enum"|"extends"|"final"|"finally"|"float"|"for"|"goto"|"implements"|"import"|"instanceof"|"interface"|"long"|"native"|"new"|"private"|"protected"|"short"|"strictfp"|"super"|"switch"|"synchronized"|"this"|"throw"|"throws"|"transient"|"try"|"volatile"|"_"

digit   [0-9]
letter  [a-zA-Z_$]
whitespace [ \t\r\n\f]
escape [fnrt\"\\]

%x MULTI_COMMENT
%x STRING
%%

{BOOLLIT}                       { if(verbose) printf("BOOLLIT(%s)\n", yytext); }
{AND}                           { if(verbose) printf("AND\n"); }
{ASSIGN}                        { if(verbose) printf("ASSIGN\n"); }
{STAR}                          { if(verbose) printf("STAR\n"); }
{COMMA}                         { if(verbose) printf("COMMA\n"); }
{DIV}                           { if(verbose) printf("DIV\n"); }
{EQ}                            { if(verbose) printf("EQ\n"); }
{GE}                            { if(verbose) printf("GE\n"); }
{GT}                            { if(verbose) printf("GT\n"); }
{LBRACE}                        { if(verbose) printf("LBRACE\n"); }
{LE}                            { if(verbose) printf("LE\n"); }
{LPAR}                          { if(verbose) printf("LPAR\n"); }
{LSQ}                           { if(verbose) printf("LSQ\n"); }
{LT}                            { if(verbose) printf("LT\n"); }
{MINUS}                         { if(verbose) printf("MINUS\n"); }
{MOD}                           { if(verbose) printf("MOD\n"); }
{NE}                            { if(verbose) printf("NE\n"); }
{NOT}                           { if(verbose) printf("NOT\n"); }
{OR}                            { if(verbose) printf("OR\n"); }
{PLUS}                          { if(verbose) printf("PLUS\n"); }
{RBRACE}                        { if(verbose) printf("RBRACE\n"); }
{RPAR}                          { if(verbose) printf("RPAR\n"); }
{RSQ}                           { if(verbose) printf("RSQ\n"); }
{SEMICOLON}                     { if(verbose) printf("SEMICOLON\n"); }
{ARROW}                         { if(verbose) printf("ARROW\n"); }
{LSHIFT}                        { if(verbose) printf("LSHIFT\n"); }
{RSHIFT}                        { if(verbose) printf("RSHIFT\n"); }
{XOR}                           { if(verbose) printf("XOR\n"); }
{BOOL}                          { if(verbose) printf("BOOL\n"); }
{CLASS}                         { if(verbose) printf("CLASS\n"); }
{DOTLENGTH}                     { if(verbose) printf("DOTLENGTH\n"); }
{DOUBLE}                        { if(verbose) printf("DOUBLE\n"); }
{ELSE}                          { if(verbose) printf("ELSE\n"); }
{IF}                            { if(verbose) printf("IF\n"); }
{INT}                           { if(verbose) printf("INT\n"); }
{PRINT}                         { if(verbose) printf("PRINT\n"); }
{PARSEINT}                      { if(verbose) printf("PARSEINT\n"); }
{PUBLIC}                        { if(verbose) printf("PUBLIC\n"); }
{RETURN}                        { if(verbose) printf("RETURN\n"); }
{STATIC}                        { if(verbose) printf("STATIC\n"); }
{STRING}                        { if(verbose) printf("STRING\n"); }
{VOID}                          { if(verbose) printf("VOID\n"); }
{WHILE}                         { if(verbose) printf("WHILE\n"); }
{RESERVED}                      { if(verbose) printf("RESERVED(%s)\n", yytext); }

{whitespace}                    { }
{digit}+                        { if(verbose) printf("NATURAL(%s)\n", yytext); }
{digit}*"."{digit}+             { if(verbose) printf("DECIMAL(%s)\n", yytext); }
{letter}({digit}|{letter})*     { if(verbose) printf("IDENTIFIER(%s)\n", yytext); }

"//".*                          { /* comentário de linha: ignorar tudo até ao fim da linha */ }

"/*"                            { start_line = cur_line; start_col = cur_col - 1; BEGIN(MULTI_COMMENT); }
<MULTI_COMMENT>"*/"             { BEGIN(INITIAL); }
<MULTI_COMMENT>\n               { ; }
<MULTI_COMMENT>.                { ; }
<MULTI_COMMENT><<EOF>>          { printf("Line %d, col %d: unterminated comment\n", start_line, start_col); return 0; }

"\""                            { start_line = cur_line; start_col = cur_col; string_buf_len = 0; string_buf[0] = '\0'; BEGIN(STRING); }
<STRING>"\\"{escape}            { strcat(string_buf, yytext); }
<STRING>"\\".                   { printf("Line %d, col %d: invalid escape sequence (%s)\n", cur_line, cur_col - (int)yyleng + 1, yytext); strcat(string_buf, yytext); }
<STRING>"\""                    { if(verbose) printf("STRLIT(\"%s\")\n", string_buf); BEGIN(INITIAL); }
<STRING>\n                      { printf("Line %d, col %d: unterminated string literal\n", start_line, start_col); BEGIN(INITIAL); }
<STRING><<EOF>>                 { printf("Line %d, col %d: unterminated string literal\n", start_line, start_col); return 0; }
<STRING>.                       { strcat(string_buf, yytext); }

<<EOF>>                         { return 0; }

.                               { printf("Line %d, col %d: illegal character (%s)\n", cur_line, cur_col, yytext); }
%%

extern int yylex();
int main(int argc, char **argv) {
    if (argc > 1) {
        if (strcmp(argv[1], "-l") == 0) {
            verbose = 1;
        } else if (strcmp(argv[1], "-e1") == 0) {
            verbose = 0;  // erros lexicais também precisam de output
        }
    }
    yylex();    /* run the lexical analysis automaton */
    return 0;
}