#ifndef SCOPE_H
#define SCOPE_H

#include <stdio.h>

typedef enum {
    NOOP,
    ADD,
    MUL,
    SUB,
    DIV,
    COP,
    AFC,
    JMP,
    JMF,
    INF,
    SUP,
    EQU,
    PRI
} opcode_t;

typedef union {

    long value;
    struct instr* instruction;

} argument_t;

typedef struct instr {

    opcode_t opcode;
    argument_t arguments[3];
    int address;
    int relative; // flag to know if JMP or JMF inst is relative (offset) or absolute (pointer to instruction)

    struct __instr* next;

} instruction_t;



typedef struct scope {

    instruction_t* instruction_list;
    instruction_t* last;
    int symbol_table_base;
    int instruction_count;
    struct scope* parent;

} scope_t;

scope_t* scope_init();
scope_t* scope_init_child(scope_t* parent, int symbol_base);
void scope_add_instruction(scope_t* scope, instruction_t* instr);
void scope_write_instructions(FILE* file, scope_t* scope);
void scope_resolve_references(scope_t* scope);
scope_t* scope_flush(scope_t* scope);

instruction_t* i_op1(opcode_t opcode, argument_t a1);
instruction_t* i_op2(opcode_t opcode, argument_t a1, argument_t a2);
instruction_t* i_op3(opcode_t opcode, argument_t a1, argument_t a2, argument_t a3);

#endif