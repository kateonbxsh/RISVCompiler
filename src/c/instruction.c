#include "instruction.h"

#include <stdlib.h>

instruction_t* i_op(opcode_t opcode) {
    instruction_t* instruction = malloc(sizeof(instruction_t));
    if (!instruction) {
        return NULL;
    }

    instruction->opcode = opcode;
    instruction->relative = 0;
    instruction->next = NULL;
    return instruction;
}

instruction_t* i_op1(opcode_t opcode, argument_t a1) {
    instruction_t* instruction = i_op(opcode);
    if (!instruction) {
        return NULL;
    }

    instruction->arguments[0] = a1;
    return instruction;
}

instruction_t* i_op2(opcode_t opcode, argument_t a1, argument_t a2) {
    instruction_t* instruction = i_op(opcode);
    if (!instruction) {
        return NULL;
    }

    instruction->arguments[0] = a1;
    instruction->arguments[1] = a2;
    return instruction;
}

instruction_t* i_op3(opcode_t opcode, argument_t a1, argument_t a2, argument_t a3) {
    instruction_t* instruction = i_op(opcode);
    if (!instruction) {
        return NULL;
    }

    instruction->arguments[0] = a1;
    instruction->arguments[1] = a2;
    instruction->arguments[2] = a3;
    return instruction;
}

const char* opcode_name(opcode_t opcode) {
    switch(opcode) {
        case OP_NOP: return "NOP";
        case OP_AFC: return "AFC";
        case OP_COP: return "COP";
        case OP_ADD: return "ADD";
        case OP_MUL: return "MUL";
        case OP_SOU: return "SOU";
        case OP_DIV: return "DIV";
        case OP_NOT: return "NOT";
        case OP_AND: return "AND";
        case OP_OR: return "OR";
        case OP_BNOT: return "BNOT";
        case OP_BAND: return "BAND";
        case OP_BOR: return "BOR";
        case OP_LOAD: return "LOAD";
        case OP_STORE: return "STORE";
        case OP_LOADR: return "LOADR";
        case OP_STORER: return "STORER";
        case OP_JMP: return "JMP";
        case OP_JMF: return "JMF";
        case OP_JMPR: return "JMPR";
        case OP_LT: return "LT";
        case OP_GT: return "GT";
        case OP_EQ: return "EQ";
        case OP_LEQ: return "LEQ";
        case OP_GEQ: return "GEQ";
        case OP_NEQ: return "NEQ";
        case OP_PRI: return "PRI";
    }

    return "UNKNOWN";
}

void print_register(FILE* out, long reg) {
    fprintf(out, "r%ld", reg);
}

void print_hex(FILE* out, long value) {
    if (value < 0) {
        fprintf(out, "-0x%lX", -value);
    } else {
        fprintf(out, "0x%lX", value);
    }
}

void print_binary_register_instruction(FILE* out, const char* name, instruction_t* instruction) {
    fprintf(out, "%s ", name);
    print_register(out, instruction->arguments[0].value);
    fprintf(out, " ");
    print_register(out, instruction->arguments[1].value);
    fprintf(out, "\n");
}

void print_ternary_register_instruction(FILE* out, const char* name, instruction_t* instruction) {
    fprintf(out, "%s ", name);
    print_register(out, instruction->arguments[0].value);
    fprintf(out, " ");
    print_register(out, instruction->arguments[1].value);
    fprintf(out, " ");
    print_register(out, instruction->arguments[2].value);
    fprintf(out, "\n");
}

void instruction_write(FILE* out, instruction_t* instruction) {
    const char* name = opcode_name(instruction->opcode);

    fprintf(out, "0x%02X: ", instruction->address);

    switch(instruction->opcode) {
        case OP_ADD:
        case OP_MUL:
        case OP_SOU:
        case OP_DIV:
        case OP_AND:
        case OP_OR:
        case OP_BAND:
        case OP_BOR:
        case OP_LT:
        case OP_GT:
        case OP_EQ:
        case OP_LEQ:
        case OP_GEQ:
        case OP_NEQ:
            print_ternary_register_instruction(out, name, instruction);
            break;

        case OP_COP:
        case OP_NOT:
        case OP_BNOT:
        case OP_LOADR:
        case OP_STORER:
            print_binary_register_instruction(out, name, instruction);
            break;

        case OP_AFC:
            fprintf(out, "%s ", name);
            print_register(out, instruction->arguments[0].value);
            fprintf(out, " ");
            print_hex(out, instruction->arguments[1].value);
            fprintf(out, "\n");
            break;

        case OP_LOAD:
            fprintf(out, "%s ", name);
            print_register(out, instruction->arguments[0].value);
            fprintf(out, " ");
            print_hex(out, instruction->arguments[1].value);
            fprintf(out, "\n");
            break;

        case OP_STORE:
            fprintf(out, "%s ", name);
            print_hex(out, instruction->arguments[0].value);
            fprintf(out, " ");
            print_register(out, instruction->arguments[1].value);
            fprintf(out, "\n");
            break;

        case OP_JMF:
            fprintf(out, "%s ", name);
            print_register(out, instruction->arguments[0].value);
            fprintf(out, " ");
            print_hex(out, instruction->arguments[1].value);
            fprintf(out, "\n");
            break;

        case OP_JMP:
            fprintf(out, "%s ", name);
            print_hex(out, instruction->arguments[0].value);
            fprintf(out, "\n");
            break;

        case OP_JMPR:
        case OP_PRI:
            fprintf(out, "%s ", name);
            print_register(out, instruction->arguments[0].value);
            fprintf(out, "\n");
            break;

        case OP_NOP:
            fprintf(out, "%s\n", name);
            break;
    }
}
