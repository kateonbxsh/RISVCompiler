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

The compiler currently produces three output files:

- `build/out.s`: human-readable assembly, useful for debugging.
- `build/out.bin`: raw binary instructions for loading into instruction memory.
- `build/out.vhd`: VHDL ROM array values, written as lines such as `x"010DFF00",`.

The reason we used a `union` for the `argument_t` type is because some instructions have a well known value (like a register index or a memory address) while others (like JMP) might point to an instruction address that was not defined yet, the relevance of this will become more apparent in the next section.

### `if` and `while`

In order to implement `if` and `while`, we had the idea of representing **scopes** within the code, since when translating `if` and `while` instruction, the compiler needs to know how many instructions are in the following block of code before outputing the `JMP` instruction.

We settled on this representation:
```c
typedef struct scope {
    instruction_t* instruction_list;
    instruction_t* last;
    int symbol_table_base;
    // used later when adding stack frames for local variables
    int local_offset_base;
    int instruction_count;
    struct scope* parent;
} scope_t;
```
This representation uses a linked link of scopes, by order of hierarchy (each child points to his parent) which itself references a linked list of instruction blocks.

In the yacc, when entering a block, we call `begin_block()`. This creates a child scope and temporarily makes it the current scope. All instructions produced inside the block are therefore stored separately from the parent scope. When the closing brace is reached, `end_block()` flushes the child scope back into its parent and returns the number of instructions that were inside it.

When entering a block, the compiler also stores the current symbol table size in `symbol_table_base`. Later, when leaving the block, the symbol table is restored to this saved size. This means variables declared inside a block are removed from the compiler symbol table and cannot be used outside the block.

This is useful for `if` because we first generate the condition, then a `JF` instruction with a target that is not known yet:

```
; if (a < 5) { ... }
LT r1 r1 r2
JF r1 ?      ; unknown until the block is parsed
```

After the block is parsed, the compiler knows exactly how many instructions it contains, so it can patch the `JF` instruction:

```
LT r1 r1 r2
JF r1 0x06   ; jump after the if block if the condition is false
```

The same idea is used for `while`, but with one additional jump at the end. First, the compiler remembers the instruction where the condition starts. Then it emits a `JF` placeholder for leaving the loop. Once the body is parsed, it patches the `JF` and emits a final `J` that goes back to the condition:

```
; while (a < 5) { a = a + 1; }
LT r1 r1 r2
JF r1 end
...
J condition
```

This makes the yacc actions easier to write, because the grammar can parse the body first and patch jump targets only when enough information is known.

### Compiler organization

To keep the compiler readable, the code generation logic was split into several files:

- `yacc.y` contains the grammar and calls the code generation helpers from semantic actions.
- `code_core.c` contains program initialization, `main`, and block enter/exit logic.
- `code_expressions.c` contains expression generation, variable assignment, declarations, and pointer operations.
- `code_functions.c` contains function definitions, function calls, parameters, and stack frame logic.
- `code_jumps.c` contains jump placeholders and patching helpers.
- `registers.c` contains the register descriptors and allocation logic.
- `instruction.c` contains instruction creation, printing, binary encoding, and VHDL array output.

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

Of course this highly depends on how the cross-assembler is implemented, it can definitely be optimized to reduce redundancy, but we thought it would be nice to use the context from the compiler directly.

We decided to instead rewrite our compiler as a register-oriented assembler.

The main idea of the register-oriented compiler is to directly manipulate expressions inside registers, and only store and load them into memory when done. The temporary symbol table becomes obsolete at this point. Each expression returns a register number to the yacc action above it.

To manage this, the compiler keeps a small descriptor for each register:

```c
#define REGISTER_COUNT 16

#define REG_TMP 12
#define REG_FIRST_GENERAL 1
#define REG_LAST_GENERAL 11 
#define REG_NO_MEMORY -1 // constant for memory_address that means it's not liked to any memory

typedef struct {
    int in_use;
    int memory_address;
    int temporary;
} register_descriptor_t;
```

