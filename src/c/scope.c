#include "scope.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

scope_t scope_init() {
    scope_t* scope = (scope_t*) malloc(sizeof(scope_t));
    scope->symbol_table_base = 0;
    scope->instruction_count = 0;
    scope->instruction_list = NULL;
    scope->last = NULL;
}

scope_t scope_init_child(scope_t* parent, int symbol_base) {
    scope_t* scope = (scope_t*) malloc(sizeof(scope_t));
    scope->symbol_table_base = symbol_base;
    scope->instruction_count = 0;
    scope->instruction_list = NULL;
    scope->last = NULL;
    scope->parent = parent;
}

void scope_add_instruction(scope_t* scope, const char* instr) {

    if (!scope || !instr) {
        return;
    }

    instruction_t* node = (instruction_t*) malloc(sizeof(instruction_t));
    if (!node) {
        perror("malloc");
        return;
    }

    node->code = strdup(instr);
    if (!node->code) {
        perror("strdup");
        free(node);
        return;
    }
    node->next = NULL;

    if (scope->last) {
        scope->last->next = node;
    } else {
        scope->instruction_list = node;
    }

    scope->last = node;
    scope->instruction_count++;
}

void scope_write_instructions(FILE* file, scope_t* scope) {

    if (!scope) {
        return;
    }

    instruction_t* current = scope->instruction_list;
    while (current) {
        if (current->code) {
            fputs(file, current->code);
        }
        current = current->next;
    }
}

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

    scope->instruction_list = NULL;
    scope->last = NULL;
    scope->instruction_count = 0;
    scope->parent = NULL;

    free(scope);
    return parent;
}
