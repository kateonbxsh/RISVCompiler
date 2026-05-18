#ifndef SCOPE_H
#define SCOPE_H

#include <stdio.h>

#include "instruction.h"

/*
    this scope structure was made to make
    "scopes" inside the code well represented
    especially for loops and if statements, to know exactly how many
    instructions inside a scope (block delimited with brackets {})

    `instruction_list` is the linked list of instructions inside the scope
    `last` is the last of them :) lol
    `symbol_table_base` is the last index of the symbol table before the scope, so that once we leave the scope, we can forget all the variables defined after this base
    `local_offset_base` is the next local stack offset before the scope, so that leaving the scope can free local stack slots
    `parent` is the parent

*/
typedef struct scope {
    instruction_t* instruction_list;
    instruction_t* last;
    int symbol_table_base;
    int local_offset_base;
    int instruction_count;
    struct scope* parent;
} scope_t;

scope_t* scope_init();
scope_t* scope_init_child(scope_t* parent, int symbol_base, int local_offset_base);
scope_t* scope_flush(scope_t* scope);

void scope_add_instruction(scope_t* scope, instruction_t* instr);
void scope_resolve_references(scope_t* scope);
void scope_write_instructions(FILE* file, scope_t* scope);
void scope_write_binary(FILE* file, scope_t* scope);
void scope_write_vhdl_array(FILE* file, scope_t* scope);

#endif
