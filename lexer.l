%{
    /* place here any C variables and definitions */
    #include <string.h>
    int cur_line = 1;
    int cur_col = 0;

    char string_buf[4096];
    int string_buf_len;

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

BOOLIT                          "true"|"false"
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
RESERVED                        "++"|"--"|"null"|"Integer"|"System"|"abstract"|"assert"|"break"|"byte"|"case"|"catch"|"char"|"const"|"continue"|"default"|"do"|"enum"|"extends"|"final"|"finally"|"float"|"for"|"goto"|"implements"|"import"|"instanceof"|"interface"|"long"|"native"|"new"|"package"|"private"|"protected"|"short"|"strictfp"|"super"|"switch"|"synchronized"|"this"|"throw"|"throws"|"transient"|"try"|"volatile"

digit   [0-9]
letter  [a-zA-Z_$]
whitespace [ \t\n]
escape [fnrt\"\\]

%x COMMENT
%x STRING
%%
{BOOLIT}                        { printf("BOOLIT(%s)\n", yytext); }
{AND}                           { printf("AND\n"); }
{ASSIGN}                        { printf("ASSIGN\n"); }
{STAR}                          { printf("STAR\n"); }
{COMMA}                         { printf("COMMA\n"); }
{DIV}                           { printf("DIV\n"); }
{EQ}                            { printf("EQ\n"); }
{GE}                            { printf("GE\n"); }
{GT}                            { printf("GT\n"); }
{LBRACE}                        { printf("LBRACE\n"); }
{LE}                            { printf("LE\n"); }
{LPAR}                          { printf("LPAR\n"); }
{LSQ}                           { printf("LSQ\n"); }
{LT}                            { printf("LT\n"); }
{MINUS}                         { printf("MINUS\n"); }
{MOD}                           { printf("MOD\n"); }
{NE}                            { printf("NE\n"); }
{NOT}                           { printf("NOT\n"); }
{OR}                            { printf("OR\n"); }
{PLUS}                          { printf("PLUS\n"); }
{RBRACE}                        { printf("RBRACE\n"); }
{RPAR}                          { printf("RPAR\n"); }
{RSQ}                           { printf("RSQ\n"); }
{SEMICOLON}                     { printf("SEMICOLON\n"); }
{ARROW}                         { printf("ARROW\n"); }
{LSHIFT}                        { printf("LSHIFT\n"); }
{RSHIFT}                        { printf("RSHIFT\n"); }
{XOR}                           { printf("XOR\n"); }
{BOOL}                          { printf("BOOL\n"); }
{CLASS}                         { printf("CLASS\n"); }
{DOTLENGTH}                     { printf("DOTLENGTH\n"); }
{DOUBLE}                        { printf("DOUBLE\n"); }
{ELSE}                          { printf("ELSE\n"); }
{IF}                            { printf("IF\n"); }
{INT}                           { printf("INT\n"); }
{PRINT}                         { printf("PRINT\n"); }
{PARSEINT}                      { printf("PARSEINT\n"); }
{PUBLIC}                        { printf("PUBLIC\n"); }
{RETURN}                        { printf("RETURN\n"); }
{STATIC}                        { printf("STATIC\n"); }
{STRING}                        { printf("STRING\n"); }
{VOID}                          { printf("VOID\n"); }
{WHILE}                         { printf("WHILE\n"); }
{RESERVED}                      { printf("RESERVED(%s)\n", yytext); }

{whitespace}                    { }
{digit}+                        { printf("NATURAL(%s)\n", yytext); }
{digit}*"."{digit}+             { printf("DECIMAL(%s)\n", yytext); }
{letter}({digit}|{letter})*     { printf("IDENTIFIER(%s)\n", yytext); }

"/*"                            { BEGIN(COMMENT); }
<COMMENT>\n                     { ; } //newlines são ignorados
<COMMENT>.                      { ; } //todos os outros caracteres são ignorados
<COMMENT>"*/"                   { BEGIN(INITIAL); }

"\""                            { string_buf_len = 0; string_buf[0] = '\0'; BEGIN(STRING); }
<STRING>"\\"{escape}            { strcat(string_buf, yytext); }
<STRING>"\\".                   { printf("Line %d, column %d: invalid escape sequence (%s)\n", cur_line, cur_col - (int)yyleng + 1, yytext); }
<STRING>"\""                    { printf("STRLIT(\"%s\")\n", string_buf); BEGIN(INITIAL); }
<STRING>\n                      { printf("Line %d, column %d: unterminated string literal\n", cur_line, cur_col); BEGIN(INITIAL); }
<STRING>.                       { strcat(string_buf, yytext); }

.                               { printf("ERROR: line %d, column %d -> unrecognizable token '%s'\n", cur_line, cur_col, yytext); }
%%

extern int yylex();
int main() {
    yylex();    /* run the lexical analysis automaton */
    return 0;
}
int yywrap() {  /* called on EOF, return 1 to terminate */
    return 1;
}