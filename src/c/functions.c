#include "functions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

function_t function_table[MAX_FUNCTIONS];
int function_count;

void function_table_init(void) {
    function_count = 0;
}

void function_add(char* name, instruction_t* entry) {
    if (function_count >= MAX_FUNCTIONS) {
        fprintf(stderr, "too many functions\n");
        exit(1);
    }

    function_table[function_count].name = strdup(name);
    function_table[function_count].entry = entry;
    function_count++;
}

instruction_t* function_get_entry(char* name) {
    for (int i = function_count - 1; i >= 0; i--) {
        if (strcmp(function_table[i].name, name) == 0) {
            return function_table[i].entry;
        }
    }

    fprintf(stderr, "unknown function: %s\n", name);
    exit(1);
}
