#include "instruction.h"

#include <stdlib.h>
#include <string.h>
#include "registers.h"

void print_instruction_end(FILE* out, instruction_t* instruction);

// create a new instruction with only its opcode initialized
instruction_t* i_op(opcode_t opcode) {
    instruction_t* instruction = malloc(sizeof(instruction_t));
    if (!instruction) {
        return NULL;
    }

    instruction->opcode = opcode;
    instruction->relative = 0;
    instruction->comment = NULL;
    instruction->next = NULL;
    return instruction;
}

// create an instruction with one argument
instruction_t* i_op1(opcode_t opcode, argument_t a1) {
    instruction_t* instruction = i_op(opcode);
    if (!instruction) {
        return NULL;
    }

    instruction->arguments[0] = a1;
    return instruction;
}

// create an instruction with two arguments
instruction_t* i_op2(opcode_t opcode, argument_t a1, argument_t a2) {
    instruction_t* instruction = i_op(opcode);
    if (!instruction) {
        return NULL;
    }

    instruction->arguments[0] = a1;
    instruction->arguments[1] = a2;
    return instruction;
}

// create an instruction with three arguments
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

// convert an opcode enum value to the assembly name we print
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
        case OP_STORE: return "STR";
        case OP_LOADR: return "LDI";
        case OP_STORER: return "STI";
        case OP_JMP: return "J";
        case OP_JMF: return "JF";
        case OP_JMPR: return "JI";
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

// print a register name, including special names like sp, fp, and ra
void print_register(FILE* out, long reg) {
    if (register_is_general(reg)) {
        fprintf(out, "r%ld", reg);
        return;
    }
    switch(reg) {
        case REG_RETURN:
            fprintf(out, "r0");
            break;
        case REG_TMP:
            fprintf(out, "r12");
            break;
        case REG_SP:
            fprintf(out, "sp");
            break;
        case REG_FP:
            fprintf(out, "fp");
            break;
        case REG_RA:
            fprintf(out, "ra");
            break;
    }
}

// print a number in hexadecimal format for the assembly file
void print_hex(FILE* out, long value) {
    if (value < 0) {
        fprintf(out, "-0x%lX", -value);
    } else {
        fprintf(out, "0x%lX", value);
    }
}

// print an instruction that has two register operands
void print_binary_register_instruction(FILE* out, const char* name, instruction_t* instruction) {
    fprintf(out, "%s ", name);
    print_register(out, instruction->arguments[0].value);
    fprintf(out, " ");
    print_register(out, instruction->arguments[1].value);
    print_instruction_end(out, instruction);
}

// print an instruction that has three register operands
void print_ternary_register_instruction(FILE* out, const char* name, instruction_t* instruction) {
    fprintf(out, "%s ", name);
    print_register(out, instruction->arguments[0].value);
    fprintf(out, " ");
    print_register(out, instruction->arguments[1].value);
    fprintf(out, " ");
    print_register(out, instruction->arguments[2].value);
    print_instruction_end(out, instruction);
}

// finish the printed instruction line and add a comment when there is one
void print_instruction_end(FILE* out, instruction_t* instruction) {
    if (instruction->comment) {
        fprintf(out, "          %% %s", instruction->comment);
    }

    fprintf(out, "\n");
}

// write one instruction to the output assembly file
void instruction_write(FILE* out, instruction_t* instruction) {
    const char* name = opcode_name(instruction->opcode);

    fprintf(out, "0x%04X:  ", instruction->address);

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
            print_instruction_end(out, instruction);
            break;

        case OP_LOAD:
            fprintf(out, "%s ", name);
            print_register(out, instruction->arguments[0].value);
            fprintf(out, " ");
            print_hex(out, instruction->arguments[1].value);
            print_instruction_end(out, instruction);
            break;

        case OP_STORE:
            fprintf(out, "%s ", name);
            print_hex(out, instruction->arguments[0].value);
            fprintf(out, " ");
            print_register(out, instruction->arguments[1].value);
            print_instruction_end(out, instruction);
            break;

        case OP_JMF:
            fprintf(out, "%s ", name);
            print_register(out, instruction->arguments[0].value);
            fprintf(out, " ");
            print_hex(out, instruction->arguments[1].value);
            print_instruction_end(out, instruction);
            break;

        case OP_JMP:
            fprintf(out, "%s ", name);
            print_hex(out, instruction->arguments[0].value);
            print_instruction_end(out, instruction);
            break;

        case OP_JMPR:
        case OP_PRI:
            fprintf(out, "%s ", name);
            print_register(out, instruction->arguments[0].value);
            print_instruction_end(out, instruction);
            break;

        case OP_NOP:
            fprintf(out, "%s", name);
            print_instruction_end(out, instruction);
            break;
    }
}

// get the first byte operand for the fixed-size binary instruction
unsigned char instruction_binary_a(instruction_t* instruction) {
    return (unsigned char) instruction->arguments[0].value;
}

// get the second byte operand for the fixed-size binary instruction
unsigned char instruction_binary_b(instruction_t* instruction) {
    return (unsigned char) instruction->arguments[1].value;
}

// get the third byte operand for the fixed-size binary instruction
unsigned char instruction_binary_c(instruction_t* instruction) {
    return (unsigned char) instruction->arguments[2].value;
}

// write one instruction as OP A B C bytes
void instruction_write_binary(FILE* out, instruction_t* instruction) {
    unsigned char bytes[4] = {0, 0, 0, 0};

    bytes[0] = (unsigned char) instruction->opcode;

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
            bytes[1] = instruction_binary_a(instruction);
            bytes[2] = instruction_binary_b(instruction);
            bytes[3] = instruction_binary_c(instruction);
            break;

        case OP_COP:
        case OP_NOT:
        case OP_BNOT:
        case OP_LOAD:
        case OP_STORE:
        case OP_LOADR:
        case OP_STORER:
        case OP_AFC:
        case OP_JMF:
            bytes[1] = instruction_binary_a(instruction);
            bytes[2] = instruction_binary_b(instruction);
            break;

        case OP_JMP:
        case OP_JMPR:
        case OP_PRI:
            bytes[1] = instruction_binary_a(instruction);
            break;

        case OP_NOP:
            break;
    }

    fwrite(bytes, sizeof(bytes), 1, out);
}

// attach a human-readable comment to an instruction
void instruction_set_comment(instruction_t* instruction, const char* comment) {
    if (instruction) {
        instruction->comment = strdup(comment);
    }
}
