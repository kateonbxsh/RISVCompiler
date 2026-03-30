typedef struct {

    char* name;

} symbol_t;

typedef struct {

    symbol_t table[256];
    int symbol_size = 0;
    int temp_symbol_index = 255;

} symbol_table_t;


void symbol_table_add(symbol_table_t* table, const char* name);
int symbol_get_address(symbol_table_t* table, const char* name);

int symbol_table_push_temporary(symbol_table_t* table);
int symbol_table_pop_temporary(symbol_table_t* table);