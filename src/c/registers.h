#ifndef REGISTERS_H
#define REGISTERS_H

#define REGISTER_COUNT 16

#define REG_RETURN 0
#define REG_SP 13 // stack pointer
#define REG_FP 14 // frame pointer
#define REG_RA 15 // return address
#define REG_TMP 12 // compiler scratch register

#define REG_FIRST_GENERAL 1
#define REG_LAST_GENERAL 11
#define REG_NO_MEMORY -1

/*
the descriptor for a register
`in_use` is whether the register is in use for a computation
`temporary` is whether the register holds a value for a computation, if `memory_address != REG_NO_MEMORY` and `temporary == true` that tells the compiler that the register to not treat this as
a clean cached value from the memory
*/
typedef struct {
    int in_use;
    int memory_address;
    int temporary;
} register_descriptor_t;

void registers_init();
int register_alloc();
void register_free(int reg);
int register_is_general(int reg);
int register_is_temporary(int reg);
int register_memory_address(int reg);

int register_find_memory(int memory_address);
void register_bind_memory(int reg, int memory_address);
void register_forget_memory(int memory_address);
void register_clear_memory(int reg);
void register_mark_temporary(int reg);

#endif
