#ifndef SYMBOL_H
#define SYMBOL_H

typedef enum {
    SYMBOL_GLOBAL,
    SYMBOL_LOCAL
} symbol_storage_t;

typedef struct {
    char* name;
    symbol_storage_t storage;
    int address;
    int offset;
} symbol_t;

typedef struct {

    symbol_t table[256];
    int symbol_size;
    int next_global_address;
    // used in old memory-oriented compiler
    int temp_symbol_size;

} symbol_table_t;

void symbol_table_init(symbol_table_t* table);

int symbol_table_add(symbol_table_t* table, const char* name);
int symbol_table_add_local(symbol_table_t* table, const char* name, int offset);
symbol_t* symbol_get(symbol_table_t* table, const char* name);
int symbol_get_address(symbol_table_t* table, const char* name);
void symbol_table_restore(symbol_table_t* table, int symbol_size);

// used in old memory-oriented compiler
int symbol_table_push_temporary(symbol_table_t* table);
int symbol_table_pop_temporary(symbol_table_t* table);

// used in old memory-oriented compiler
int symbol_is_temporary(symbol_table_t* table, int address);

#endif