The registers `r1` to `r11` are used for normal expression computations. Registers like `r12` are reserved as a scratch register for the compiler itself. For example, `r12` is reserved if the compiler needs a temporary register to compute an address before loading or storing a value, this will be used later down the line for pointers and functions, as will the rest of the registers also be reserved for special values (stack pointer, etc...)

At the start of code generation, all registers are reset, then any special registers are reserved:

```c
void registers_init() {
    for (int i = 0; i < REGISTER_COUNT; i++) {
        registers[i].in_use = 0;
        registers[i].memory_address = REG_NO_MEMORY;
        registers[i].temporary = 0;
    }
    // reserve special registers...
}
```

When the compiler needs a register for an expression, it only searches through the general registers:

```c
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
```

The field `in_use` tells the compiler whether the register is currently part of an expression being generated. For example, while compiling `a + 1`, the register holding `a` and the register holding `1` are both marked as in use. When the addition is done, the right register can be freed.

The field `memory_address` is used as a small cache. If a register contains a clean copy of `MEM[3]`, then the compiler can remember `memory_address = 3`. Later, if the same variable is needed again and memory address `3` is still cached in a free register, the compiler can reuse that register instead of generating another `LOAD`.

The field `temporary` tells whether the register still represents a clean memory value or whether it now contains a computed expression. For example, if `r1` initially contains `a`, then:

```
ADD r1 r1 r2
```

means `r1` no longer contains only `a`. It now contains the result of a computation. In that case, it is marked as temporary, and the compiler should not reuse it later as if it were still a clean copy of `a`.

For example, when parsing:

```c
let c = a + 1;
```

The compiler first loads `a` into a register:

```
LOAD r1 @a
```

Then it loads the constant `1` into another register:

```
AFC r2 0x1
```

Then the rule `Expression tPLUS Expression` combines the two registers. In our implementation, the left register is reused as the destination:

```
ADD r1 r1 r2
```

At this point, the expression `a + 1` is represented by `r1`. The declaration then stores this final value directly into `c`:

```
STR @c r1
```

So the complete translation becomes:

```
LOAD r1 @a
AFC r2 0x1
ADD r1 r1 r2
STR @c r1
```

This is already better than the memory-oriented version, because no temporary memory slot is needed for `a + 1`.

The register table also allows the compiler to reuse a value that is already in a register:

```c
let a = 1;
let b = a;
```

This can avoid reloading `a` from memory if the value of `a` is still known to be inside a register. However, this optimization is intentionally conservative. After branches, loops, and block exits, the compiler resets its register knowledge because memory may have changed through another path.

It is important to note that this reset is only a compiler reset. It does not emit instructions to clear the real CPU registers. It only clears the compiler's internal knowledge of what each register contains, so that future code generation reloads values from memory when needed.

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
STI     _     Ri    Rj    ; MEM[Ri] = Rj

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

These two instructions count as a read/write for the purposes of the managing hazards.

#### JI

This instruction sets the program counter to the value inside a certain register. Unlike `J`, where the destination is written directly in the instruction, `JI` takes the destination from a register:

```
JI r1
```

This is useful whenever the jump target is not fixed inside the instruction itself. Later, in the Functions section, we use it to return to the caller, because the return address depends on where the function was called from.

On the pipeline side, `JI` behaves like a jump whose destination is read from a register instead of being encoded directly in the instruction. When the jump is taken, the instructions that were already fetched after it are no longer valid, so the fetch/decode stages have to be flushed in the same way as for the other jump instructions.

#### Other instructions

The rest of the instructions (`AND`, `NOT`, `OR`, `BAND`, `BNOT`, `BOR`, `EQ` etc...) were straightforward to implement, they directly mimmick operators added to the grammar.

On the VHDL side, they are added cases to the ALU we added, with straightforward syntax. Since our ALU operation set count is higher than 7, we added one more bit to the operation index in the ALU.

