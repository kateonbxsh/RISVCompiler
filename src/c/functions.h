#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "instruction.h"

#define MAX_FUNCTIONS 64

typedef struct {
    char* name;
    instruction_t* entry;
    int parameter_count;
} function_t;

void function_table_init();
void function_add(char* name, instruction_t* entry, int parameter_count);
instruction_t* function_get_entry(char* name);
int function_get_parameter_count(char* name);

#endif
