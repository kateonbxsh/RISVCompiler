#ifndef REGISTERS_H
#define REGISTERS_H

#define REGISTER_COUNT 16

#define REG_RETURN 0
#define REG_SP 13 // stack pointer
#define REG_FP 14 // frame pointer
#define REG_RA 15 // return address

#define REG_FIRST_GENERAL 1
#define REG_LAST_GENERAL 12
#define REG_NO_MEMORY -1

typedef struct {
    int used;
    int memory_address;
    int dirty;
} register_descriptor_t;

void registers_init(void);
int register_alloc(void);
void register_free(int reg);
int register_is_general(int reg);
int register_is_dirty(int reg);
int register_memory_address(int reg);

int register_find_memory(int memory_address);
void register_bind_memory(int reg, int memory_address);
void register_forget_memory(int memory_address);
void register_clear_memory(int reg);
void register_mark_dirty(int reg);

#endif
