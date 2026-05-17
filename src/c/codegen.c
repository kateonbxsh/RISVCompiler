#include "codegen.h"
#include "functions.h"
#include "registers.h"

#include <stdlib.h>
#include <stdio.h>

symbol_table_t* code_symbol_table;
scope_t** code_current_scope;
instruction_t* main_jump;

long current_function_body_start_address;
int inside_function;
int next_local_offset;

#define MAX_FUNCTION_PARAMETERS 16
#define MAX_CALL_DEPTH 16

char* function_parameters[MAX_FUNCTION_PARAMETERS];
int function_parameter_count;
long function_argument_stack[MAX_CALL_DEPTH][MAX_FUNCTION_PARAMETERS];
int function_argument_counts[MAX_CALL_DEPTH];
int function_argument_depth;

// get the scope where instructions are currently being added
scope_t* scope() {
    return *code_current_scope;
}

// count how many instructions exist before the next instruction
long current_program_address() {
    long address = 0;
    scope_t* current = scope();

    while (current) {
        address += current->instruction_count;
        current = current->parent;
    }

    return address;
}

// build an instruction argument from a plain number
argument_t arg_value(long value) {
    argument_t argument = { .value = value };
    return argument;
}

// add an instruction with one operand into the current scope
instruction_t* code_add_one_operand_instruction(opcode_t opcode, long a1) {
    instruction_t* instruction = i_op1(opcode, arg_value(a1));
    scope_add_instruction(scope(), instruction);
    return instruction;
}

// add an instruction with two operands into the current scope
instruction_t* code_add_2_operand_instruction(opcode_t opcode, long a1, long a2) {
    instruction_t* instruction = i_op2(opcode, arg_value(a1), arg_value(a2));
    scope_add_instruction(scope(), instruction);
    return instruction;
}

// add an instruction with three operands into the current scope
instruction_t* code_add_3_operand_instruction(opcode_t opcode, long a1, long a2, long a3) {
    instruction_t* instruction = i_op3(opcode, arg_value(a1), arg_value(a2), arg_value(a3));
    scope_add_instruction(scope(), instruction);
    return instruction;
}

// connect code generation to the symbol table and the root scope
void code_init(symbol_table_t* symbols, scope_t** scope) {
    code_symbol_table = symbols;
    code_current_scope = scope;
    inside_function = 0;
    next_local_offset = 0;
    function_parameter_count = 0;
    function_argument_depth = 0;
    registers_init();
    function_table_init();
}

// forget all cached register information
void code_reset_registers() {
    registers_init();
}

// add a NOP instruction that can be used as a jump target
instruction_t* code_add_label() {
    instruction_t* label = i_op1(OP_NOP, arg_value(0));
    scope_add_instruction(scope(), label);
    return label;
}

// move the stack pointer to the next free cell lower in memory
void code_add_stack_push() {
    code_add_2_operand_instruction(OP_AFC, REG_TMP, 1);
    code_add_3_operand_instruction(OP_SUB, REG_SP, REG_SP, REG_TMP);
}

// move the stack pointer back to the previous used cell higher in memory
void code_add_stack_pop() {
    code_add_2_operand_instruction(OP_AFC, REG_TMP, 1);
    code_add_3_operand_instruction(OP_ADD, REG_SP, REG_SP, REG_TMP);
}

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

// compute the memory address of a local variable into the tmp (r12) register
void code_add_compute_local_address(int offset) {
    code_add_2_operand_instruction(OP_AFC, REG_TMP, offset);
    code_add_3_operand_instruction(OP_SUB, REG_TMP, REG_FP, REG_TMP);
}

// prepare to collect the parameters of the next function definition
void code_clear_function_parameters() {
    function_parameter_count = 0;
}

// add one parameter name for the function currently being parsed
void code_add_function_parameter(char* name) {
    if (function_parameter_count >= MAX_FUNCTION_PARAMETERS) {
        fprintf(stderr, "too many function parameters\n");
        exit(1);
    }

    function_parameters[function_parameter_count++] = name;
}

// copy the arguments below the saved ra/fp into normal local variable slots
void code_add_parameter_locals() {
    for (int i = 0; i < function_parameter_count; i++) {
        int source_offset = function_parameter_count - i + 2;
        int destination_offset = next_local_offset++;
        int value_register = register_alloc();

        symbol_table_add_local(code_symbol_table, function_parameters[i], destination_offset);
        code_add_2_operand_instruction(OP_AFC, REG_TMP, source_offset);
        code_add_3_operand_instruction(OP_ADD, REG_TMP, REG_FP, REG_TMP);
        code_add_2_operand_instruction(OP_LOADR, value_register, REG_TMP);
        code_add_compute_local_address(destination_offset);
        code_add_2_operand_instruction(OP_STORER, REG_TMP, value_register);
        code_add_stack_push();
        register_free(value_register);
    }
}

// add the first jump, which skips function definitions and goes to main
void code_add_program_start() {
    main_jump = i_op1(OP_JMP, (argument_t){ .instruction = NULL });
    main_jump->relative = 0;
    scope_add_instruction(scope(), main_jump);
    code_reset_registers();
}

