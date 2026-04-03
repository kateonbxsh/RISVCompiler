%{
#include <stdlib.h>
#include <stdio.h>
#include "symbol.h"
#include "scope.h"

void yyerror(char *s);
FILE* out;
symbol_table_t symbol_table;
scope_t* current_scope;


%}
%union { int nb; char* name; }
%token tINT
%token tMAIN tPRINTF tCONST tERROR tIF tELSE tWHILE
%token <nb> tNUMBER
%token <name> tIDENTIFIER
%token tEQUALS tPLUS tMINUS tTIMES tDIVIDE tGT tLT tGEQ tLEQ
%token tLEFTPAREN tRIGHTPAREN tLEFTBRACE tRIGHTBRACE tSEMICOLON tCOMMA tAFFECT

%type <nb> Expression
%type <nb> Operator

%right tEQUALS
%left tPLUS tMINUS
%left tTIMES tDIVIDE

%start Program
%%

Operator : tGEQ { $$ = 9; }
         | tLEQ { $$ = 8; }
         | tGT { $$ = 7; }
         | tLT { $$ = 6; }
         | tEQUALS { $$ = 5; }
         | tDIVIDE { $$ = 4; }
         | tMINUS { $$ = 3; }
         | tTIMES { $$ = 2; }
         | tPLUS { $$ = 1; }

Expression : Expression Operator Expression {
                if (symbol_is_temporary(&symbol_table, $3)) {
                    symbol_table_pop_temporary(&symbol_table);
                }
                if (symbol_is_temporary(&symbol_table, $1)) {
                    symbol_table_pop_temporary(&symbol_table);
                }
                int addr = symbol_table_push_temporary(&symbol_table);
                switch ($2) {
                    case 1:
                        fprintf(out, "ADD %d %d %d\n", addr, $1, $3);
                        break;
                    case 2:
                        fprintf(out, "MUL %d %d %d\n", addr, $1, $3);
                        break;
                    case 3:
                        fprintf(out, "SOU %d %d %d\n", addr, $1, $3);
                        break;
                    case 4:
                        fprintf(out, "DIV %d %d %d\n", addr, $1, $3);
                        break;
                    case 5:
                        fprintf(out, "EQU %d %d %d\n", addr, $1, $3);
                        break;
                    case 6:
                        fprintf(out, "INF %d %d %d\n", addr, $1, $3);
                        break;
                    case 7:
                        fprintf(out, "SUP %d %d %d\n", addr, $1, $3);
                        break;
                    case 8:
                        fprintf(out, "INF %d %d %d\n", addr, $1, $3);
                        break;
                    case 9:
                        fprintf(out, "SUP %d %d %d\n", addr, $1, $3);
                        break;
                }
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

VarDeclaration : tINT tIDENTIFIER tAFFECT Expression tSEMICOLON
                { 
                    if (symbol_is_temporary(&symbol_table, $4)) {
                        symbol_table_pop_temporary(&symbol_table);
                    }
                    int addr = symbol_table_add(&symbol_table, $2);
                    if (addr != $4) fprintf(out, "COP %d %d\n", addr, $4);
                }
                | tINT MultivariableDeclaration tSEMICOLON;

VarAffectation : tIDENTIFIER tAFFECT Expression tSEMICOLON
                { 
                    if (symbol_is_temporary(&symbol_table, $3)) {
                        symbol_table_pop_temporary(&symbol_table);
                    }
                    int addr = symbol_get_address(&symbol_table, $1);
                    fprintf(out, "COP %d %d\n", addr, $3);
                };

Printf : tPRINTF tLEFTPAREN Expression tRIGHTPAREN tSEMICOLON
        { 
            if (symbol_is_temporary(&symbol_table, $3)) {
                symbol_table_pop_temporary(&symbol_table);
            }
            fprintf(out, "PRI %d\n", $3);
        };

IfCondition : tIF tLEFTPAREN Expression tRIGHTPAREN { 
            if (symbol_is_temporary(&symbol_table, $3)) {
                symbol_table_pop_temporary(&symbol_table);
            }
            fprintf(out, "JMF %d\n", $3);
        };

SingleIfStatement : IfCondition Block;

IfStatement : SingleIfStatement tELSE IfStatement | SingleIfStatement;

WhileStatement : tWHILE tLEFTPAREN Expression tRIGHTPAREN Block;

Statement : VarDeclaration | VarAffectation | Printf | IfStatement | WhileStatement;

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