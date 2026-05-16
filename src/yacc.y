%{
#include <stdlib.h>
#include <stdio.h>
#include "symbol.h"
#include "scope.h"
#include "codegen.h"

void yyerror(char *s);
int yylex(void);
FILE* out;
symbol_table_t symbol_table;
scope_t* current_scope;

%}
%union { long nb; char* name; }
%token tLET
%token tMAIN tPRINTF tCONST tERROR tIF tELSE tWHILE
%token <nb> tNUMBER
%token <name> tIDENTIFIER
%token tEQUALS tNEQ tPLUS tMINUS tTIMES tDIVIDE tGT tLT tGEQ tLEQ
%token tNOT tAND tOR tADDRESS
%token tLEFTPAREN tRIGHTPAREN tLEFTBRACE tRIGHTBRACE tSEMICOLON tCOMMA tAFFECT

%type <nb> Expression
%type <nb> PointerTarget PointerOperand
%type <nb> IfCondition SingleIfStatement Block TakenIfStatement IfStatement

%left tOR
%left tAND
%left tEQUALS tNEQ tGT tLT tGEQ tLEQ
%left tPLUS tMINUS
%left tTIMES tDIVIDE
%right tNOT tADDRESS tDEREF

%start Program
%%

Expression : Expression tOR Expression { $$ = emit_binary_expression(OP_OR, $1, $3); }
           | Expression tAND Expression { $$ = emit_binary_expression(OP_AND, $1, $3); }
           | Expression tGT Expression { $$ = emit_binary_expression(OP_GT, $1, $3); }
           | Expression tLT Expression { $$ = emit_binary_expression(OP_LT, $1, $3); }
           | Expression tGEQ Expression { $$ = emit_binary_expression(OP_GEQ, $1, $3); }
           | Expression tLEQ Expression { $$ = emit_binary_expression(OP_LEQ, $1, $3); }
           | Expression tEQUALS Expression { $$ = emit_binary_expression(OP_EQ, $1, $3); }
           | Expression tNEQ Expression { $$ = emit_binary_expression(OP_NEQ, $1, $3); }
           | Expression tDIVIDE Expression { $$ = emit_binary_expression(OP_DIV, $1, $3); }
           | Expression tMINUS Expression { $$ = emit_binary_expression(OP_SOU, $1, $3); }
           | Expression tTIMES Expression { $$ = emit_binary_expression(OP_MUL, $1, $3); }
           | Expression tPLUS Expression { $$ = emit_binary_expression(OP_ADD, $1, $3); }
           | tNOT Expression { $$ = emit_unary_expression(OP_NOT, $2); }
           | tADDRESS tIDENTIFIER { $$ = emit_address_of($2); }
           | tTIMES Expression %prec tDEREF { $$ = emit_pointer_load($2); }
           | tLEFTPAREN Expression tRIGHTPAREN { $$ = $2; }
           | tNUMBER {
                $$ = emit_number($1);
            }
           | tIDENTIFIER { $$ = emit_identifier($1); };

PointerTarget : tTIMES PointerOperand %prec tDEREF { $$ = $2; };

PointerOperand : tIDENTIFIER { $$ = emit_identifier($1); }
               | tLEFTPAREN Expression tRIGHTPAREN { $$ = $2; }
               | tTIMES PointerOperand %prec tDEREF { $$ = emit_pointer_load($2); };

MultivariableDeclaration : tIDENTIFIER tCOMMA MultivariableDeclaration { symbol_table_add(&symbol_table, $1); }
                           | tIDENTIFIER { symbol_table_add(&symbol_table, $1); };

VarDeclaration : tLET tIDENTIFIER tAFFECT Expression tSEMICOLON
                { 
                    emit_variable_declaration($2, $4);
                }
                | tLET MultivariableDeclaration tSEMICOLON;

VarAffectation : tIDENTIFIER tAFFECT Expression tSEMICOLON
                { 
                    emit_variable_assignment($1, $3);
                };

PointerAffectation : PointerTarget tAFFECT Expression tSEMICOLON
                {
                    emit_pointer_assignment($1, $3);
                };

Printf : tPRINTF tLEFTPAREN Expression tRIGHTPAREN tSEMICOLON
        { 
            emit_print($3);
        };

IfCondition: tIF tLEFTPAREN Expression tRIGHTPAREN {
          $$ = (long) emit_jmf_placeholder($3);
      };

SingleIfStatement : IfCondition Block {
    patch_jmf_relative((instruction_t*) $1, $2 + 1);
    $$ = $2 + 1;
}

TakenIfStatement : IfCondition Block tELSE {
    patch_jmf_relative((instruction_t*) $1, $2 + 2);
    $$ = $2 + 1;
}

IfStatement: TakenIfStatement {
        $<nb>$ = (long) emit_jmp_placeholder();
    } IfStatement {
        patch_jmp_relative((instruction_t*) $<nb>2, $3 + 1);
        $$ = $1 + $3 + 1;
    }
    | TakenIfStatement {
        $<nb>$ = (long) emit_jmp_placeholder();
    } Block {
        patch_jmp_relative((instruction_t*) $<nb>2, $3 + 1);
        $$ = $1 + $3 + 1;
    }
    | SingleIfStatement {
        $$ = $1;
    };

WhileStatement : tWHILE tLEFTPAREN {
        $<nb>$ = (long) current_last_instruction();
        codegen_reset_registers();
    } Expression tRIGHTPAREN {
        $<nb>$ = (long) emit_jmf_placeholder($4);
    } Block {
        instruction_t* starting_point = first_instruction_after((instruction_t*) $<nb>3);
        patch_jmf_relative((instruction_t*) $<nb>6, $7 + 2);
        emit_loop_back_jump(starting_point);
    };

Statement
    : Block
    | VarDeclaration
    | VarAffectation
    | PointerAffectation
    | Printf
    | IfStatement
    | WhileStatement
;

StatementList : Statement | Statement StatementList;

Block: tLEFTBRACE
      {
          begin_block();
      }
      StatementList
      tRIGHTBRACE
      {
          $$ = end_block();
      }
;

MainMethod : tMAIN Block {
    end_main_method();
} ;

Program : MainMethod { printf("found main\n"); };

%%
void yyerror(char *s) { fprintf(stderr, "%s\n", s); }

int main() {

    current_scope = scope_init();
    symbol_table_init(&symbol_table);
    codegen_init(&symbol_table, &current_scope);
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
