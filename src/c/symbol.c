#include "symbol.h"
#include <stdlib.h>
#include <string.h>

// reset the symbol table before parsing a program
void symbol_table_init(symbol_table_t* table) {
    table->symbol_size = 0;
    table->next_global_address = 0;
    table->temp_symbol_size = 0;
}

// add a global variable and return its memory address
int symbol_table_add(symbol_table_t* table, const char* name) {

    symbol_t symbol;
    symbol.name = strdup(name);
    symbol.storage = SYMBOL_GLOBAL;
    symbol.address = table->next_global_address++;
    symbol.offset = 0;
    table->table[table->symbol_size++] = symbol;
    return symbol.address;

}

// add a local variable and return its offset from the frame pointer
int symbol_table_add_local(symbol_table_t* table, const char* name, int offset) {
    symbol_t symbol;
    symbol.name = strdup(name);
    symbol.storage = SYMBOL_LOCAL;
    symbol.address = 0;
    symbol.offset = offset;
    table->table[table->symbol_size++] = symbol;
    return symbol.offset;
}

// find a symbol by name, searching from the newest scope to the oldest
symbol_t* symbol_get(symbol_table_t* table, const char* name) {

    for(int i = table->symbol_size - 1; i >= 0; i--) {
        if (strcmp(table->table[i].name, name) == 0) {
            return &table->table[i];
        }
    }

    return NULL;
}

// get the memory address or local offset of a symbol
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

// remove all symbols declared after a scope started
void symbol_table_restore(symbol_table_t* table, int symbol_size) {
    while (table->symbol_size > symbol_size) {
        free(table->table[--table->symbol_size].name);
    }
    table->temp_symbol_size = 0;
}

// reserve a temporary memory slot from the old memory-based compiler design
int symbol_table_push_temporary(symbol_table_t* table) {
    return table->symbol_size + table->temp_symbol_size++;
}

// release the most recent temporary memory slot
int symbol_table_pop_temporary(symbol_table_t* table) {
    return table->symbol_size + table->temp_symbol_size--;
}

// check if an address belongs to the temporary memory area
int symbol_is_temporary(symbol_table_t* table, int address) {
    return address >= table->symbol_size && (address < table->symbol_size + table->temp_symbol_size);
}
