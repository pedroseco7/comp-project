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

digit   [0-9]
letter  [a-zA-Z_$]
whitespace [ \t\n]
ponctuation [()=,*/+-]
escape [fnrt\"\\]
%x COMMENT
%x STRING
%%
"integer"                       { printf("INTEGER\n"); }
"double"                        { printf("DOUBLE\n"); }
"if"                            { printf("IF\n"); }
"else"                          { printf("ELSE\n"); }
"then"                          { printf("THEN\n"); }
"&&"                            { printf("AND\n"); }

{whitespace}                    { }
{ponctuation}                   { printf("PONCTUATION(%s)\n", yytext); }
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