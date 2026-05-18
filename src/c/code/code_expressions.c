#include "code_internal.h"

#include <stdio.h>

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
