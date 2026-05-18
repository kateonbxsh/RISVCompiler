%{
#include <stdlib.h>
#include <stdio.h>
#include "y.tab.h"
#include <string.h>
%}

%option noyywrap

%%
let return tLET;
function return tFUNCTION;
return return tRETURN;
main return tMAIN;
print return tPRINTF;
if return tIF;
else return tELSE;
while return tWHILE;

const return tCONST;

true|false { yylval.nb = strcmp(yytext, "true") == 0 ? 1 : 0; return tNUMBER; }
[_a-zA-Z][_a-zA-Z0-9]* { yylval.name = strdup(yytext); return tIDENTIFIER; };
"!=" return tNEQ;
"&&" return tAND;
"||" return tOR;
"&" return tADDRESS;
"!" return tNOT;
"=" return tAFFECT;
"+" return tPLUS;
"-" return tMINUS;
"*" return tTIMES;
"==" return tEQUALS;
">=" return tGEQ;
"<=" return tLEQ;
">" return tGT;
"<" return tLT;
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
