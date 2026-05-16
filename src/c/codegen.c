#include "codegen.h"
#include "registers.h"

symbol_table_t* codegen_symbol_table;
scope_t** codegen_current_scope;

scope_t* scope() {
    return *codegen_current_scope;
}

argument_t arg_value(long value) {
    argument_t argument = { .value = value };
    return argument;
}

instruction_t* emit1(opcode_t opcode, long a1) {
    instruction_t* instruction = i_op1(opcode, arg_value(a1));
    scope_add_instruction(scope(), instruction);
    return instruction;
}

instruction_t* emit2(opcode_t opcode, long a1, long a2) {
    instruction_t* instruction = i_op2(opcode, arg_value(a1), arg_value(a2));
    scope_add_instruction(scope(), instruction);
    return instruction;
}

instruction_t* emit3(opcode_t opcode, long a1, long a2, long a3) {
    instruction_t* instruction = i_op3(opcode, arg_value(a1), arg_value(a2), arg_value(a3));
    scope_add_instruction(scope(), instruction);
    return instruction;
}

void codegen_init(symbol_table_t* symbols, scope_t** scope) {
    codegen_symbol_table = symbols;
    codegen_current_scope = scope;
    registers_init();
}

void codegen_reset_registers(void) {
    registers_init();
}

long emit_number(long value) {
    int reg = register_alloc();
    emit2(OP_AFC, reg, value);
    register_mark_dirty(reg);
    return reg;
}

long emit_identifier(char* name) {
    int memory_address = symbol_get_address(codegen_symbol_table, name);
    int reg = register_find_memory(memory_address);

    if (reg != -1) {
        return reg;
    }

    reg = register_alloc();
    emit2(OP_LOAD, reg, memory_address);
    register_bind_memory(reg, memory_address);
    return reg;
}

long emit_binary_expression(opcode_t opcode, long left, long right) {
    emit3(opcode, left, left, right);
    register_free(right);
    register_mark_dirty(left);
    return left;
}

long emit_unary_expression(opcode_t opcode, long reg) {
    emit2(opcode, reg, reg);
    register_mark_dirty(reg);
    return reg;
}

long emit_address_of(char* name) {
    int reg = register_alloc();
    int memory_address = symbol_get_address(codegen_symbol_table, name);

    emit2(OP_AFC, reg, memory_address);
    register_mark_dirty(reg);
    return reg;
}

long emit_pointer_load(long address_register) {
    emit2(OP_LOADR, address_register, address_register);
    register_mark_dirty(address_register);
    return address_register;
}

void emit_pointer_assignment(long address_register, long value_register) {
    emit2(OP_STORER, address_register, value_register);
    register_free(address_register);
    register_free(value_register);
    codegen_reset_registers();
}

void emit_variable_declaration(char* name, long expression_register) {
    long variable_address = symbol_table_add(codegen_symbol_table, name);

    register_forget_memory(variable_address);
    emit2(OP_STORE, variable_address, expression_register);
    register_bind_memory(expression_register, variable_address);
    register_free(expression_register);
}

void emit_variable_assignment(char* name, long expression_register) {
    long variable_address = symbol_get_address(codegen_symbol_table, name);

    register_forget_memory(variable_address);
    emit2(OP_STORE, variable_address, expression_register);
    register_bind_memory(expression_register, variable_address);
    register_free(expression_register);
}

void emit_print(long expression_register) {
    emit1(OP_PRI, expression_register);
    register_free(expression_register);
}

instruction_t* emit_jmf_placeholder(long condition_register) {
    instruction_t* jmf = emit2(OP_JMF, condition_register, 0);
    jmf->relative = 1;
    register_free(condition_register);
    codegen_reset_registers();
    return jmf;
}

instruction_t* emit_jmp_placeholder(void) {
    instruction_t* jmp = emit1(OP_JMP, 0);
    jmp->relative = 1;
    codegen_reset_registers();
    return jmp;
}

void patch_jmf_relative(instruction_t* jmf, long offset) {
    jmf->arguments[1] = arg_value(offset);
    codegen_reset_registers();
}

void patch_jmp_relative(instruction_t* jmp, long offset) {
    jmp->arguments[0] = arg_value(offset);
    codegen_reset_registers();
}

void begin_block(void) {
    *codegen_current_scope = scope_init_child(scope(), codegen_symbol_table->symbol_size);
}

long end_block() {
    long instruction_count = scope()->instruction_count;
    symbol_table_restore(codegen_symbol_table, scope()->symbol_table_base);
    codegen_reset_registers();
    *codegen_current_scope = scope_flush(scope());
    return instruction_count;
}

void end_main_method(void) {
    *codegen_current_scope = scope_flush(scope());
}

instruction_t* current_last_instruction(void) {
    return scope()->last;
}

instruction_t* first_instruction_after(instruction_t* instruction) {
    return instruction ? instruction->next : NULL;
}

void emit_loop_back_jump(instruction_t* target) {
    instruction_t* jmp = i_op1(OP_JMP, (argument_t){ .instruction = target });
    jmp->relative = 0;
    scope_add_instruction(scope(), jmp);
    codegen_reset_registers();
}
