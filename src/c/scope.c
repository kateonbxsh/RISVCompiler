#include "scope.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

scope_t* scope_init() {
    scope_t* scope = (scope_t*) malloc(sizeof(scope_t));
    scope->symbol_table_base = 0;
    scope->instruction_count = 0;
    scope->instruction_list = NULL;
    scope->last = NULL;
    return scope;
}

scope_t* scope_init_child(scope_t* parent, int symbol_base) {
    scope_t* scope = malloc(sizeof(scope_t));
    scope->symbol_table_base = symbol_base;
    scope->instruction_count = 0;
    scope->instruction_list = NULL;
    scope->last = NULL;
    scope->parent = parent;
    return scope;
}

void scope_add_instruction(scope_t* scope, instruction_t* instr) {

    if (!scope || !instr) {
        return;
    }

    instruction_t* node = instr;
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
        i_printf(file, current);
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

instruction_t* i_op(opcode_t opcode) {

    instruction_t* instruction = (instruction_t*) malloc(sizeof(instruction_t));
    if (!instruction) {
        perror("malloc");
        return;
    }
    instruction->opcode = opcode;
    instruction->relative = 0;
    return instruction;

}

instruction_t* i_op1(opcode_t opcode, argument_t a1) {
    instruction_t* instruction = i_op(opcode);
    instruction->arguments[0] = a1;
    return instruction;
}
instruction_t* i_op2(opcode_t opcode, argument_t a1, argument_t a2) {
    instruction_t* instruction = i_op(opcode);
    instruction->arguments[0] = a1;
    instruction->arguments[1] = a2;
    return instruction;
}
instruction_t* i_op3(opcode_t opcode, argument_t a1, argument_t a2, argument_t a3) {
    instruction_t* instruction = i_op(opcode);
    instruction->arguments[0] = a1;
    instruction->arguments[1] = a2;
    instruction->arguments[2] = a3;
    return instruction;
}

void i_printf(FILE* out, instruction_t* instruction) {

    opcode_t c = instruction->opcode;
    char* name; char* code;
    switch(c) {
        case ADD: name = "ADD"; break;
        case MUL: name = "MUL"; break;
        case SUB: name = "SUB"; break;
        case DIV: name = "DIV"; break;
        case COP: name = "COP"; break;
        case AFC: name = "AFC"; break;
        case JMP: name = "JMP"; break;
        case JMF: name = "JMF"; break;
        case INF: name = "INF"; break;
        case SUP: name = "SUP"; break;
        case EQU: name = "EQU"; break;
        case PRI: name = "PRI"; break;
    }
    switch(c) {
        case ADD:
        case MUL:
        case SUB:
        case DIV:
        case INF:
        case SUP:
        case EQU:
            fprintf(out, "%s %d %d %d\n", name, 
                instruction->arguments[0], 
                instruction->arguments[1], 
                instruction->arguments[2]);
            break;
        case COP:
        case JMF:
        case AFC:
            fprintf(out, "%s %d %d\n", name, 
                instruction->arguments[0], 
                instruction->arguments[1]);
            break;
        case JMP:    
        case PRI:
            fprintf(out, "%s %d\n", name, 
                instruction->arguments[0]);
            break;
    }

}

void scope_resolve_references(scope_t* scope) {

    instruction_t* current = scope->instruction_list;
    int program_counter = 1;
    while (current) {
        current->address = program_counter++;
        current = current->next;
    }

    current = scope->instruction_list;
    while (current) {
        if (current->opcode == JMP) {
            if (current->relative)
                current->arguments[0].value = current->address + current->arguments[0].value;
            else 
                current->arguments[0].value = current->arguments[0].instruction->address;
        } else if (current->opcode == JMF) {
            if (current->relative)
                current->arguments[1].value = current->address + current->arguments[1].value;
            else 
                current->arguments[1].value = current->arguments[1].instruction->address;
        } 
        current = current->next;
    }

} 