#include "functions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

function_t function_table[MAX_FUNCTIONS];
int function_count;

// reset the function table before compiling
void function_table_init() {
    function_count = 0;
}

// save the name and first instruction of a function as a NOP
void function_add(char* name, instruction_t* entry) {
    if (function_count >= MAX_FUNCTIONS) {
        fprintf(stderr, "too many functions\n");
        exit(1);
    }

    function_table[function_count].name = strdup(name);
    function_table[function_count].entry = entry;
    function_count++;
}

// find where a function starts so a call can jump to it
instruction_t* function_get_entry(char* name) {
    for (int i = function_count - 1; i >= 0; i--) {
        if (strcmp(function_table[i].name, name) == 0) {
            return function_table[i].entry;
        }
    }

    fprintf(stderr, "unknown function: %s\n", name);
    exit(1);
}
