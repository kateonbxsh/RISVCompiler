#include "registers.h"

#include <stdio.h>
#include <stdlib.h>

// descriptors of registers
register_descriptor_t registers[REGISTER_COUNT];

// reserve a register, aka mark it as in use
void reserve_register(int reg) {
    registers[reg].in_use = 1;
    registers[reg].memory_address = REG_NO_MEMORY;
    registers[reg].temporary = 0;
}

// initialize registers, including special ones
void registers_init() {
    for (int i = 0; i < REGISTER_COUNT; i++) {
        registers[i].in_use = 0;
        registers[i].memory_address = REG_NO_MEMORY;
        registers[i].temporary = 0;
    }

    reserve_register(REG_RETURN);
    reserve_register(REG_TMP);
    reserve_register(REG_SP);
    reserve_register(REG_FP);
    reserve_register(REG_RA);
}

// check if register is general, aka if it's r1 to r11
int register_is_general(int reg) {
    return reg >= REG_FIRST_GENERAL && reg <= REG_LAST_GENERAL;
}

// check if a general register is currently needed by the compiler
int register_is_in_use(int reg) {
    return register_is_general(reg) && registers[reg].in_use;
}

// check if register holds a temporary/computed value rather than a clean memory copy
int register_is_temporary(int reg) {
    return register_is_general(reg) && registers[reg].temporary;
}

// grab the memory address of specific register
int register_memory_address(int reg) {
    if (!register_is_general(reg)) {
        return REG_NO_MEMORY;
    }

    return registers[reg].memory_address;
}

// find the register corresponding to a specific memory address
int register_find_memory(int memory_address) {
    for (int reg = REG_FIRST_GENERAL; reg <= REG_LAST_GENERAL; reg++) {
        if (!registers[reg].in_use && registers[reg].memory_address == memory_address && !registers[reg].temporary) {
            registers[reg].in_use = 1;
            return reg;
        }
    }

    return -1;
}

// allocate a new register amidst the ones not in use
int register_alloc() {
    for (int reg = REG_FIRST_GENERAL; reg <= REG_LAST_GENERAL; reg++) {
        if (!registers[reg].in_use) {
            registers[reg].in_use = 1;
            registers[reg].memory_address = REG_NO_MEMORY;
            registers[reg].temporary = 0;
            return reg;
        }
    }

    fprintf(stderr, "out of registers\n");
    exit(1);
}

// free a register (mark it as no longer in use)
void register_free(int reg) {
    if (register_is_general(reg)) {
        registers[reg].in_use = 0;
    }
}

// mark register as a clean copy of a memory address
void register_bind_memory(int reg, int memory_address) {
    if (register_is_general(reg)) {
        registers[reg].memory_address = memory_address;
        registers[reg].temporary = 0;
    }
}

// clear all registers linked to a certain memory address
void register_forget_memory(int memory_address) {
    for (int reg = REG_FIRST_GENERAL; reg <= REG_LAST_GENERAL; reg++) {
        if (registers[reg].memory_address == memory_address) {
            registers[reg].memory_address = REG_NO_MEMORY;
            registers[reg].temporary = 0;
        }
    }
}

// forget cached values in registers that the current expression is not using
void register_forget_available() {
    for (int reg = REG_FIRST_GENERAL; reg <= REG_LAST_GENERAL; reg++) {
        if (!registers[reg].in_use) {
            registers[reg].memory_address = REG_NO_MEMORY;
            registers[reg].temporary = 0;
        }
    }
}

// clear the memory address linked to one register
void register_clear_memory(int reg) {
    if (register_is_general(reg)) {
        registers[reg].memory_address = REG_NO_MEMORY;
    }
}

// mark register as a temporary value
void register_mark_temporary(int reg) {
    if (register_is_general(reg)) {
        registers[reg].temporary = 1;
        registers[reg].memory_address = REG_NO_MEMORY;
    }
}
