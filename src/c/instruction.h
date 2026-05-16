#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <stdio.h>

typedef enum {
    OP_NOP = 0x00,     // no operation
    OP_AFC = 0x01,     // Ri = imm
    OP_COP = 0x02,     // Ri = Rj

    OP_ADD = 0x03,     // Ri = Rj + Rk
    OP_MUL = 0x04,     // Ri = Rj * Rk
    OP_SOU = 0x05,     // Ri = Rj - Rk
    OP_DIV = 0x06,     // Ri = Rj / Rk

    OP_NOT = 0x07,     // Ri = !Rj
    OP_AND = 0x08,     // Ri = Rj && Rk
    OP_OR  = 0x09,     // Ri = Rj || Rk

    OP_BNOT = 0x0A,    // Ri = ~Rj
    OP_BAND = 0x0B,    // Ri = Rj & Rk
    OP_BOR  = 0x0C,    // Ri = Rj | Rk

    OP_LOAD   = 0x0D,  // Ri = MEM[@j]
    OP_STORE  = 0x0E,  // MEM[@i] = Rj
    OP_LOADR  = 0x0F,  // Ri = MEM[Rj]
    OP_STORER = 0x10,  // MEM[Ri] = Rj

    OP_JMP  = 0x11,    // PC = addr
    OP_JMF  = 0x12,    // if Ri == 0, PC = addr
    OP_JMPR = 0x13,    // PC = Ri

    OP_LT  = 0x14,     // Ri = Rj < Rk
    OP_GT  = 0x15,     // Ri = Rj > Rk
    OP_EQ  = 0x16,     // Ri = Rj == Rk
    OP_LEQ = 0x17,     // Ri = Rj <= Rk
    OP_GEQ = 0x18,     // Ri = Rj >= Rk
    OP_NEQ = 0x19,     // Ri = Rj != Rk

    OP_PRI = 0x1A      // print Ri
} opcode_t;

typedef union {
    long value;
    struct instr* instruction;
} argument_t;

typedef struct instr {
    opcode_t opcode;
    argument_t arguments[3];
    int address;
    int relative;

    struct instr* next;
} instruction_t;

instruction_t* i_op1(opcode_t opcode, argument_t a1);
instruction_t* i_op2(opcode_t opcode, argument_t a1, argument_t a2);
instruction_t* i_op3(opcode_t opcode, argument_t a1, argument_t a2, argument_t a3);

void instruction_write(FILE* out, instruction_t* instruction);

#endif
