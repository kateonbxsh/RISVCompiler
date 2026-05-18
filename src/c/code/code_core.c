#include "code_internal.h"

symbol_table_t* code_symbol_table;
scope_t** code_current_scope;
instruction_t* main_jump;

long current_function_body_start_address;
int inside_function;
int next_local_offset;

function_parameter_list_t current_function_parameters;
function_argument_list_t function_call_arguments[MAX_CALL_DEPTH];
int function_call_argument_depth;

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
    instruction_t* instruction;

    if (opcode == OP_JMP || opcode == OP_JMPR || opcode == OP_PRI) {
        instruction = i_op3(opcode, arg_value(0), arg_value(a1), arg_value(0));
    } else {
        instruction = i_op1(opcode, arg_value(a1));
    }

    scope_add_instruction(scope(), instruction);
    return instruction;
}

// add an instruction with two operands into the current scope
instruction_t* code_add_2_operand_instruction(opcode_t opcode, long a1, long a2) {
    instruction_t* instruction;

    if (opcode == OP_JMF || opcode == OP_STORER) {
        instruction = i_op3(opcode, arg_value(0), arg_value(a1), arg_value(a2));
    } else {
        instruction = i_op2(opcode, arg_value(a1), arg_value(a2));
    }

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
    current_function_parameters.count = 0;
    function_call_argument_depth = 0;
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

// compute the memory address of a local variable into the tmp (r12) register
void code_add_compute_local_address(int offset) {
    code_add_2_operand_instruction(OP_AFC, REG_TMP, offset);
    code_add_3_operand_instruction(OP_SUB, REG_TMP, REG_FP, REG_TMP);
}

// add the first jump, which skips function definitions and goes to main
void code_add_program_start() {
    main_jump = i_op3(OP_JMP, arg_value(0), (argument_t){ .instruction = NULL }, arg_value(0));
    main_jump->relative = 0;
    scope_add_instruction(scope(), main_jump);
    code_reset_registers();
}

// mark the start of main and initialize the stack/frame pointers
void begin_main_method() {
    instruction_t* main_entry = code_add_label();
    instruction_set_comment(main_entry, "main");
    main_jump->arguments[1].instruction = main_entry;
    inside_function = 1;
    next_local_offset = 0;
    code_add_2_operand_instruction(OP_AFC, REG_SP, 255);
    code_add_2_operand_instruction(OP_COP, REG_FP, REG_SP);
    code_reset_registers();
}

// enter a new block scope
void begin_block() {
    *code_current_scope = scope_init_child(scope(), code_symbol_table->symbol_size, next_local_offset);
}

// leave the current block scope and return how many instructions it contained
long end_block() {
    int local_count = next_local_offset - scope()->local_offset_base;

    for (int i = 0; i < local_count; i++) {
        code_add_stack_pop();
    }

    long instruction_count = scope()->instruction_count;
    symbol_table_restore(code_symbol_table, scope()->symbol_table_base);
    next_local_offset = scope()->local_offset_base;
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
    instruction_t* jmp = i_op3(OP_JMP, arg_value(0), (argument_t){ .instruction = target }, arg_value(0));
    jmp->relative = 0;
    scope_add_instruction(scope(), jmp);
    code_reset_registers();
}
