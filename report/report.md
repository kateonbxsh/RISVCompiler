# Computer System Project

#### Students
- Aouab Admou
- Nawfal El-Hammadi

## Base Project

### Overview

We initially implemented the required base project in the lab, this included a memory-oriented compiler using `yacc` and `lex` and a microcontroller in VHDL.

Our syntax relied on C-style declarations, although some aspects are different (we used `let` for declarations, `main { ... }` for the main method etc...), a main method typically looks like this:

```
main {

    let a, b;
    let c = 2;
    b = c + 1;
    while (a < 5) {
        a = b * c;
        c = c + 1;
    }

    if (a == 5) {
        print(a);
    }

}
```

### Symbol table

Initially our symbol table was implemented using a list of structures:

```c
typedef struct {
    char* name;
    int address;
} symbol_t;

typedef struct {

    symbol_t table[256];
    int symbol_size;
    int temp_symbol_size;   

} symbol_table_t;
```

The table initially worked as follows, when a variable is declared, we allocate a space for it at `symbol_size` and increment `symbol_size`, when values are needed for a computation (e.g. 1 + 2), temporary "slots" for the computations at `symbol_size + temp_symbol_size` and increment `temp_symbol_size` (used in old memory-oriented compiler), after the computation is done, in other words, in the action code of `Expression Operator Expression` of the yacc, we check if `$3` (which is the address of which the right operand lived in memory) was temporary, meaning if it was between `symbol_size` and `temp_symbol_size`, in which case we free that slot and decrement `temp_symbol_size`, and we do the same for `$1`.
As a result, temporary slots are freed as soon as they are used for computations, and they stack on top of variable slots.

### Representing instructions

To ease our workflow and make code more readable, we reprensented instructions inside our code as a struct:
```c
typedef union {
    long value;
    struct instr* instruction;
} argument_t;

typedef struct instr {
    opcode_t opcode;
    argument_t arguments[3];
    int address;
    int relative;
    char* comment;

    struct instr* next;
} instruction_t;
```
This is very handy, especially since we made helper functions like `i_op1(opcode, a)`, `i_op2(opcode, a, b)`, `i_op3(opcode, a, b, c)` to help us quickly instantiate a `instruction_t` struct and use it later, it is also very useful to output the code in multiple format (text assembly, binary and VHDL array format).

The reason we used a `union` for the `argument_t` type is because some instructions have a well known value (like a register index or a memory address) while others (like JMP) might point to an instruction address that was not defined yet, the relevance of this will become more apparent in the next section.

### `if` and `while`

In order to implement `if` and `while`, we had the idea of representing **scopes** within the code, since when translating `if` and `while` instruction, the compiler needs to know how many instructions are in the following block of code before outputing the `JMP` instruction.

We settled on this representation:
```c
typedef struct scope {
    instruction_t* instruction_list;
    instruction_t* last;
    int symbol_table_base;
    int instruction_count;
    struct scope* parent;
} scope_t;
```
This representation uses a linked link of scopes, by order of hierarchy (each child points to his parent) which itself references a linked list of instruction blocks.

In the yacc, [explain what happens when entering a if statement or while statemnet with scopes and undefined instruction pointers]

### Microcontroller

On the microprocessor side, we fully implemented the pipeline based on the instruction set defined in the lab specification.

One problem we ran into is in the 3rd stage of the pipeline, where data is read from the RAM, which is synchronized to the clock.
On instructions like `LOAD`, on the clock cycle where data moves from EX/MEM to MEM/RE, we expect data to be read from the RAM and reach MEM/RE, but because of how DRAMs work (and unlike SRAMs), they are slow and require one more clock cycle, at this point we have two choices, either add one more stage to the pipeline, or, have the RAM synchronized on the falling edge of the clock instead of the rising edge, this will mean that by the next rising edge, data will be passd to MEM/RE, but this is only guaranteed in simulation because we do not take into account the signal propagation speed. 

As a matter of fact, in practice, the RAM has half a clock cycle to propagate the signal, when running the implementation, with the clock set to $100MHz$, we noticed a slack of $-0,107ns$, while it was $1.681 ns$ without the falling edge fix.

In order to fix this issue, we decided we should implement a clock divider, this is not only useful to slow down the clock for the CPU so that the signal has enough time to propagate, but it's also useful later on for us to visualize our program on the FPGA.

## Improvements

### Managing Hazards

In the lab specifications, implementing a way to manage hazards was suggested. Only we thought that the proposed way of checking if instruction $i$ is a `AFC` and $i+1$ is a `COP` misses a lot of other cases (other instructions that might write to a register, if there is an instruction between the `AFC` and `COP` etc..)

Since the only hazards we observed are because reads and writes to the registers don't happen at the same stage of the pipeline, we thought of implementing the following logic on our microprocessor:

First we implement a small bank of registers containing a value we will name $busy(R_i)$ for each register $i$, which represents long the CPU should stall should the next instruction be one that reads from $R_i$

When an instruction is processed at the first stage of the pipeline, if it writes to $R_i$, we set $busy(R_i) = 4$, if it reads from $R_i$, we stall the CPU until $busy(R_i) = 0$

This logic takes into account that hazards can still be caused even if two hazardous instructions are seperated by a few others, and lets in instructions that are unrelated and are sure to not cause hazards.