### Pointers

Pointers were added with two operators: `&` to take the address of a variable, and `*` to read or write through an address.

For a first example with a global variable:

```c
let a = 7;

main {
    let b = &a;
    print(*b);
}
```

Here, `&a` does not load the value of `a`. It computes the memory address where `a` is stored.

This address is directly known by the compiler. If `a` is stored at address `0`, then `&a` can be generated with:

```
AFC r1 0x0
```

After this instruction, `r1` contains the address of `a`, not the value of `a`.

For local variables, the idea is the same, but the address is computed using the stack mechanism described in the Functions section. The important point for pointers is the same in both cases: `&a` produces an address, and `*b` uses an address to access memory.

Reading through a pointer uses `LDI`. For example, `*b` first loads the value of `b`, which is an address, then loads the memory at that address:

```
; load b
LOAD r1 @b

; load *b
LDI r1 r1
```

Writing through a pointer uses `STI`. For example:

```c
*b = 10;
```

becomes:

```
; r1 contains the address stored in b
; r2 contains 10
STI r1 r2
```

This is the exact reason `LDI` and `STI` are needed. With only `LOAD` and `STR`, the address must be hardcoded in the instruction. With pointers, the address is itself a value computed at runtime.

One difficulty was the `*` token. It is used both for multiplication and for pointer dereference:

```c
a * b
*p
```

Yacc normally gives a rule the precedence of its last terminal symbol, but here the same token has two different meanings. Multiplication should behave like a binary operator, while dereference should behave like a unary operator with higher priority. We discovered that we can solve this with the `%prec` token:

```yacc
tTIMES Expression %prec tDEREF
```

This tells yacc that this rule should not use the normal precedence of `*`, but the artificial precedence of `tDEREF`. As a result, expressions such as `*p + 1` and `*p * x` are parsed correctly.

### Functions

Functions are defined before `main` with the following syntax:

```c
function add(a, b) {
    return a + b;
}

main {
    print(add(2, 3));
}
```

At first, function calls only needed two special registers: `ra` for the return address, and `r0` for the return value. The caller placed the return address in `ra`, jumped to the function, and the function placed its result in `r0`. This works for very simple functions, but it is not enough for nested calls, recursion, or local variables. For that, each function call needs its own stack frame.

At this point, the full set of reserved registers becomes useful:

```c
#define REG_RETURN 0 // return value
#define REG_TMP 12 // temporary scratch register
#define REG_SP 13 // stack pointer
#define REG_FP 14 // frame pointer
#define REG_RA 15 // return address
```

`r0` is reserved for function return values, `r12` remains the compiler scratch register, `sp` points to the current top of the stack, `fp` points to the base of the current function frame, and `ra` stores the return address of the current call.

At this point, we changed how the symbol table worked. The variables no longer have a static slot on the symbol table, but rather an offset from the frame pointer.

```c
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
    int temp_symbol_size; // used in old memory-oriented compiler

} symbol_table_t;
```
But this is for variables declared inside a function body. We added the option to declare variables in the top section of the programs, counting as global variables, which are stored in the beginning of our symbol table.
The local ones are saved as offset from the frame pointer (stack), which starts at the top of memory and grows downward.

We defined two types of storage for variables:

```c
typedef enum {
    SYMBOL_GLOBAL,
    SYMBOL_LOCAL
} symbol_storage_t;
```

When leaving a block, the compiler restores the `SYMBOL_LOCAL` symbol table to the size it had when entering the block, so variables declared inside that block are not visible outside it.

For stack frames, the same idea is also applied to local stack slots. When entering a block, the compiler remembers the current `next_local_offset` in `local_offset_base`. When leaving the block, it pops the local variables declared inside that block from the stack, restores the symbol table to `symbol_table_base`, and restores `next_local_offset` to `local_offset_base`. This keeps block-local variables scoped correctly and avoids keeping unused stack slots alive after the block ends.

