#ifndef SCOPE_H
#define SCOPE_H

#include <stdio.h>

typedef struct {

    char* code;
    instruction_t* next;

} instruction_t;

typedef struct {

    instruction_t* instruction_list;
    instruction_t* last;
    int symbol_table_base;
    int instruction_count;
    scope_t* parent;

} scope_t;

scope_t* scope_init();
void scope_add_instruction(scope_t* scope, const char* instr);
void scope_write_instructions(FILE* file, scope_t* scope);
scope_t* scope_flush(scope_t* scope);

#endif