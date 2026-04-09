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
%union { long nb; char* name; }
%token tINT
%token tMAIN tPRINTF tCONST tERROR tIF tELSE tWHILE
%token <nb> tNUMBER
%token <name> tIDENTIFIER
%token tEQUALS tPLUS tMINUS tTIMES tDIVIDE tGT tLT tGEQ tLEQ
%token tLEFTPAREN tRIGHTPAREN tLEFTBRACE tRIGHTBRACE tSEMICOLON tCOMMA tAFFECT

%type <nb> Expression
%type <nb> Operator, IfCondition, SingleIfStatement, Block, TakenIfStatement, IfStatement

%right tEQUALS
%left tPLUS tMINUS
%left tTIMES tDIVIDE

%start Program
%%

Operator : tGT { $$ = SUP; }
         | tLT { $$ = INF; }
         | tEQUALS { $$ = EQU; }
         | tDIVIDE { $$ = DIV; }
         | tMINUS { $$ = SUB; }
         | tTIMES { $$ = MUL; }
         | tPLUS { $$ = ADD; }

Expression : Expression Operator Expression {
                if (symbol_is_temporary(&symbol_table, $3)) {
                    symbol_table_pop_temporary(&symbol_table);
                }
                if (symbol_is_temporary(&symbol_table, $1)) {
                    symbol_table_pop_temporary(&symbol_table);
                }
                int addr = symbol_table_push_temporary(&symbol_table);
                argument_t a1 = { .value = addr };
                argument_t a2 = { .value = $1 };
                argument_t a3 = { .value = $3 };
                scope_add_instruction(current_scope, i_op3($2, a1, a2, a3));
                $$ = addr;
            }
           | tNUMBER {
                int addr = symbol_table_push_temporary(&symbol_table);
                argument_t a1 = { .value = addr };
                argument_t a2 = { .value = $1 };
                scope_add_instruction(current_scope, i_op2(AFC, a1, a2));
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
                    argument_t a1 = { .value = addr };
                    argument_t a2 = { .value = $4 };
                    if (addr != $4) scope_add_instruction(current_scope, i_op2(COP, a1, a2));
                }
                | tINT MultivariableDeclaration tSEMICOLON;

VarAffectation : tIDENTIFIER tAFFECT Expression tSEMICOLON
                { 
                    if (symbol_is_temporary(&symbol_table, $3)) {
                        symbol_table_pop_temporary(&symbol_table);
                    }
                    int addr = symbol_get_address(&symbol_table, $1);
                    argument_t a1 = { .value = addr };
                    argument_t a2 = { .value = $3 };
                    scope_add_instruction(current_scope, i_op2(COP, a1, a2));
                };

Printf : tPRINTF tLEFTPAREN Expression tRIGHTPAREN tSEMICOLON
        { 
            if (symbol_is_temporary(&symbol_table, $3)) {
                symbol_table_pop_temporary(&symbol_table);
            }
            argument_t a1 = { .value = $3 };
            scope_add_instruction(current_scope, i_op1(PRI, a1));
        };

IfCondition: tIF tLEFTPAREN Expression tRIGHTPAREN {
          if (symbol_is_temporary(&symbol_table, $3))
              symbol_table_pop_temporary(&symbol_table);

          argument_t cond = { .value = $3 };
          argument_t target = {0};

          instruction_t* jmf = i_op2(JMF, cond, target);
          jmf->relative = 1;

          scope_add_instruction(current_scope, jmf);

          $$ = (long) jmf;
      };

SingleIfStatement : IfCondition Block {
    ((instruction_t*) $1)->arguments[1] = (argument_t){$2 + 1};
    $$ = $2 + 1;
}

TakenIfStatement : IfCondition Block tELSE {
    ((instruction_t*) $1)->arguments[1] = (argument_t){$2 + 2};
    $$ = $2 + 1;
}

IfStatement: TakenIfStatement {
        instruction_t* jmp = i_op1(JMP, (argument_t){0});
        jmp->relative = 1;
        scope_add_instruction(current_scope, jmp);
        $<nb>$ = (long)jmp;
    } IfStatement {
        ((instruction_t*) $<nb>2)->arguments[0] = (argument_t){$3 + 1};
        $$ = $1 + $3 + 1;
    }
    | TakenIfStatement {
        instruction_t* jmp = i_op1(JMP, (argument_t){0});
        jmp->relative = 1;
        scope_add_instruction(current_scope, jmp);
        $<nb>$ = (long)jmp;
    } Block {
        ((instruction_t*) $<nb>2)->arguments[0] = (argument_t){$3 + 1};
        $$ = $1 + $3 + 1;
    }
    | SingleIfStatement {
        $$ = $1;
    };

WhileStatement : tWHILE tLEFTPAREN {
        $<nb>$ = (long) current_scope->last;
    } Expression tRIGHTPAREN {
        instruction_t* jmf = i_op2(JMF, (argument_t){$4}, (argument_t){0});
        jmf->relative = 1;
        scope_add_instruction(current_scope, jmf);
        $<nb>$ = (long) jmf;
    } Block {
        instruction_t* starting_point = (instruction_t*) $<nb>3; // first instruction after it
        if (starting_point) starting_point = starting_point->next;
        instruction_t* jmf = (instruction_t*) $<nb>6; // skip loop if false
        jmf->arguments[1].value = $7 + 2;
        instruction_t* jmp = i_op1(JMP, (argument_t){starting_point}); // jump back to test loop again
        jmp->relative = 0;
        scope_add_instruction(current_scope, jmp);
    };

Statement
    : Block
    | VarDeclaration
    | VarAffectation
    | Printf
    | IfStatement
    | WhileStatement
;

StatementList : Statement | Statement StatementList;

Block: tLEFTBRACE
      {
          current_scope = scope_init_child(current_scope, symbol_table.symbol_size);
      }
      StatementList
      tRIGHTBRACE
      {
          $$ = current_scope->instruction_count; 
          current_scope = scope_flush(current_scope);
      }
;

MainMethod : tMAIN Block {
    current_scope = scope_flush(current_scope);
} ;

Program : MainMethod { printf("found main\n"); };

%%
void yyerror(char *s) { fprintf(stderr, "%s\n", s); }
int main() {

    current_scope = scope_init();
    symbol_table_init(&symbol_table);
    out = fopen("build/out.s", "w+");
    if (out == NULL) {
        yyerror("Could not write to assembly file");
        return 1;
    }
    yyparse();
    scope_resolve_references(current_scope);
    scope_write_instructions(out, current_scope);

    return 0;

}