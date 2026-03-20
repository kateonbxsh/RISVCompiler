%{
#include <stdlib.h>
#include <stdio.h>
%}

%option noyywrap

%%
int printf("found tINT\n");
main printf("found tMAIN\n");
printf printf("found tPRINTF\n");

return printf("found tRETURN\n");
const printf("found tCONST\n");

"-"?[0-9] printf("found tNUMBER\n");
[1-9]+"e"?[0-9]+ printf("found tNUMBEREXP\n");

[_a-zA-Z][_a-zA-Z0-9]* printf("found tIDENTIFIER %s\n", yytext);
"=" printf("found tEQUAL\n");
"+" printf("found tPLUS\n");
"-" printf("found tMINUS\n");
"*" printf("found tTIMES\n");
"/" printf("found tDIVIDE\n");
"!" printf("found tNOT\n");
"(" printf("found tLEFTPAREN\n");
")" printf("found tRIGHTPAREN\n");
"{" printf("found tLEFTBRACE\n");
"}" printf("found tRIGHTBRACE\n");
";" printf("found tSEMICOLON\n");
"," printf("found tCOMMA\n");
"\n" printf("found tNEWLINE\n");

[ \t]+ printf("found %d length tab or whitespace\n", yyleng);

. printf("found unknown token %s\n", yytext);

%%
int main() {
    yylex();
}