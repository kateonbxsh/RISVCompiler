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
    symbol.storage = SYMBOL_GLOBAL;
    symbol.address = table->symbol_size;
    symbol.offset = 0;
    table->table[table->symbol_size++] = symbol;
    return symbol.address;

}

int symbol_table_add_local(symbol_table_t* table, const char* name, int offset) {
    symbol_t symbol;
    symbol.name = strdup(name);
    symbol.storage = SYMBOL_LOCAL;
    symbol.address = 0;
    symbol.offset = offset;
    table->table[table->symbol_size++] = symbol;
    return symbol.offset;
}

symbol_t* symbol_get(symbol_table_t* table, const char* name) {

    for(int i = table->symbol_size - 1; i >= 0; i--) {
        if (strcmp(table->table[i].name, name) == 0) {
            return &table->table[i];
        }
    }

    return NULL;
}

int symbol_get_address(symbol_table_t* table, const char* name) {
    symbol_t* symbol = symbol_get(table, name);

    if (!symbol) {
        return -1;
    }

    if (symbol->storage == SYMBOL_LOCAL) {
        return symbol->offset;
    }

    return symbol->address;

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
