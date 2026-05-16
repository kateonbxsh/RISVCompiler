#ifndef SCOPE_H
#define SCOPE_H

#include <stdio.h>

#include "instruction.h"

typedef struct scope {
    instruction_t* instruction_list;
    instruction_t* last;
    int symbol_table_base;
    int instruction_count;
    struct scope* parent;
} scope_t;

scope_t* scope_init(void);
scope_t* scope_init_child(scope_t* parent, int symbol_base);
scope_t* scope_flush(scope_t* scope);

void scope_add_instruction(scope_t* scope, instruction_t* instr);
void scope_resolve_references(scope_t* scope);
void scope_write_instructions(FILE* file, scope_t* scope);

#endif
