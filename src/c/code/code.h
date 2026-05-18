#ifndef CODE_H
#define CODE_H

#include "../scope.h"
#include "../symbol.h"

void code_init(symbol_table_t* symbols, scope_t** scope);
void code_reset_registers();

void code_add_program_start();
void begin_main_method();
void begin_function_definition(char* name);
void end_function_definition();
void code_clear_function_parameters();
void code_add_function_parameter(char* name);
void code_begin_function_arguments();
void code_add_function_argument(long expression_register);
void code_discard_expression(long expression_register);
void code_add_return_statement(long expression_register);
long code_add_function_call(char* name);

long code_add_constant_load(long value);
long code_add_variable_load(char* name);
long code_add_binary_expression(opcode_t opcode, long left, long right);
long code_add_unary_expression(opcode_t opcode, long reg);
long code_add_variable_address(char* name);
long code_add_pointer_load(long address_register);
void code_add_pointer_store(long address_register, long value_register);
void code_add_variable_declaration(char* name, long expression_address);
void code_add_empty_variable_declaration(char* name);
void code_add_variable_assignment(char* name, long expression_address);
void code_add_print_statement(long expression_address);

instruction_t* code_add_conditional_jump_placeholder(long condition_address);
instruction_t* code_add_jump_placeholder();
void patch_jmf_relative(instruction_t* jmf, long offset);
void patch_jmp_relative(instruction_t* jmp, long offset);

void begin_block();
long end_block();
void end_main_method();

instruction_t* current_last_instruction();
instruction_t* first_instruction_after(instruction_t* instruction);
void code_add_loop_back_jump(instruction_t* target);

#endif
