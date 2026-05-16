Register resets are handled inside the codegen control-flow helpers:

- `emit_jmf_placeholder()` resets after emitting a conditional branch.
- `emit_jmp_placeholder()` resets after emitting an unconditional jump.
- `patch_jmf_relative()` resets at branch/join patching.
- `patch_jmp_relative()` resets at join patching.
- `emit_loop_back_jump()` resets after the loop-back jump.
- `end_block()` resets after leaving a block.
- `while` resets before compiling the condition, so each iteration reloads truth from memory.

This is needed because the register table is only the compiler's guess about what each register contains. In straight-line code, this cache is useful. After branches, loops, jumps, and block exits, control flow may have changed memory in a way the current path cannot know. At that point, memory becomes the source of truth again, so the compiler forgets cached register contents and reloads variables when needed.

The current register allocation strategy is intentionally simple: binary expressions reuse the left operand register as the destination register. This keeps expression generation compact, but it can overwrite a clean cached variable value. For example, in b = a + 1, if a is already cached in r1, the compiler may emit ADD r1 r1 r2, replacing a with the result. A smarter allocator could preserve clean variable registers and compute results in another register, allowing later expressions such as c = a + 2 to reuse the cached value of a without reloading it from memory.

Limitations:
- if in an expression is long enough so that it uses all general registers, it will give an error
- the register system does not know if a variable is needed later on, so it stores it back to memory right away after being done