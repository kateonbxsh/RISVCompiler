#ifndef CODE_INTERNAL_H
#define CODE_INTERNAL_H

#include "code.h"
#include "../functions.h"
#include "../registers.h"

#define MAX_FUNCTION_PARAMETERS 16
#define MAX_CALL_DEPTH 16

// names of parameters in the current function definition
typedef struct {
    char* names[MAX_FUNCTION_PARAMETERS];
    int count;
} function_parameter_list_t;

// registers holding argument values for the function call currently being prepared
typedef struct {
    long registers[MAX_FUNCTION_PARAMETERS];
    int count;
} function_argument_list_t;

extern symbol_table_t* code_symbol_table;
extern scope_t** code_current_scope;
extern instruction_t* main_jump;

extern long current_function_body_start_address;
extern int inside_function;
extern int next_local_offset;

extern function_parameter_list_t current_function_parameters;
extern function_argument_list_t function_call_arguments[MAX_CALL_DEPTH];
extern int function_call_argument_depth;

scope_t* scope();
long current_program_address();
argument_t arg_value(long value);

instruction_t* code_add_one_operand_instruction(opcode_t opcode, long a1);
instruction_t* code_add_2_operand_instruction(opcode_t opcode, long a1, long a2);
instruction_t* code_add_3_operand_instruction(opcode_t opcode, long a1, long a2, long a3);
instruction_t* code_add_label();

void code_add_stack_push();
void code_add_stack_pop();
void code_add_function_prefix();
void code_add_function_suffix();
void code_add_compute_local_address(int offset);
void code_add_parameter_locals();
int code_is_function_argument_register(int reg, function_argument_list_t* arguments);

#endif
