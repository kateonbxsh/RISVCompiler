%{
#include <stdlib.h>
#include <stdio.h>
void yyerror(char *s);
%}
%token tINT
%token tMAIN tPRINTF tCONST tERROR
%token tNUMBER tNUMBEREXP tIDENTIFIER
%token tEQUAL tPLUS tMINUS tTIMES tDIVIDE  
%token tLEFTPAREN tRIGHTPAREN tLEFTBRACE tRIGHTBRACE tSEMICOLON tCOMMA
%start Program
%%

Operator : tPLUS { printf("found plus operator"); }
         | tMINUS { printf("found minus operator"); }
         | tTIMES { printf("found times operator"); }
         | tDIVIDE { printf("found divide operator"); };

Number : tNUMBER { printf("found Number\n"); }
       | tNUMBEREXP { printf("found Number in exponent form\n");  };

Identifier : tIDENTIFIER { printf("found identifier"); };

Expression : Expression Operator Expression { printf("found expr + expr"); }
           | Number { printf("found number expression"); }
           | tIDENTIFIER { printf("found identifier expression"); };


VarDeclaration : tCONST tINT Identifier tEQUAL Expression tSEMICOLON
                { printf("found constant declaration"); }
               | tINT Identifier tEQUAL Expression tSEMICOLON
                { printf("found variable declaration"); };

VarAffectation : tIDENTIFIER tEQUAL Expression tSEMICOLON
                { printf("found variable affectation"); };

Printf : tPRINTF tLEFTPAREN Expression tRIGHTPAREN tSEMICOLON
        { printf("found printf"); };

Statement : VarDeclaration | VarAffectation | Printf;

StatementList : Statement | Statement StatementList;

Block : tLEFTBRACE StatementList tRIGHTBRACE | tLEFTBRACE tRIGHTBRACE;

MainMethod : tINT tMAIN tLEFTPAREN tRIGHTPAREN Block;

Program : MainMethod;

%%
void yyerror(char *s) { fprintf(stderr, "%s\n", s); }
int main() {

    return yyparse();

}