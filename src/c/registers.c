#include "registers.h"

#include <stdio.h>
#include <stdlib.h>

register_descriptor_t registers[REGISTER_COUNT];

void reserve_register(int reg) {
    registers[reg].used = 1;
    registers[reg].memory_address = REG_NO_MEMORY;
    registers[reg].dirty = 0;
}

void registers_init() {
    for (int i = 0; i < REGISTER_COUNT; i++) {
        registers[i].used = 0;
        registers[i].memory_address = REG_NO_MEMORY;
        registers[i].dirty = 0;
    }

    reserve_register(REG_RETURN);
    reserve_register(REG_TMP);
    reserve_register(REG_SP);
    reserve_register(REG_FP);
    reserve_register(REG_RA);
}

int register_is_general(int reg) {
    return reg >= REG_FIRST_GENERAL && reg <= REG_LAST_GENERAL;
}

int register_is_dirty(int reg) {
    return register_is_general(reg) && registers[reg].dirty;
}

int register_memory_address(int reg) {
    if (!register_is_general(reg)) {
        return REG_NO_MEMORY;
    }

    return registers[reg].memory_address;
}

int register_find_memory(int memory_address) {
    for (int reg = REG_FIRST_GENERAL; reg <= REG_LAST_GENERAL; reg++) {
        if (!registers[reg].used && registers[reg].memory_address == memory_address && !registers[reg].dirty) {
            registers[reg].used = 1;
            return reg;
        }
    }

    return -1;
}

int register_alloc() {
    for (int reg = REG_FIRST_GENERAL; reg <= REG_LAST_GENERAL; reg++) {
        if (!registers[reg].used) {
            registers[reg].used = 1;
            registers[reg].memory_address = REG_NO_MEMORY;
            registers[reg].dirty = 0;
            return reg;
        }
    }

    fprintf(stderr, "out of registers\n");
    exit(1);
}

void register_free(int reg) {
    if (register_is_general(reg)) {
        registers[reg].used = 0;
    }
}

void register_bind_memory(int reg, int memory_address) {
    if (register_is_general(reg)) {
        registers[reg].memory_address = memory_address;
        registers[reg].dirty = 0;
    }
}

void register_forget_memory(int memory_address) {
    for (int reg = REG_FIRST_GENERAL; reg <= REG_LAST_GENERAL; reg++) {
        if (registers[reg].memory_address == memory_address) {
            registers[reg].memory_address = REG_NO_MEMORY;
            registers[reg].dirty = 0;
        }
    }
}

void register_clear_memory(int reg) {
    if (register_is_general(reg)) {
        registers[reg].memory_address = REG_NO_MEMORY;
    }
}

void register_mark_dirty(int reg) {
    if (register_is_general(reg)) {
        registers[reg].dirty = 1;
        registers[reg].memory_address = REG_NO_MEMORY;
    }
}
