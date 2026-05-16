#ifndef CODEGEN_H
#define CODEGEN_H

#include "scope.h"
#include "symbol.h"

void codegen_init(symbol_table_t* symbols, scope_t** scope);

long emit_number(long value);
long emit_binary_expression(opcode_t opcode, long left, long right);
void emit_variable_declaration(char* name, long expression_address);
void emit_variable_assignment(char* name, long expression_address);
void emit_print(long expression_address);

instruction_t* emit_jmf_placeholder(long condition_address);
instruction_t* emit_jmp_placeholder();
void patch_jmf_relative(instruction_t* jmf, long offset);
void patch_jmp_relative(instruction_t* jmp, long offset);

void begin_block();
long end_block();
void end_main_method();

instruction_t* current_last_instruction();
instruction_t* first_instruction_after(instruction_t* instruction);
void emit_loop_back_jump(instruction_t* target);

#endif
