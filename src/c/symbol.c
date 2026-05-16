#include "symbol.h"
#include <stdlib.h>
#include <string.h>

void symbol_table_init(symbol_table_t* table) {
    table->symbol_size = 0;
    table->temp_symbol_size = 0;
}

int symbol_table_add(symbol_table_t* table, const char* name) {

    symbol_t symbol;
    symbol.name = strdup(name);
    table->table[table->symbol_size++] = symbol;
    return table->symbol_size-1;

}
int symbol_get_address(symbol_table_t* table, const char* name) {

    for(int i = table->symbol_size - 1; i >= 0; i--) {
        symbol_t isymbol = table->table[i];
        if (strcmp(isymbol.name, name) == 0) {
            return i;
        }
    }
    return -1;

}

void symbol_table_restore(symbol_table_t* table, int symbol_size) {
    while (table->symbol_size > symbol_size) {
        free(table->table[--table->symbol_size].name);
    }
    table->temp_symbol_size = 0;
}

int symbol_table_push_temporary(symbol_table_t* table) {
    return table->symbol_size + table->temp_symbol_size++;
}
int symbol_table_pop_temporary(symbol_table_t* table) {
    return table->symbol_size + table->temp_symbol_size--;
}

int symbol_is_temporary(symbol_table_t* table, int address) {
    return address >= table->symbol_size && (address < table->symbol_size + table->temp_symbol_size);
}