In theory and from the tests we conducted by writing hazardous code to the instruction memory, it seems to solve all possible hazards.

### Register-oriented assembler

We understand that the lab specification strongly suggested we implement a cross-assembler, that transpiles the initial memory-oriented assembler into a reigster-oriented one. But we noticed that in that process, the context of the original code will be lost, and redundant code will be produced.

As an example, take the following code sample:

```c
c = a + a;
```

Firstly, this would be compiled into a memory-oriented assembly, which would typically be:

```
ADD @tmp @a @a
COP @c @tmp
```

Then, with a cross-assembler, this would be transpiled into:

```
// ADD @tmp @a @a is transpiled to
LOAD r1 @a
LOAD r2 @a
ADD r1 r1 r2
STORE @tmp r1

// COP @c @tmp
LOAD r1 @tmp
STORE @c r1
```

As you notice, there is a lot of redundant instructions, on the one hand, we load the value of `a` from memory twice, on the second hand, we store the result of the addition to `tmp`, while we could've directly stored that into `c`.

Of course this highly depends on how the cross-assembler is implemented, it can definitely be optimized to reduce redundancy, but we came to the conclusion that we will never have enough context.

We decided to instead rewrite our compiler as a register-oriented assembler.

[explain how the register oriented assembler exactly, with the same style above, and code snippets, in a step by step pedagogic way (with examples)]

### Homogenizing the instruction set

So far, we decided to homogenize the instruction set, since `LOAD` and `STORE` are part of our new register-oriented assembler. And since we are implementing other features like **functions** and **pointers**, we added `JI` (jump indirect), `LDI` (load indirect), `STI` (store indirect) instructions, we also renamed instructions like jump to `J`, jump if false to `JF`.

We also added comparative operators, logical and bitwise operators as instructions.

This is the final instruction set we settled on:

```
NOP     _     _     _     ; no operation
AFC     Ri    imm   _     ; Ri = imm
COP     Ri    Rj    _     ; Ri = Rj

ADD     Ri    Rj    Rk    ; Ri = Rj + Rk
MUL     Ri    Rj    Rk    ; Ri = Rj * Rk
SUB     Ri    Rj    Rk    ; Ri = Rj - Rk
DIV     Ri    Rj    Rk    ; Ri = Rj / Rk

NOT     Ri    Rj    _     ; Ri = !Rj
AND     Ri    Rj    Rk    ; Ri = Rj && Rk
OR      Ri    Rj    Rk    ; Ri = Rj || Rk
BNOT    Ri    Rj    _     ; Ri = ~Rj
BAND    Ri    Rj    Rk    ; Ri = Rj & Rk
BOR     Ri    Rj    Rk    ; Ri = Rj | Rk

LOAD    Ri    @j    _     ; Ri = MEM[@j]
STR     @i    Rj    _     ; MEM[@i] = Rj
LDI     Ri    Rj    _     ; Ri = MEM[Rj]
STI     Ri    Rj    _     ; MEM[Ri] = Rj

J       _     addr  _     ; PC = addr
JF      _     Ri    addr  ; if Ri == 0, PC = addr
JI      _     Ri    _     ; PC = Ri

LT      Ri    Rj    Rk    ; Ri = Rj < Rk
GT      Ri    Rj    Rk    ; Ri = Rj > Rk
EQ      Ri    Rj    Rk    ; Ri = Rj == Rk
LEQ     Ri    Rj    Rk    ; Ri = Rj <= Rk
GEQ     Ri    Rj    Rk    ; Ri = Rj >= Rk
NEQ     Ri    Rj    Rk    ; Ri = Rj != Rk

PRI     _     Ri   _     ; print Ri
```

The reason we put the arguments of the jump instructions on the second and third slots, is to make it easy to implement on the already existing pipeline.

#### `STI` and `LDI`

These two instructions were needed for manipulating pointers, without them, we cannot translate code like `*b = a`, since we can only store into hardcoded addresses in the assembly.

Their flow in the CPU pipeline is very similar to `STR` and `LOAD`, instead of passing the value directly into the data memory, we pass `B` (or `C`) through the register bank in order to retreive its value before storing or loading.

These two instructions count as a read/write for the purposes of the hazard managing unit.

#### JI

This instruction sets the program counter to the value inside a certain register.
This instruction is required for returning from functions. As a matter of fact, when a function is called, the return address should be 

[TODO LATER, here we explain exactly how flush1 and flush2 works in the pipeline]

#### Other instructions

The rest of the instructions (`AND`, `NOT`, `OR`, `BAND`, `BNOT`, `BOR`, `EQ` etc...) were straightforward to implement, they directly mimmick operators added to the grammar.

On the VHDL side, they are added cases to the ALU we added, with straightforward syntax. Since our ALU operation set count is higher than 7, we added one more bit to the operation index in the ALU.

### Pointers

[explain syntax, how they work, how we discovered %prec in yacc, etc...]

### Functions

[same, explain how we define functions inside our code, that we started by only reserving ra and r0, for simple function calls, then we moved into stack definitions, explain all asepcts function calls, prefix and suffix, what each line does, how the new symbol table works]

### Web app interpreter

[explain quickly how the webapp works, and put a little screenshot of it]

## Final grammar and language

### BNF Representation

[write the BNF representation of our language]

### Program Layout

[typical program layout]

### Discussion & Limitations

[cite all limitations and things that are not possible]