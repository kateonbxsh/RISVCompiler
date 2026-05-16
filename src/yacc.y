%{
#include <stdlib.h>
#include <stdio.h>
#include "symbol.h"
#include "scope.h"
#include "codegen.h"

void yyerror(char *s);
int yylex(void);
FILE* out;
FILE* binary_out;
symbol_table_t symbol_table;
scope_t* current_scope;

%}
%union { long nb; char* name; }
%token tLET
%token tMAIN tFUNCTION tRETURN tPRINTF tCONST tERROR tIF tELSE tWHILE
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

Expression : Expression tOR Expression { $$ = code_add_binary_expression(OP_OR, $1, $3); }
           | Expression tAND Expression { $$ = code_add_binary_expression(OP_AND, $1, $3); }
           | Expression tGT Expression { $$ = code_add_binary_expression(OP_GT, $1, $3); }
           | Expression tLT Expression { $$ = code_add_binary_expression(OP_LT, $1, $3); }
           | Expression tGEQ Expression { $$ = code_add_binary_expression(OP_GEQ, $1, $3); }
           | Expression tLEQ Expression { $$ = code_add_binary_expression(OP_LEQ, $1, $3); }
           | Expression tEQUALS Expression { $$ = code_add_binary_expression(OP_EQ, $1, $3); }
           | Expression tNEQ Expression { $$ = code_add_binary_expression(OP_NEQ, $1, $3); }
           | Expression tDIVIDE Expression { $$ = code_add_binary_expression(OP_DIV, $1, $3); }
           | Expression tMINUS Expression { $$ = code_add_binary_expression(OP_SOU, $1, $3); }
           | Expression tTIMES Expression { $$ = code_add_binary_expression(OP_MUL, $1, $3); }
           | Expression tPLUS Expression { $$ = code_add_binary_expression(OP_ADD, $1, $3); }
           | tNOT Expression { $$ = code_add_unary_expression(OP_NOT, $2); }
           | tADDRESS tIDENTIFIER { $$ = code_add_variable_address($2); }
           | tTIMES Expression %prec tDEREF { $$ = code_add_pointer_load($2); }
           | tLEFTPAREN Expression tRIGHTPAREN { $$ = $2; }
           | tIDENTIFIER tLEFTPAREN tRIGHTPAREN { $$ = code_add_function_call($1); }
           | tNUMBER {
                $$ = code_add_constant_load($1);
            }
           | tIDENTIFIER { $$ = code_add_variable_load($1); };

PointerTarget : tTIMES PointerOperand %prec tDEREF { $$ = $2; };

PointerOperand : tIDENTIFIER { $$ = code_add_variable_load($1); }
               | tLEFTPAREN Expression tRIGHTPAREN { $$ = $2; }
               | tTIMES PointerOperand %prec tDEREF { $$ = code_add_pointer_load($2); };

MultivariableDeclaration : tIDENTIFIER tCOMMA MultivariableDeclaration { code_add_empty_variable_declaration($1); }
                           | tIDENTIFIER { code_add_empty_variable_declaration($1); };

VarDeclaration : tLET tIDENTIFIER tAFFECT Expression tSEMICOLON
                { 
                    code_add_variable_declaration($2, $4);
                }
                | tLET MultivariableDeclaration tSEMICOLON;

VarAffectation : tIDENTIFIER tAFFECT Expression tSEMICOLON
                { 
                    code_add_variable_assignment($1, $3);
                };

PointerAffectation : PointerTarget tAFFECT Expression tSEMICOLON
                {
                    code_add_pointer_store($1, $3);
                };

ReturnStatement : tRETURN Expression tSEMICOLON
                {
                    code_add_return_statement($2);
                };

Printf : tPRINTF tLEFTPAREN Expression tRIGHTPAREN tSEMICOLON
        { 
            code_add_print_statement($3);
        };

IfCondition: tIF tLEFTPAREN Expression tRIGHTPAREN {
          $$ = (long) code_add_conditional_jump_placeholder($3);
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
        $<nb>$ = (long) code_add_jump_placeholder();
    } IfStatement {
        patch_jmp_relative((instruction_t*) $<nb>2, $3 + 1);
        $$ = $1 + $3 + 1;
    }
    | TakenIfStatement {
        $<nb>$ = (long) code_add_jump_placeholder();
    } Block {
        patch_jmp_relative((instruction_t*) $<nb>2, $3 + 1);
        $$ = $1 + $3 + 1;
    }
    | SingleIfStatement {
        $$ = $1;
    };

WhileStatement : tWHILE tLEFTPAREN {
        $<nb>$ = (long) current_last_instruction();
        code_reset_registers();
    } Expression tRIGHTPAREN {
        $<nb>$ = (long) code_add_conditional_jump_placeholder($4);
    } Block {
        instruction_t* starting_point = first_instruction_after((instruction_t*) $<nb>3);
        patch_jmf_relative((instruction_t*) $<nb>6, $7 + 2);
        code_add_loop_back_jump(starting_point);
    };

Statement
    : Block
    | VarDeclaration
    | VarAffectation
    | PointerAffectation
    | ReturnStatement
    | Printf
    | IfStatement
    | WhileStatement
;

StatementList
    :
    | Statement StatementList
;

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

FunctionList
    :
    | FunctionList FunctionDefinition
;

GlobalDeclarationList
    :
    | GlobalDeclarationList VarDeclaration
;

FunctionDefinition : tFUNCTION tIDENTIFIER tLEFTPAREN tRIGHTPAREN
    {
        begin_function_definition($2);
    }
    Block
    {
        end_function_definition();
    }
;

ProgramStart :
    {
        code_add_program_start();
    }
;

MainMethod : tMAIN
    {
        begin_main_method();
    }
    Block {
    end_main_method();
} ;

Program : GlobalDeclarationList ProgramStart FunctionList MainMethod { printf("found main\n"); };

%%
void yyerror(char *s) { fprintf(stderr, "%s\n", s); }

int main() {

    current_scope = scope_init();
    symbol_table_init(&symbol_table);
    code_init(&symbol_table, &current_scope);
    out = fopen("build/out.s", "w+");
    if (out == NULL) {
        yyerror("Could not write to assembly file");
        return 1;
    }
    binary_out = fopen("build/out.bin", "wb+");
    if (binary_out == NULL) {
        yyerror("Could not write to binary file");
        fclose(out);
        return 1;
    }
    yyparse();
    scope_resolve_references(current_scope);
    scope_write_instructions(out, current_scope);
    scope_write_binary(binary_out, current_scope);
    fclose(out);
    fclose(binary_out);

    return 0;

}
