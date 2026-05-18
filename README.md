Register resets are handled inside the code control-flow helpers:

- `emit_jmf_placeholder()` resets after emitting a conditional branch.
- `emit_jmp_placeholder()` resets after emitting an unconditional jump.
- `patch_jmf_relative()` resets at branch/join patching.
- `patch_jmp_relative()` resets at join patching.
- `emit_loop_back_jump()` resets after the loop-back jump.
- `end_block()` resets after leaving a block.
- `while` resets before compiling the condition, so each iteration reloads truth from memory.

This is needed because the register table is only the compiler's guess about what each register contains. In straight-line code, this cache is useful. After branches, loops, jumps, and block exits, control flow may have changed memory in a way the current path cannot know. At that point, memory becomes the source of truth again, so the compiler forgets cached register contents and reloads variables when needed.

The current register allocation strategy is intentionally simple: binary expressions reuse the left operand register as the destination register. This keeps expression generation compact, but it can overwrite a clean cached variable value. For example, in b = a + 1, if a is already cached in r1, the compiler may emit ADD r1 r1 r2, replacing a with the result. A smarter allocator could preserve clean variable registers and compute results in another register, allowing later expressions such as c = a + 2 to reuse the cached value of a without reloading it from memory.

Function call arguments are stored as a small stack of argument lists because function calls can be nested. In an expression such as `add(1, mul(2, 3))`, the compiler starts collecting arguments for `add`, then has to temporarily collect another list for `mul` before finishing `add`. The call depth selects which function call is currently being prepared, and the argument index selects which argument inside that call is being stored.

Limitations:
- if in an expression is long enough so that it uses all general registers, it will give an error
- the register system does not know if a variable is needed later on, so it stores it back to memory right away after being done

Function prefix and suffix:

At the beginning of a function, the compiler adds a prefix to save the caller state and create a new stack frame. The stack starts at the top of memory and grows downward. First, the return address register `ra` is stored at `MEM[sp]`, then `sp` is decremented. Then the old frame pointer `fp` is stored at the new `MEM[sp]`, and `sp` is decremented again. Finally, `fp` is copied from `sp`, so the function has its own frame base. After this, local variables are addressed relative to `fp`.

```
STI sp ra   ; save return address at memory[sp]
sp--        ; move stack pointer down
STI sp fp   ; save old frame pointer at memory[sp]
sp--        ; move stack pointer down
COP fp sp   ; new frame starts here
```

```
COP sp fp   ; discard local variables, go back to frame base
sp++        ; move to saved old fp
LDI fp sp   ; restore caller's fp
sp++        ; move to saved ra
LDI ra sp   ; restore return address
JI ra       ; jump back to caller
```

At the end of a function, the compiler adds a suffix to restore the caller state. It first copies `fp` back into `sp`, which discards the current function's local variables. Then it increments `sp` and reloads the old `fp` from memory. It increments `sp` again and reloads the saved return address into `ra`. Finally, it jumps indirectly to `ra`, returning execution to the caller.

%prec is used in the yacc grammar to manually assign a precedence to a rule. By default, yacc gives a rule the precedence of its last terminal symbol, but this is not always enough when the same token has different meanings. In our grammar, * can mean multiplication or pointer dereference. Multiplication uses the normal tTIMES precedence, while dereference should behave like a unary operator with higher priority. For this reason, the dereference rule is written as tTIMES Expression %prec tDEREF, which tells yacc to use the precedence of the artificial token tDEREF instead of treating it like multiplication. This avoids ambiguity in expressions such as *p + 1 or *p * x.

Synchronous instruction ROM and stalls:

The instruction ROM is synchronous, so its output does not change immediately when `pc` changes. The instruction visible on `instruction_line` belongs to the address requested on the previous clock edge. This matters during stalls. If the CPU only freezes `pc` and `LI/DI`, the ROM output can still contain a useful instruction that arrived during the stall. Without saving it, that instruction can be skipped, or the stall can end one cycle too early.

The fix is to save the whole 32-bit `instruction_line` when a stall starts:

```
signal instr_saved    : std_logic_vector(31 downto 0) := (others => '0');
signal has_saved      : std_logic := '0';
signal instr_selected : std_logic_vector(31 downto 0);
```

The decode stage should read from `instr_selected`, not directly from `instruction_line`:

```
instr_selected <= instr_saved when has_saved = '1' else instruction_line;

op_li <= instr_selected(31 downto 24);
a_li  <= instr_selected(23 downto 16);
b_li  <= instr_selected(15 downto 8);
c_li  <= instr_selected(7 downto 0);
```

Inside the main clocked pipeline process, when `stall = '1'`, save the instruction once, freeze `pc`, keep `LI/DI`, and inject a `NOP` bubble into `DI/EX`:

```
if stall = '1' then
    if has_saved = '0' then
        instr_saved <= instruction_line;
        has_saved <= '1';
    end if;

    pc <= pc;

    op_lidi <= op_lidi;
    a_lidi  <= a_lidi;
    b_lidi  <= b_lidi;
    c_lidi  <= c_lidi;

    op_diex <= OP_NOP;
    a_diex  <= (others => '0');
    b_diex  <= (others => '0');
    c_diex  <= (others => '0');
```

When the stall ends, consume the saved instruction before advancing the fetch again:

```
if has_saved = '1' then
    pc <= pc;
    has_saved <= '0';
else
    pc <= std_logic_vector(unsigned(pc) + 1);
end if;

op_lidi <= op_li;
a_lidi  <= a_li;
b_lidi  <= b_li;
c_lidi  <= c_li;
```

With this behavior, a stall does this:

```
PC      frozen
LI      saved if needed
LI/DI   frozen
DI/EX   NOP bubble
EX/MEM  continues
MEM/ER  continues
```