At the beginning of a function, the compiler adds a prefix:

```
STI sp ra   ; push return address to stack
SUB sp sp 1 ; move stack pointer down
STI sp fp   ; push old frame pointer to stack
SUB sp sp 1 ; move stack pointer down
COP fp sp   ; start the new frame
```

In the real assembly, the `1` is loaded into `r12` first, but the idea is the same. After this prefix, the function has its own `fp`, and local variables can be addressed as `fp - offset`.

Parameters are passed by the caller on the stack. For a call such as:

```c
add(2, 3)
```

the caller computes each argument into a register, pushes the argument values, sets `ra`, and jumps to the function:

```
AFC r1 0x2
AFC r2 0x3
STI sp r1    ; push first argument
SUB sp sp 1
STI sp r2    ; push second argument
SUB sp sp 1
AFC ra return_address ; set return address
J add ; call method
```

When the function starts, the arguments are below the saved `ra` and `fp`. The compiler copies them into normal local variable slots. This means parameters can be treated exactly like local variables inside the function body:

```c
function add(a, b) {
    return a + b;
}
```

Inside the function, `a` and `b` are stored in the symbol table as local symbols with offsets from `fp`.

At the end of a function, the compiler adds a suffix:

```
COP sp fp   ; discard local variables
ADD sp sp 1 ; go back to saved fp which was at fp + 1
LDI fp sp   ; restore old fp
ADD sp sp 1 ; go back to saved ra
LDI ra sp   ; restore return address
JI ra       ; return to caller
```

The return value is placed in `r0` before this suffix:

```
COP r0 r1
```

where `r1` is the register containing the expression returned by the function.

At the end of every function, the compiler also adds a fallback return. This fallback sets `r0` to `0` and emits the normal function suffix. If the function already executed an explicit `return`, this fallback code is not reached because the explicit return already jumps back to the caller. If execution reaches the end of the function without a return statement, the function therefore behaves like `return 0;`.

Function calls can also be nested:

```c
print(add(1, add(2, 3)));
```

For this reason, the compiler stores function call arguments as a stack of argument lists. The first level may be collecting arguments for the outer `add`, then a second level is created for the inner `add`. When the inner call finishes, its result becomes one of the arguments of the outer call.

Before jumping to a function, the compiler also saves registers that are currently in use. This is necessary because the called function is free to reuse general registers. After the call returns, those saved registers are restored from the stack, and the return value is copied from `r0` into a normal register.

### Web app interpreter

The web app is a small browser-based interpreter for the assembly produced by the compiler. It can load the textual assembly or the binary output, decode the instructions, and execute them one by one.

The interface is intentionally simple. It shows the program on one side, the registers and memory on the other side, and a small output area for `PRI` instructions. The user can reset the machine, execute one instruction at a time, or run the program continuously.

The interpreter keeps the same machine state as the microprocessor model:

```
PC      ; program counter
r0-r12  ; general and special-purpose registers
sp      ; stack pointer
fp      ; frame pointer
ra      ; return address
memory  ; data memory
```

At each step, it reads the instruction at `PC`, executes its effect in JavaScript, updates registers or memory, then moves `PC` to the next instruction. For jumps, it updates `PC` directly instead of incrementing it normally. This made the web app useful for debugging generated assembly, especially stack behavior for function calls and pointer behavior for `LDI` and `STI`.

## Final grammar and language

### BNF Representation

The following BNF is a simplified representation of the final language:

