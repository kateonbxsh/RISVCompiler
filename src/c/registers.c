#include "registers.h"

#include <stdio.h>
#include <stdlib.h>

static int used_registers[REGISTER_COUNT];

void registers_init(void) {
    for (int i = 0; i < REGISTER_COUNT; i++) {
        used_registers[i] = 0;
    }

    used_registers[REG_RETURN] = 1;
    used_registers[REG_SP] = 1;
    used_registers[REG_FP] = 1;
    used_registers[REG_RA] = 1;
}

int register_is_general(int reg) {
    return reg >= REG_FIRST_GENERAL && reg <= REG_LAST_GENERAL;
}

int register_alloc(void) {
    for (int reg = REG_FIRST_GENERAL; reg <= REG_LAST_GENERAL; reg++) {
        if (!used_registers[reg]) {
            used_registers[reg] = 1;
            return reg;
        }
    }

    fprintf(stderr, "out of registers\n");
    exit(1);
}

void register_free(int reg) {
    if (register_is_general(reg)) {
        used_registers[reg] = 0;
    }
}