// mark the start of main and initialize the stack/frame pointers
void begin_main_method() {
    instruction_t* main_entry = code_add_label();
    instruction_set_comment(main_entry, "main");
    main_jump->arguments[0].instruction = main_entry;
    inside_function = 1;
    next_local_offset = 0;
    code_add_2_operand_instruction(OP_AFC, REG_SP, 255);
    code_add_2_operand_instruction(OP_COP, REG_FP, REG_SP);
    code_reset_registers();
}

// mark the start of a function and add its prefix
void begin_function_definition(char* name) {
    char comment[128];
    instruction_t* entry = code_add_label();

    snprintf(comment, sizeof(comment), "%s", name);
    instruction_set_comment(entry, comment);
    function_add(name, entry, function_parameter_count);
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
    if (function_argument_depth >= MAX_CALL_DEPTH) {
        fprintf(stderr, "too many nested function calls\n");
        exit(1);
    }

    function_argument_counts[function_argument_depth++] = 0;
}

// remember one argument register until the call instruction is added
void code_add_function_argument(long expression_register) {
    int current_depth = function_argument_depth - 1;
    int argument_count;

    if (current_depth < 0) {
        fprintf(stderr, "function argument outside call\n");
        exit(1);
    }

    argument_count = function_argument_counts[current_depth];
    if (argument_count >= MAX_FUNCTION_PARAMETERS) {
        fprintf(stderr, "too many function arguments\n");
        exit(1);
    }

    function_argument_stack[current_depth][argument_count] = expression_register;
    function_argument_counts[current_depth]++;
}

