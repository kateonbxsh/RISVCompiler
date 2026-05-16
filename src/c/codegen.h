#ifndef CODEGEN_H
#define CODEGEN_H

#include "scope.h"
#include "symbol.h"

void codegen_init(symbol_table_t* symbols, scope_t** scope);
void codegen_reset_registers(void);

void emit_program_start(void);
void begin_main_method(void);
void begin_function_definition(char* name);
void end_function_definition(void);
void emit_return(long expression_register);
long emit_function_call(char* name);

long emit_number(long value);
long emit_identifier(char* name);
long emit_binary_expression(opcode_t opcode, long left, long right);
long emit_unary_expression(opcode_t opcode, long reg);
long emit_address_of(char* name);
long emit_pointer_load(long address_register);
void emit_pointer_assignment(long address_register, long value_register);
void emit_variable_declaration(char* name, long expression_address);
void emit_variable_assignment(char* name, long expression_address);
void emit_print(long expression_address);

instruction_t* emit_jmf_placeholder(long condition_address);
instruction_t* emit_jmp_placeholder(void);
void patch_jmf_relative(instruction_t* jmf, long offset);
void patch_jmp_relative(instruction_t* jmp, long offset);

void begin_block(void);
long end_block(void);
void end_main_method();

instruction_t* current_last_instruction(void);
instruction_t* first_instruction_after(instruction_t* instruction);
void emit_loop_back_jump(instruction_t* target);

#endif
