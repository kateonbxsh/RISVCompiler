%{
#include <stdlib.h>
#include <stdio.h>
#include "symbol.h"

void yyerror(char *s);
FILE* out;
symbol_table_t symbol_table;

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

Expression : Expression tPLUS Expression {
                if (symbol_is_temporary(&symbol_table, $3)) {
                    symbol_table_pop_temporary(&symbol_table);
                }
                if (symbol_is_temporary(&symbol_table, $1)) {
                    symbol_table_pop_temporary(&symbol_table);
                }
                int addr = symbol_table_push_temporary(&symbol_table);
                fprintf(out, "ADD %d %d %d\n", addr, $1, $3);
                $$ = addr;
            }
           | tNUMBER {
                int addr = symbol_table_push_temporary(&symbol_table);
                fprintf(out, "AFC %d %d\n", addr, $1);
                $$ = addr;
            }
           | tIDENTIFIER { $$ = symbol_get_address(&symbol_table, $1); };

MultivariableDeclaration : tIDENTIFIER tCOMMA MultivariableDeclaration { symbol_table_add(&symbol_table, $1); }
                           | tIDENTIFIER { symbol_table_add(&symbol_table, $1); };

VarDeclaration : tINT tIDENTIFIER tEQUAL Expression tSEMICOLON
                { 
                    if (symbol_is_temporary(&symbol_table, $4)) {
                        symbol_table_pop_temporary(&symbol_table);
                    }
                    int addr = symbol_table_add(&symbol_table, $2);
                    if (addr != $4) fprintf(out, "COP %d %d\n", addr, $4);
                }
                | tINT MultivariableDeclaration tSEMICOLON;

VarAffectation : tIDENTIFIER tEQUAL Expression tSEMICOLON
                { 
                    if (symbol_is_temporary(&symbol_table, $3)) {
                        symbol_table_pop_temporary(&symbol_table);
                    }
                    int addr = symbol_get_address(&symbol_table, $1);
                    fprintf(out, "COP %d %d\n", addr, $3);
                };

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

    symbol_table_init(&symbol_table);
    out = fopen("build/out.s", "w+");
    if (out == NULL) {
        yyerror("Could not write to assembly file");
        return 1;
    }
    yyparse();

    return 0;

}