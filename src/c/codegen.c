#include "codegen.h"
#include "functions.h"
#include "registers.h"

#include <stdio.h>

symbol_table_t* codegen_symbol_table;
scope_t** codegen_current_scope;
instruction_t* main_jump;

int next_return_save_address;
long current_function_body_start_address;
int inside_function;
int next_local_offset;

// get the scope where instructions are currently being emitted
scope_t* scope() {
    return *codegen_current_scope;
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

// emit an instruction with one argument into the current scope
instruction_t* emit1(opcode_t opcode, long a1) {
    instruction_t* instruction = i_op1(opcode, arg_value(a1));
    scope_add_instruction(scope(), instruction);
    return instruction;
}

// emit an instruction with two arguments into the current scope
instruction_t* emit2(opcode_t opcode, long a1, long a2) {
    instruction_t* instruction = i_op2(opcode, arg_value(a1), arg_value(a2));
    scope_add_instruction(scope(), instruction);
    return instruction;
}

// emit an instruction with three arguments into the current scope
instruction_t* emit3(opcode_t opcode, long a1, long a2, long a3) {
    instruction_t* instruction = i_op3(opcode, arg_value(a1), arg_value(a2), arg_value(a3));
    scope_add_instruction(scope(), instruction);
    return instruction;
}

// connect code generation to the symbol table and the root scope
void codegen_init(symbol_table_t* symbols, scope_t** scope) {
    codegen_symbol_table = symbols;
    codegen_current_scope = scope;
    next_return_save_address = 240;
    inside_function = 0;
    next_local_offset = 0;
    registers_init();
    function_table_init();
}

// forget all cached register information
void codegen_reset_registers() {
    registers_init();
}

// emit a NOP instruction that can be used as a jump target
instruction_t* emit_label() {
    instruction_t* label = i_op1(OP_NOP, arg_value(0));
    scope_add_instruction(scope(), label);
    return label;
}

// increment the stack pointer by one memory cell
void emit_inc_sp() {
    emit2(OP_AFC, REG_TMP, 1);
    emit3(OP_ADD, REG_SP, REG_SP, REG_TMP);
}

// decrement the stack pointer by one memory cell
void emit_dec_sp() {
    emit2(OP_AFC, REG_TMP, 1);
    emit3(OP_SOU, REG_SP, REG_SP, REG_TMP);
}

// save the caller state at the start of a function
void emit_function_prologue() {
    emit2(OP_STORER, REG_SP, REG_RA);
    emit_inc_sp();
    emit2(OP_STORER, REG_SP, REG_FP);
    emit_inc_sp();
    emit2(OP_COP, REG_FP, REG_SP);
}

// restore the caller state and jump back at the end of a function
void emit_function_epilogue() {
    emit2(OP_COP, REG_SP, REG_FP);
    emit_dec_sp();
    emit2(OP_LOADR, REG_FP, REG_SP);
    emit_dec_sp();
    emit2(OP_LOADR, REG_RA, REG_SP);
    emit1(OP_JMPR, REG_RA);
}

// compute the memory address of a local variable into the scratch register
void emit_local_address(int offset) {
    emit2(OP_AFC, REG_TMP, offset);
    emit3(OP_ADD, REG_TMP, REG_FP, REG_TMP);
}

// emit the first jump, which skips function definitions and goes to main
void emit_program_start() {
    main_jump = i_op1(OP_JMP, (argument_t){ .instruction = NULL });
    main_jump->relative = 0;
    scope_add_instruction(scope(), main_jump);
    codegen_reset_registers();
}

// mark the start of main and initialize the stack/frame pointers
void begin_main_method() {
    instruction_t* main_entry = emit_label();
    instruction_set_comment(main_entry, "main");
    main_jump->arguments[0].instruction = main_entry;
    inside_function = 1;
    next_local_offset = 0;
    emit2(OP_AFC, REG_SP, 128);
    emit2(OP_COP, REG_FP, REG_SP);
    codegen_reset_registers();
}

// mark the start of a function and emit its prologue
void begin_function_definition(char* name) {
    char comment[128];
    instruction_t* entry = emit_label();

    snprintf(comment, sizeof(comment), "function %s", name);
    instruction_set_comment(entry, comment);
    function_add(name, entry);
    inside_function = 1;
    next_local_offset = 0;
    emit_function_prologue();
    current_function_body_start_address = current_program_address();
    codegen_reset_registers();
}

// finish a function definition, adding a default return if the body was empty
void end_function_definition() {
    if (current_program_address() == current_function_body_start_address) {
        emit2(OP_AFC, REG_RETURN, 0);
        emit_function_epilogue();
    }

    inside_function = 0;
    codegen_reset_registers();
}

// return from a function with the given expression register as return value
void emit_return(long expression_register) {
    emit2(OP_COP, REG_RETURN, expression_register);
    register_free(expression_register);
    emit_function_epilogue();
    codegen_reset_registers();
}

// emit a function call and return a register containing the function result
long emit_function_call(char* name) {
    instruction_t* entry = function_get_entry(name);
    long return_address = current_program_address() + 2;
    int result_register;

    emit2(OP_AFC, REG_RA, return_address);

    instruction_t* jmp = i_op1(OP_JMP, (argument_t){ .instruction = entry });
    jmp->relative = 0;
    scope_add_instruction(scope(), jmp);

    emit_label();
    codegen_reset_registers();

    result_register = register_alloc();
    emit2(OP_COP, result_register, REG_RETURN);
    register_mark_temporary(result_register);
    return result_register;
}

// emit a constant number into a fresh register
long emit_number(long value) {
    int reg = register_alloc();
    emit2(OP_AFC, reg, value);
    register_mark_temporary(reg);
    return reg;
}

// emit code to read a variable value into a register
long emit_identifier(char* name) {
    symbol_t* symbol = symbol_get(codegen_symbol_table, name);
    int memory_address;
    int reg;

    if (!symbol) {
        fprintf(stderr, "unknown variable: %s\n", name);
        return emit_number(0);
    }

    if (symbol->storage == SYMBOL_LOCAL) {
        reg = register_alloc();
        emit_local_address(symbol->offset);
        emit2(OP_LOADR, reg, REG_TMP);
        register_mark_temporary(reg);
        return reg;
    }

    memory_address = symbol->address;
    reg = register_find_memory(memory_address);

    if (reg != -1) {
        return reg;
    }

    reg = register_alloc();
    emit2(OP_LOAD, reg, memory_address);
    register_bind_memory(reg, memory_address);
    return reg;
}

// emit a binary operation, using the left register as the result register
long emit_binary_expression(opcode_t opcode, long left, long right) {
    emit3(opcode, left, left, right);
    register_free(right);
    register_mark_temporary(left);
    return left;
}

// emit a unary operation, updating the same register
long emit_unary_expression(opcode_t opcode, long reg) {
    emit2(opcode, reg, reg);
    register_mark_temporary(reg);
    return reg;
}

// emit the address of a variable for pointer expressions
long emit_address_of(char* name) {
    int reg = register_alloc();
    symbol_t* symbol = symbol_get(codegen_symbol_table, name);

    if (!symbol) {
        fprintf(stderr, "unknown variable: %s\n", name);
        return reg;
    }

    if (symbol->storage == SYMBOL_LOCAL) {
        emit2(OP_AFC, reg, symbol->offset);
        emit3(OP_ADD, reg, REG_FP, reg);
        register_mark_temporary(reg);
        return reg;
    }

    emit2(OP_AFC, reg, symbol->address);
    register_mark_temporary(reg);
    return reg;
}

// emit a pointer read: register = memory[register]
long emit_pointer_load(long address_register) {
    emit2(OP_LOADR, address_register, address_register);
    register_mark_temporary(address_register);
    return address_register;
}

// emit a pointer write: memory[address_register] = value_register
void emit_pointer_assignment(long address_register, long value_register) {
    emit2(OP_STORER, address_register, value_register);
    register_free(address_register);
    register_free(value_register);
    codegen_reset_registers();
}

// declare a variable and store its initial value
void emit_variable_declaration(char* name, long expression_register) {
    long variable_address;

    if (inside_function) {
        int offset = next_local_offset++;
        symbol_table_add_local(codegen_symbol_table, name, offset);
        emit_local_address(offset);
        emit2(OP_STORER, REG_TMP, expression_register);
        emit_inc_sp();
        register_free(expression_register);
        codegen_reset_registers();
        return;
    }

    variable_address = symbol_table_add(codegen_symbol_table, name);

    register_forget_memory(variable_address);
    emit2(OP_STORE, variable_address, expression_register);
    register_bind_memory(expression_register, variable_address);
    register_free(expression_register);
}

// declare a variable with the default value 0
void emit_empty_variable_declaration(char* name) {
    int zero_register = emit_number(0);
    emit_variable_declaration(name, zero_register);
}

// assign a new value to an existing variable
void emit_variable_assignment(char* name, long expression_register) {
    symbol_t* symbol = symbol_get(codegen_symbol_table, name);
    long variable_address;

    if (!symbol) {
        fprintf(stderr, "unknown variable: %s\n", name);
        register_free(expression_register);
        return;
    }

    if (symbol->storage == SYMBOL_LOCAL) {
        emit_local_address(symbol->offset);
        emit2(OP_STORER, REG_TMP, expression_register);
        register_free(expression_register);
        codegen_reset_registers();
        return;
    }

    variable_address = symbol->address;

    register_forget_memory(variable_address);
    emit2(OP_STORE, variable_address, expression_register);
    register_bind_memory(expression_register, variable_address);
    register_free(expression_register);
}

// emit a print instruction for the expression register
void emit_print(long expression_register) {
    emit1(OP_PRI, expression_register);
    register_free(expression_register);
}

// emit a conditional jump with a placeholder target
instruction_t* emit_jmf_placeholder(long condition_register) {
    instruction_t* jmf = emit2(OP_JMF, condition_register, 0);
    jmf->relative = 1;
    register_free(condition_register);
    codegen_reset_registers();
    return jmf;
}

// emit an unconditional jump with a placeholder target
instruction_t* emit_jmp_placeholder() {
    instruction_t* jmp = emit1(OP_JMP, 0);
    jmp->relative = 1;
    codegen_reset_registers();
    return jmp;
}

// patch a conditional jump once we know how far it should jump
void patch_jmf_relative(instruction_t* jmf, long offset) {
    jmf->arguments[1] = arg_value(offset);
    codegen_reset_registers();
}

// patch an unconditional jump once we know how far it should jump
void patch_jmp_relative(instruction_t* jmp, long offset) {
    jmp->arguments[0] = arg_value(offset);
    codegen_reset_registers();
}

// enter a new block scope
void begin_block() {
    *codegen_current_scope = scope_init_child(scope(), codegen_symbol_table->symbol_size);
}

// leave the current block scope and return how many instructions it contained
long end_block() {
    long instruction_count = scope()->instruction_count;
    symbol_table_restore(codegen_symbol_table, scope()->symbol_table_base);
    codegen_reset_registers();
    *codegen_current_scope = scope_flush(scope());
    return instruction_count;
}

// finish main by flushing its scope into the root scope
void end_main_method() {
    inside_function = 0;
    codegen_reset_registers();
    *codegen_current_scope = scope_flush(scope());
}

// get the last instruction emitted in the current scope
instruction_t* current_last_instruction() {
    return scope()->last;
}

// get the instruction immediately after another instruction
instruction_t* first_instruction_after(instruction_t* instruction) {
    return instruction ? instruction->next : NULL;
}

// emit the jump that goes back to the start of a while loop
void emit_loop_back_jump(instruction_t* target) {
    instruction_t* jmp = i_op1(OP_JMP, (argument_t){ .instruction = target });
    jmp->relative = 0;
    scope_add_instruction(scope(), jmp);
    codegen_reset_registers();
}
