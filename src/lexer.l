%{
#include <stdlib.h>
#include <stdio.h>
#include "y.tab.h"
%}

%option noyywrap

%%
int return tINT;
main return tMAIN;
printf return tPRINTF;

const return tCONST;

[_a-zA-Z][_a-zA-Z0-9]* { yylval.name = strdup(yytext); return tIDENTIFIER; };
"=" return tEQUAL;
"+" return tPLUS;
"-" return tMINUS;
"*" return tTIMES;
"/" return tDIVIDE;
"(" return tLEFTPAREN;
")" return tRIGHTPAREN;
"{" return tLEFTBRACE;
"}" return tRIGHTBRACE;
";" return tSEMICOLON;
"," return tCOMMA;
"\n"; 

[0-9]+ { yylval.nb = atoi(yytext); return tNUMBER; };
[1-9]+[eE]"+"?[0-9]+ { yylval.nb = atoi(yytext); return tNUMBER; };

[ \t]+ ;

. return tERROR;