// check if a register is currently used to hold a pending argument
int code_is_function_argument_register(int reg, long* arguments, int argument_count) {
    for (int i = 0; i < argument_count; i++) {
        if (arguments[i] == reg) {
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
    int current_depth = function_argument_depth - 1;
    int argument_count;
    int parameter_count = function_get_parameter_count(name);
    long* arguments;
    long return_address;
    int result_register;

    if (current_depth < 0) {
        fprintf(stderr, "function call arguments not initialized\n");
        exit(1);
    }

    argument_count = function_argument_counts[current_depth];
    arguments = function_argument_stack[current_depth];

    if (argument_count != parameter_count) {
        fprintf(stderr, "function %s expects %d argument(s), got %d\n", name, parameter_count, argument_count);
        exit(1);
    }

    // save any registers currently in use for the computation
    for (int reg = REG_FIRST_GENERAL; reg <= REG_LAST_GENERAL; reg++) {
        if (register_is_in_use(reg) && !code_is_function_argument_register(reg, arguments, argument_count)) {
            saved_registers[saved_register_count++] = reg;
            code_add_2_operand_instruction(OP_STORER, REG_SP, reg);
            code_add_stack_push();
        }
    }

    for (int i = 0; i < argument_count; i++) {
        code_add_2_operand_instruction(OP_STORER, REG_SP, arguments[i]);
        code_add_stack_push();
        register_free(arguments[i]);
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
    function_argument_depth--;

    result_register = register_alloc();
    code_add_2_operand_instruction(OP_COP, result_register, REG_RETURN);
    register_mark_temporary(result_register);
    return result_register;
}

// add a constant load into a fresh register
long code_add_constant_load(long value) {
    int reg = register_alloc();
    code_add_2_operand_instruction(OP_AFC, reg, value);
    register_mark_temporary(reg);
    return reg;
}

// add code to read a variable value into a register
long code_add_variable_load(char* name) {
    symbol_t* symbol = symbol_get(code_symbol_table, name);
    int memory_address;
    int reg;

    if (!symbol) {
        fprintf(stderr, "unknown variable: %s\n", name);
        return code_add_constant_load(0);
    }

    if (symbol->storage == SYMBOL_LOCAL) {
        reg = register_alloc();
        code_add_compute_local_address(symbol->offset);
        code_add_2_operand_instruction(OP_LOADR, reg, REG_TMP);
        register_mark_temporary(reg);
        return reg;
    }

    memory_address = symbol->address;
    reg = register_find_memory(memory_address);

    if (reg != -1) {
        return reg;
    }

    reg = register_alloc();
    code_add_2_operand_instruction(OP_LOAD, reg, memory_address);
    register_bind_memory(reg, memory_address);
    return reg;
}

// add a binary operation, using the left register as the result register
long code_add_binary_expression(opcode_t opcode, long left, long right) {
    code_add_3_operand_instruction(opcode, left, left, right);
    register_free(right);
    register_mark_temporary(left);
    return left;
}

// discard the result of an expression used as a statement
void code_discard_expression(long expression_register) {
    register_free(expression_register);
    code_reset_registers();
}

// add a unary operation, updating the same register
long code_add_unary_expression(opcode_t opcode, long reg) {
    code_add_2_operand_instruction(opcode, reg, reg);
    register_mark_temporary(reg);
    return reg;
}

// add the address of a variable for pointer expressions
long code_add_variable_address(char* name) {
    int reg = register_alloc();
    symbol_t* symbol = symbol_get(code_symbol_table, name);

    if (!symbol) {
        fprintf(stderr, "unknown variable: %s\n", name);
        return reg;
    }

    if (symbol->storage == SYMBOL_LOCAL) {
        code_add_2_operand_instruction(OP_AFC, reg, symbol->offset);
        code_add_3_operand_instruction(OP_SUB, reg, REG_FP, reg);
        register_mark_temporary(reg);
        return reg;
    }

    code_add_2_operand_instruction(OP_AFC, reg, symbol->address);
    register_mark_temporary(reg);
    return reg;
}

// add a pointer read: register = memory[register]
long code_add_pointer_load(long address_register) {
    code_add_2_operand_instruction(OP_LOADR, address_register, address_register);
    register_mark_temporary(address_register);
    return address_register;
}

// add a pointer write: memory[address_register] = value_register
void code_add_pointer_store(long address_register, long value_register) {
    code_add_2_operand_instruction(OP_STORER, address_register, value_register);
    register_free(address_register);
    register_free(value_register);
    code_reset_registers();
}

// declare a variable and store its initial value
void code_add_variable_declaration(char* name, long expression_register) {
    long variable_address;

    if (inside_function) {
        int offset = next_local_offset++;
        symbol_table_add_local(code_symbol_table, name, offset);
        code_add_compute_local_address(offset);
        code_add_2_operand_instruction(OP_STORER, REG_TMP, expression_register);
        code_add_stack_push();
        register_free(expression_register);
        code_reset_registers();
        return;
    }

    variable_address = symbol_table_add(code_symbol_table, name);

    register_forget_memory(variable_address);
    code_add_2_operand_instruction(OP_STORE, variable_address, expression_register);
    register_bind_memory(expression_register, variable_address);
    register_free(expression_register);
}

// declare a variable with the default value 0
void code_add_empty_variable_declaration(char* name) {
    int zero_register = code_add_constant_load(0);
    code_add_variable_declaration(name, zero_register);
}

// assign a new value to an existing variable
void code_add_variable_assignment(char* name, long expression_register) {
    symbol_t* symbol = symbol_get(code_symbol_table, name);
    long variable_address;

    if (!symbol) {
        fprintf(stderr, "unknown variable: %s\n", name);
        register_free(expression_register);
        return;
    }

    if (symbol->storage == SYMBOL_LOCAL) {
        code_add_compute_local_address(symbol->offset);
        code_add_2_operand_instruction(OP_STORER, REG_TMP, expression_register);
        register_free(expression_register);
        code_reset_registers();
        return;
    }

    variable_address = symbol->address;

    register_forget_memory(variable_address);
    code_add_2_operand_instruction(OP_STORE, variable_address, expression_register);
    register_bind_memory(expression_register, variable_address);
    register_free(expression_register);
}

// add a print instruction for the expression register
void code_add_print_statement(long expression_register) {
    code_add_one_operand_instruction(OP_PRI, expression_register);
    register_free(expression_register);
}

// add a conditional jump with a placeholder target
instruction_t* code_add_conditional_jump_placeholder(long condition_register) {
    instruction_t* jmf = code_add_2_operand_instruction(OP_JMF, condition_register, 0);
    jmf->relative = 1;
    register_free(condition_register);
    code_reset_registers();
    return jmf;
}

// add an unconditional jump with a placeholder target
instruction_t* code_add_jump_placeholder() {
    instruction_t* jmp = code_add_one_operand_instruction(OP_JMP, 0);
    jmp->relative = 1;
    code_reset_registers();
    return jmp;
}

// patch a conditional jump once we know how far it should jump
void patch_jmf_relative(instruction_t* jmf, long offset) {
    jmf->arguments[1] = arg_value(offset);
    code_reset_registers();
}

// patch an unconditional jump once we know how far it should jump
void patch_jmp_relative(instruction_t* jmp, long offset) {
    jmp->arguments[0] = arg_value(offset);
    code_reset_registers();
}

// enter a new block scope
void begin_block() {
    *code_current_scope = scope_init_child(scope(), code_symbol_table->symbol_size);
}

// leave the current block scope and return how many instructions it contained
long end_block() {
    long instruction_count = scope()->instruction_count;
    symbol_table_restore(code_symbol_table, scope()->symbol_table_base);
    code_reset_registers();
    *code_current_scope = scope_flush(scope());
    return instruction_count;
}

// finish main by flushing its scope into the root scope
void end_main_method() {
    inside_function = 0;
    code_reset_registers();
    *code_current_scope = scope_flush(scope());
}

// get the last instruction added in the current scope
instruction_t* current_last_instruction() {
    return scope()->last;
}

// get the instruction immediately after another instruction
instruction_t* first_instruction_after(instruction_t* instruction) {
    return instruction ? instruction->next : NULL;
}

// add the jump that goes back to the start of a while loop
void code_add_loop_back_jump(instruction_t* target) {
    instruction_t* jmp = i_op1(OP_JMP, (argument_t){ .instruction = target });
    jmp->relative = 0;
    scope_add_instruction(scope(), jmp);
    code_reset_registers();
}
