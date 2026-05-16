#include "scope.h"

#include <stdlib.h>

// initialize a scope
scope_t* scope_init() {
    scope_t* scope = malloc(sizeof(scope_t));
    if (!scope) {
        return NULL;
    }

    scope->symbol_table_base = 0;
    scope->instruction_count = 0;
    scope->instruction_list = NULL;
    scope->last = NULL;
    scope->parent = NULL;
    return scope;
}

// initializes a child scope inside a scope, this happens whenever we enter a block (delimited with brackets)
scope_t* scope_init_child(scope_t* parent, int symbol_base) {
    scope_t* scope = scope_init();
    if (!scope) {
        return NULL;
    }

    scope->symbol_table_base = symbol_base;
    scope->parent = parent;
    return scope;
}

// adds an instruction object to the scope
void scope_add_instruction(scope_t* scope, instruction_t* instr) {
    if (!scope || !instr) {
        return;
    }

    instr->next = NULL;

    if (scope->last) {
        scope->last->next = instr;
    } else {
        scope->instruction_list = instr;
    }

    scope->last = instr;
    scope->instruction_count++;
}

// this method "flushes" a scope, which takes all the scope's instructions, and appends them to the parent's, and frees the scope object
scope_t* scope_flush(scope_t* scope) {
    if (!scope) {
        return NULL;
    }

    scope_t* parent = scope->parent;
    if (!parent) {
        return scope;
    }

    if (scope->instruction_list) {
        if (parent->last) {
            parent->last->next = scope->instruction_list;
        } else {
            parent->instruction_list = scope->instruction_list;
        }

        parent->last = scope->last;
        parent->instruction_count += scope->instruction_count;
    }

    free(scope);
    return parent;
}

/*
    after the program is fully compiled, all JMP instructions still reference either
    a relative offset, or point to another instruction
    this function firstly goes over all instructions, and defines their instruction address
    then goes over all JMP and JMF instructions, and converts those references
    into a instruction address
*/
void scope_resolve_references(scope_t* scope) {
    instruction_t* current = scope->instruction_list;
    int program_counter = 0;

    while (current) {
        current->address = program_counter++;
        current = current->next;
    }

    current = scope->instruction_list;
    while (current) {
        if (current->opcode == OP_JMP) {
            if (current->relative) {
                current->arguments[0].value = current->address + current->arguments[0].value;
            } else {
                instruction_t* addr = current->arguments[0].instruction;
                current->arguments[0].value = addr ? addr->address : 0;
            }
        } else if (current->opcode == OP_JMF) {
            if (current->relative) {
                current->arguments[1].value = current->address + current->arguments[1].value;
            } else {
                instruction_t* addr = current->arguments[1].instruction;
                current->arguments[1].value = addr ? addr->address : 0;
            }
        }

        current = current->next;
    }
}

// write every instruction from a scope into the assembly output file
void scope_write_instructions(FILE* file, scope_t* scope) {
    if (!scope) {
        return;
    }

    instruction_t* current = scope->instruction_list;
    while (current) {
        instruction_write(file, current);
        current = current->next;
    }
}
