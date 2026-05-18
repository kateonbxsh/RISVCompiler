#include "code_internal.h"

#include <stdio.h>
#include <stdlib.h>

// save the caller state at the start of a function
void code_add_function_prefix() {
    code_add_2_operand_instruction(OP_STORER, REG_SP, REG_RA);
    code_add_stack_push();
    code_add_2_operand_instruction(OP_STORER, REG_SP, REG_FP);
    code_add_stack_push();
    code_add_2_operand_instruction(OP_COP, REG_FP, REG_SP);
}

// restore the caller state and jump back at the end of a function
void code_add_function_suffix() {
    code_add_2_operand_instruction(OP_COP, REG_SP, REG_FP);
    code_add_stack_pop();
    code_add_2_operand_instruction(OP_LOADR, REG_FP, REG_SP);
    code_add_stack_pop();
    code_add_2_operand_instruction(OP_LOADR, REG_RA, REG_SP);
    code_add_one_operand_instruction(OP_JMPR, REG_RA);
}

// prepare to collect the parameters of the next function definition
void code_clear_function_parameters() {
    current_function_parameters.count = 0;
}

// add one parameter name for the function currently being parsed
void code_add_function_parameter(char* name) {
    if (current_function_parameters.count >= MAX_FUNCTION_PARAMETERS) {
        fprintf(stderr, "too many function parameters\n");
        exit(1);
    }

    current_function_parameters.names[current_function_parameters.count++] = name;
}

// copy the arguments below the saved ra/fp into normal local variable slots
void code_add_parameter_locals() {
    for (int i = 0; i < current_function_parameters.count; i++) {
        int source_offset = current_function_parameters.count - i + 2;
        int destination_offset = next_local_offset++;
        int value_register = register_alloc();

        symbol_table_add_local(code_symbol_table, current_function_parameters.names[i], destination_offset);
        code_add_2_operand_instruction(OP_AFC, REG_TMP, source_offset);
        code_add_3_operand_instruction(OP_ADD, REG_TMP, REG_FP, REG_TMP);
        code_add_2_operand_instruction(OP_LOADR, value_register, REG_TMP);
        code_add_compute_local_address(destination_offset);
        code_add_2_operand_instruction(OP_STORER, REG_TMP, value_register);
        code_add_stack_push();
        register_free(value_register);
    }
}

// mark the start of a function and add its prefix
void begin_function_definition(char* name) {
    char comment[128];
    instruction_t* entry = code_add_label();

    snprintf(comment, sizeof(comment), "%s", name);
    instruction_set_comment(entry, comment);
    function_add(name, entry, current_function_parameters.count);
    inside_function = 1;
    next_local_offset = 0;
    code_add_function_prefix();
    code_add_parameter_locals();
    current_function_body_start_address = current_program_address();
    code_reset_registers();
}

// finish a function definition, adding a default return if the body was empty
void end_function_definition() {
    if (current_program_address() == current_function_body_start_address) {
        code_add_2_operand_instruction(OP_AFC, REG_RETURN, 0);
        code_add_function_suffix();
    }

    inside_function = 0;
    code_reset_registers();
}

// add a return statement using the given expression register as return value
void code_add_return_statement(long expression_register) {
    code_add_2_operand_instruction(OP_COP, REG_RETURN, expression_register);
    register_free(expression_register);
    code_add_function_suffix();
    code_reset_registers();
}

// start collecting arguments for a function call
void code_begin_function_arguments() {
    if (function_call_argument_depth >= MAX_CALL_DEPTH) {
        fprintf(stderr, "too many nested function calls\n");
        exit(1);
    }

    function_call_arguments[function_call_argument_depth++].count = 0;
}

// remember one argument register until the call instruction is added
void code_add_function_argument(long expression_register) {
    int current_depth = function_call_argument_depth - 1;
    function_argument_list_t* arguments;

    if (current_depth < 0) {
        fprintf(stderr, "function argument outside call\n");
        exit(1);
    }

    arguments = &function_call_arguments[current_depth];
    if (arguments->count >= MAX_FUNCTION_PARAMETERS) {
        fprintf(stderr, "too many function arguments\n");
        exit(1);
    }

    arguments->registers[arguments->count++] = expression_register;
}

// check if a register currently holds one argument of the call being prepared
int code_is_function_argument_register(int reg, function_argument_list_t* arguments) {
    for (int i = 0; i < arguments->count; i++) {
        if (arguments->registers[i] == reg) {
            return 1;
        }
    }

    return 0;
}

// add a function call and return a register containing the function result
long code_add_function_call(char* name) {
    instruction_t* entry = function_get_entry(name);
    int saved_registers[REG_LAST_GENERAL - REG_FIRST_GENERAL + 1];
    int saved_register_count = 0;
    int current_depth = function_call_argument_depth - 1;
    int argument_count;
    int parameter_count = function_get_parameter_count(name);
    function_argument_list_t* arguments;
    long return_address;
    int result_register;

    if (current_depth < 0) {
        fprintf(stderr, "function call arguments not initialized\n");
        exit(1);
    }

    arguments = &function_call_arguments[current_depth];
    argument_count = arguments->count;

    if (argument_count != parameter_count) {
        fprintf(stderr, "function %s expects %d argument(s), got %d\n", name, parameter_count, argument_count);
        exit(1);
    }

    for (int reg = REG_FIRST_GENERAL; reg <= REG_LAST_GENERAL; reg++) {
        if (register_is_in_use(reg) && !code_is_function_argument_register(reg, arguments)) {
            saved_registers[saved_register_count++] = reg;
            code_add_2_operand_instruction(OP_STORER, REG_SP, reg);
            code_add_stack_push();
        }
    }

    for (int i = 0; i < argument_count; i++) {
        code_add_2_operand_instruction(OP_STORER, REG_SP, arguments->registers[i]);
        code_add_stack_push();
        register_free(arguments->registers[i]);
    }

    return_address = current_program_address() + 2;
    code_add_2_operand_instruction(OP_AFC, REG_RA, return_address);

    instruction_t* jmp = i_op1(OP_JMP, (argument_t){ .instruction = entry });
    jmp->relative = 0;
    scope_add_instruction(scope(), jmp);

    for (int i = 0; i < argument_count; i++) {
        code_add_stack_pop();
    }

    for (int i = saved_register_count - 1; i >= 0; i--) {
        code_add_stack_pop();
        code_add_2_operand_instruction(OP_LOADR, saved_registers[i], REG_SP);
    }

    register_forget_available();
    function_call_argument_depth--;

    result_register = register_alloc();
    code_add_2_operand_instruction(OP_COP, result_register, REG_RETURN);
    register_mark_temporary(result_register);
    return result_register;
}
