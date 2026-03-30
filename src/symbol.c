#include "symbol.h"
#include "stdlib.h"
#include <string.h>

void symbol_table_add(symbol_table_t* table, const char* name) {

    symbol_t symbol;
    symbol.name = strdup(name);
    table->table[table->symbol_size++] = symbol;

}
int symbol_get_address(symbol_table_t* table, const char* name) {

    for(int i = 0; i < table->symbol_size; i++) {
        symbol_t isymbol = table->table[i];
        if (strcmp(isymbol.name, name) == 0) {
            return i;
        }
    }
    return -1;

}

int symbol_table_push_temporary(symbol_table_t* table) {
    return table->temp_symbol_index++;
}
int symbol_table_pop_temporary(symbol_table_t* table) {
    return --table->temp_symbol_index;
}