```bnf
<program> ::= <global-declaration-list> <function-list> <main-method>

<global-declaration-list> ::= empty
                            | <global-declaration-list> <variable-declaration>

<function-list> ::= empty
                  | <function-list> <function-definition>

<function-definition> ::= "function" identifier "(" <parameter-list> ")" <block>

<parameter-list> ::= empty
                   | <non-empty-parameter-list>

<non-empty-parameter-list> ::= identifier
                             | <non-empty-parameter-list> "," identifier

<main-method> ::= "main" <block>

<block> ::= "{" <statement-list> "}"

<statement-list> ::= empty
                   | <statement> <statement-list>

<statement> ::= <block>
              | <variable-declaration>
              | <variable-assignment>
              | <pointer-assignment>
              | <return-statement>
              | <print-statement>
              | <expression-statement>
              | <if-statement>
              | <while-statement>

<variable-declaration> ::= "let" identifier "=" <expression> ";"
                         | "let" <identifier-list> ";"

<identifier-list> ::= identifier
                    | identifier "," <identifier-list>

<variable-assignment> ::= identifier "=" <expression> ";"

<pointer-assignment> ::= <pointer-target> "=" <expression> ";"

<pointer-target> ::= "*" <pointer-operand>

<pointer-operand> ::= identifier
                    | "(" <expression> ")"
                    | "*" <pointer-operand>

<return-statement> ::= "return" <expression> ";"

<print-statement> ::= "print" "(" <expression> ")" ";"

<expression-statement> ::= <expression> ";"

<if-statement> ::= "if" "(" <expression> ")" <block>
                 | "if" "(" <expression> ")" <block> "else" <block>
                 | "if" "(" <expression> ")" <block> "else" <if-statement>

<while-statement> ::= "while" "(" <expression> ")" <block>

<argument-list> ::= empty
                  | <non-empty-argument-list>

<non-empty-argument-list> ::= <expression>
                            | <non-empty-argument-list> "," <expression>

<expression> ::= number
               | "true"
               | "false"
               | identifier
               | identifier "(" <argument-list> ")"
               | "&" identifier
               | "*" <expression>
               | "!" <expression>
               | "(" <expression> ")"
               | <expression> "||" <expression>
               | <expression> "&&" <expression>
               | <expression> "==" <expression>
               | <expression> "!=" <expression>
               | <expression> ">" <expression>
               | <expression> "<" <expression>
               | <expression> ">=" <expression>
               | <expression> "<=" <expression>
               | <expression> "+" <expression>
               | <expression> "-" <expression>
               | <expression> "*" <expression>
               | <expression> "/" <expression>
```

### Program Layout

A complete program is organized in three parts:

```c
let global_value = 3;

function square(x) {
    return x * x;
}

function add_square(a, b) {
    return square(a) + square(b);
}

main {
    let result = add_square(global_value, 4);
    print(result);
}
```

Global variables are declared first. Functions are defined after the global declarations and before `main`. The `main` block is the entry point of the program. When the compiler starts generating code, it first emits code to initialize all global variables, then a jump instruction that jumps over all functions directly to the entrypoint `main`.

### Discussion & Limitations

The project is intentionally small, so several limitations remain.

- There is no type system. All values are treated as machine words, so integers, booleans, and pointers are represented in the same way. This keeps the compiler simple, but it also means there is no type checking for expressions such as dereferencing a value that is not really a pointer.

- The register allocator is simple. Binary operations reuse the left operand register as the destination. This produces compact code, but it can overwrite a clean cached variable value and force the compiler to reload that variable later. The allocator also does not perform liveness analysis, so it does not know precisely when a variable will be needed again.

- Function arguments are limited by `MAX_FUNCTION_PARAMETERS`, and nested function calls are limited by `MAX_CALL_DEPTH`. These limits are fixed-size arrays in the compiler implementation.

- There are no arrays or structures. Pointers can address memory cells, but there is no syntax for indexing an array or declaring aggregate data.

- There is no heap allocation. All local variables are stored on the stack, and global variables are stored at fixed addresses.

- There is no `break` or `continue` in loops, and there is no `for` loop. Iteration is done with `while`.

- There is no unary minus. Negative values can still appear as results of subtraction, but there is no direct syntax such as `-1` in the grammar.

- Finally, the compiler performs very little error recovery. Most errors are reported as syntax errors or simple semantic errors such as an unknown variable or an unknown function.
