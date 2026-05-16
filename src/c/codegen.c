#include "codegen.h"
#include "registers.h"

static symbol_table_t* symbol_table;
static scope_t** current_scope;

static scope_t* scope() {
    return *current_scope;
}

static argument_t arg_value(long value) {
    argument_t argument = { .value = value };
    return argument;
}

static instruction_t* emit1(opcode_t opcode, long a1) {
    instruction_t* instruction = i_op1(opcode, arg_value(a1));
    scope_add_instruction(scope(), instruction);
    return instruction;
}

static instruction_t* emit2(opcode_t opcode, long a1, long a2) {
    instruction_t* instruction = i_op2(opcode, arg_value(a1), arg_value(a2));
    scope_add_instruction(scope(), instruction);
    return instruction;
}

static instruction_t* emit3(opcode_t opcode, long a1, long a2, long a3) {
    instruction_t* instruction = i_op3(opcode, arg_value(a1), arg_value(a2), arg_value(a3));
    scope_add_instruction(scope(), instruction);
    return instruction;
}

static void release_if_temporary(long address) {
    if (symbol_is_temporary(symbol_table, address)) {
        symbol_table_pop_temporary(symbol_table);
    }
}

void codegen_init(symbol_table_t* symbols, scope_t** scope) {
    symbol_table = symbols;
    current_scope = scope;
    registers_init();
}

long emit_number(long value) {
    long address = symbol_table_push_temporary(symbol_table);
    emit2(OP_AFC, address, value);
    return address;
}

long emit_binary_expression(opcode_t opcode, long left, long right) {
    release_if_temporary(right);
    release_if_temporary(left);

    long address = symbol_table_push_temporary(symbol_table);
    emit3(opcode, address, left, right);
    return address;
}

void emit_variable_declaration(char* name, long expression_address) {
    release_if_temporary(expression_address);

    long variable_address = symbol_table_add(symbol_table, name);
    if (variable_address != expression_address) {
        emit2(OP_COP, variable_address, expression_address);
    }
}

void emit_variable_assignment(char* name, long expression_address) {
    release_if_temporary(expression_address);
    emit2(OP_COP, symbol_get_address(symbol_table, name), expression_address);
}

void emit_print(long expression_address) {
    release_if_temporary(expression_address);
    emit1(OP_PRI, expression_address);
}

instruction_t* emit_jmf_placeholder(long condition_address) {
    release_if_temporary(condition_address);

    instruction_t* jmf = emit2(OP_JMF, condition_address, 0);
    jmf->relative = 1;
    return jmf;
}

instruction_t* emit_jmp_placeholder(void) {
    instruction_t* jmp = emit1(OP_JMP, 0);
    jmp->relative = 1;
    return jmp;
}

void patch_jmf_relative(instruction_t* jmf, long offset) {
    jmf->arguments[1] = arg_value(offset);
}

void patch_jmp_relative(instruction_t* jmp, long offset) {
    jmp->arguments[0] = arg_value(offset);
}

void begin_block() {
    *current_scope = scope_init_child(scope(), symbol_table->symbol_size);
}

long end_block() {
    long instruction_count = scope()->instruction_count;
    symbol_table_restore(symbol_table, scope()->symbol_table_base);
    *current_scope = scope_flush(scope());
    return instruction_count;
}

void end_main_method() {
    *current_scope = scope_flush(scope());
}

instruction_t* current_last_instruction() {
    return scope()->last;
}

instruction_t* first_instruction_after(instruction_t* instruction) {
    return instruction ? instruction->next : NULL;
}

void emit_loop_back_jump(instruction_t* target) {
    instruction_t* jmp = i_op1(OP_JMP, (argument_t){ .instruction = target });
    jmp->relative = 0;
    scope_add_instruction(scope(), jmp);
}
