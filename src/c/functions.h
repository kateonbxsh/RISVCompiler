#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "instruction.h"

#define MAX_FUNCTIONS 64

typedef struct {
    char* name;
    instruction_t* entry;
} function_t;

void function_table_init(void);
void function_add(char* name, instruction_t* entry);
instruction_t* function_get_entry(char* name);

#endif
