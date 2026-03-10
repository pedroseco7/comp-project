%{
    /* place here any C variables and definitions */
    #include <string.h>
    int cur_line = 1;
    int cur_col = 1;
    int start_line = 1;
    int start_col = 1;
    
    int verbose = 0;
    int string_with_error = 0;

    char string_buf[4096];

    #define YY_USER_ACTION cur_col += yyleng;
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

RESERVED                        "++"|"--"|"null"|"Integer"|"System"|"abstract"|"assert"|"break"|"byte"|"case"|"catch"|"char"|"const"|"continue"|"default"|"do"|"enum"|"extends"|"final"|"finally"|"float"|"for"|"goto"|"implements"|"import"|"instanceof"|"interface"|"long"|"native"|"new"|"private"|"protected"|"short"|"strictfp"|"super"|"switch"|"synchronized"|"this"|"throw"|"throws"|"transient"|"try"|"volatile"


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

0|[1-9]([0-9_]*[0-9])?                  { if(verbose) printf("NATURAL(%s)\n", yytext); }
{DigitSeq}"."({DigitSeq})?({EXPONENT})? { if(verbose) printf("DECIMAL(%s)\n", yytext); }
"."{DigitSeq}({EXPONENT})?              { if(verbose) printf("DECIMAL(%s)\n", yytext); }
{DigitSeq}{EXPONENT}                    { if(verbose) printf("DECIMAL(%s)\n", yytext); }

{letter}({digit}|{letter})*     { if(verbose) printf("IDENTIFIER(%s)\n", yytext); }

{IGNORE}                        { }
{NEWLINE}                       { cur_line++; cur_col = 1; }

"//".*                          { }

"/*"                            { start_line = cur_line; start_col = cur_col - yyleng; BEGIN(MULTI_COMMENT); }
<MULTI_COMMENT>"*/"             { BEGIN(INITIAL); }
<MULTI_COMMENT>{NEWLINE}        { cur_line++; cur_col = 1; }
<MULTI_COMMENT><<EOF>>          { printf("Line %d, col %d: unterminated comment\n", start_line, start_col); return 0; }
<MULTI_COMMENT>.                { }

"\""                            { start_line = cur_line; start_col = cur_col - yyleng; string_buf[0] = '\0'; string_with_error = 0; BEGIN(STRING); }
<STRING>"\\"{escape}            { strcat(string_buf, yytext); }
<STRING>"\\"[^\n\r]             { printf("Line %d, col %d: invalid escape sequence (%s)\n", cur_line, cur_col - (int)yyleng, yytext); string_with_error = 1; }
<STRING>"\\"                    { printf("Line %d, col %d: invalid escape sequence (\\)\n", cur_line, cur_col - (int)yyleng); string_with_error = 1; }
<STRING>{NEWLINE}               { printf("Line %d, col %d: unterminated string literal\n", start_line, start_col); cur_line++; cur_col = 1; BEGIN(INITIAL); }
<STRING>\"                      { if(verbose && !string_with_error) printf("STRLIT(\"%s\")\n", string_buf); BEGIN(INITIAL); }
<STRING><<EOF>>                 { printf("Line %d, col %d: unterminated string literal\n", start_line, start_col); return 0; }
<STRING>.                       { strcat(string_buf, yytext); }

.                               { printf("Line %d, col %d: illegal character (%s)\n", cur_line, cur_col - (int)yyleng, yytext); }
<<EOF>>                         { return 0; }
%%

extern int yylex();
int main(int argc, char **argv) {
    if (argc > 1) {
        if (strcmp(argv[1], "-l") == 0) {
            verbose = 1;
        } else if (strcmp(argv[1], "-e1") == 0) {
            verbose = 0;
        }
    }
    yylex();    /* run the lexical analysis automaton */
    return 0;
}