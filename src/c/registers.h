#ifndef REGISTERS_H
#define REGISTERS_H

#define REGISTER_COUNT 16

#define REG_RETURN 0
#define REG_SP 13 // stack pointer
#define REG_FP 14 // frame pointer
#define REG_RA 15 // return address

#define REG_FIRST_GENERAL 1
#define REG_LAST_GENERAL 12

void registers_init(void);
int register_alloc(void);
void register_free(int reg);
int register_is_general(int reg);

#endif
