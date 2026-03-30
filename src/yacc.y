%{
#include <stdlib.h>
#include <stdio.h>
void yyerror(char *s);
%}
%union { int nb; char* name; }
%token tINT
%token tMAIN tPRINTF tCONST tERROR
%token <nb> tNUMBER
%token <name> tIDENTIFIER
%token tEQUAL tPLUS tMINUS tTIMES tDIVIDE 
%token tLEFTPAREN tRIGHTPAREN tLEFTBRACE tRIGHTBRACE tSEMICOLON tCOMMA

%type <nb> Expression

%right tEQUAL
%left tPLUS tMINUS
%left tTIMES tDIVIDE

%start Program
%%

Operator : | tDIVIDE { printf("found divide operator\n"); }
         | tTIMES { printf("found times operator\n"); }
         | tMINUS { printf("found minus operator\n"); }
         | tPLUS { printf("found plus operator\n"); };

Expression : Expression tPLUS Expression { printf("found expr + expr\n"); }
           | tNUMBER { printf("found number expression\n"); }
           | tIDENTIFIER { printf("found identifier expression\n"); };


VarDeclaration : tCONST tINT tIDENTIFIER tEQUAL Expression tSEMICOLON
                { printf("found constant declaration\n"); }
               | tINT tIDENTIFIER tEQUAL Expression tSEMICOLON
                { printf("found variable declaration\n"); };

VarAffectation : tIDENTIFIER tEQUAL Expression tSEMICOLON
                { printf("found variable affectation\n"); };

Printf : tPRINTF tLEFTPAREN Expression tRIGHTPAREN tSEMICOLON
        { printf("found printf\n"); };

Statement : VarDeclaration | VarAffectation | Printf;

StatementList : Statement | Statement StatementList;

Block : tLEFTBRACE StatementList tRIGHTBRACE | tLEFTBRACE tRIGHTBRACE;

MainMethod : tMAIN Block;

Program : MainMethod { printf("found main\n"); };

%%
void yyerror(char *s) { fprintf(stderr, "%s\n", s); }
int main() {

    return yyparse();

}