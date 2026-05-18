#include "code_internal.h"

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
