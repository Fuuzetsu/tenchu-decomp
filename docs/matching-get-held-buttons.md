# Case study: byte-matching `get_held_buttons`

A worked example of the decomp loop — and of why "the C is correct" is not the
same as "the bytes match". This function is *arithmetic-perfect* but not yet
byte-identical; it's a good illustration of what's easy and what's hard.

## The target

`get_held_buttons` (vram `0x8001b260`, 22 instructions). Disassembly:

```mips
addiu  $sp, $sp, -0x18
sw     $s0, 0x10($sp)
sw     $ra, 0x14($sp)
jal    FUN_8001ada4
 addu  $s0, $a0, $zero        # s0 = arg0  (saved across the call)
sra    $v0, $s0, 4            # OUTER index = arg0 >> 4   (signed)
sll    $a0, $v0, 3            #  v0*8
subu   $a0, $a0, $v0          #  v0*7
sll    $a0, $a0, 3            #  v0*56           → a0 = outer*56
andi   $s0, $s0, 0x3          # INNER index = arg0 & 3   (s0 reused!)
sll    $v0, $s0, 3
subu   $v0, $v0, $s0
sll    $v0, $v0, 1            #  s0*14           → v0 = inner*14
lui    $v1, %hi(HELD_BUTTONS)
addiu  $v1, $v1, %lo(HELD_BUTTONS)   # v1 = &HELD_BUTTONS  (full address)
addu   $v0, $v0, $v1          # v0 = inner*14 + &HELD_BUTTONS
addu   $a0, $a0, $v0          # a0 = outer*56 + (inner*14 + &HELD_BUTTONS)
lhu    $v0, 0x0($a0)          # return *(u16*)a0
lw     $ra, 0x14($sp)
lw     $s0, 0x10($sp)
jr     $ra
 addiu $sp, $sp, 0x18
```

## Reading it

The strides `*14` and `*56` (`= 4 × 14`) say `HELD_BUTTONS` is a `[N][4]` table
of a **14-byte struct**, indexed `[arg0 >> 4][arg0 & 3]`, returning the first
`u16`. `m2c` agrees:

```c
return *(((arg0 >> 4) * 0x38) + (((arg0 & 3) * 0xE) + &HELD_BUTTONS));   // 0x38=56, 0xE=14
```

So the natural C (in `main.exe.h`: a 14-byte `controller_input` with a
`buttons_held held` first field, `extern controller_input HELD_BUTTONS[][4]`):

```c
buttons_held get_held_buttons(s32 arg0)
{
    FUN_8001ada4();
    return HELD_BUTTONS[arg0 >> 4][arg0 & 3].held;
}
```

This compiles to the **correct arithmetic** — the right strength-reduced
`*14`/`*56` multiplies, the `lhu`, the prologue, the `jal`, the `move $s0,$a0`
delay slot — all match.

## What doesn't match (yet)

Two coupled instruction-scheduling / register-allocation differences remain:

1. **Index order.** The target computes the *outer* index first, because it
   *reuses `$s0`* for the inner index (`andi $s0,$s0,3` clobbers the saved
   `arg0`), which forces outer to be computed while `$s0` still holds `arg0`. Our
   build keeps `arg0` in `$s0` and computes the *inner* index first.
2. **Address form + registers.** The target materialises the full
   `&HELD_BUTTONS` (`lui`+`addiu`) grouped with the inner term, and holds
   `outer*56` in `$a0` (the recycled argument register). Our build folds `%lo`
   into the load and uses `$v0`/`$v1`.

Both are outcomes of gcc 2.8.1's register allocator. About **25** semantically-
equivalent source formulations were tried (pointer arithmetic, explicit
associations, temporaries, `int`/`unsigned`/`register` arg types, complete array
dimensions, `+0` perturbations) — several reproduced *one* of the two properties,
none reproduced *both* at once. This is exactly what the **decomp-permuter** is
for: it randomly perturbs the code to nudge the allocator until the bytes line up.

Note this is **not** a maspsx problem — there's no division, no `$gp`-relative
access (`HELD_BUTTONS` at `0x800be6d0` is far outside gp's ±32 KB range), and no
`nop` scheduling here. maspsx would leave these bytes unchanged.

## Fast iteration harness

To iterate without a full `./Build`, compile just this object with the exact
pipeline and diff the mnemonics against the target:

```bash
cpp <cppFlags> -I src/main.exe src/main.exe/get_held_buttons.c out.c
tools/cc1-281 <ccFlags> < out.c > out.s
mipsel-unknown-linux-gnu-as <asFlags> -o out.o out.s
mipsel-unknown-linux-gnu-objdump -d out.o    # compare to the target above
```

(The exact flag lists are in `shake/src/Build.hs`.) For scoring against the real
object, wire `diff.py` (`asm-differ`, already in the devShell) via
`diff_settings.py`.

## Status

Left as an `INCLUDE_ASM` stub with the analysis in `src/main.exe/get_held_buttons.c`
so `./Build check` stays green. Next: run the decomp-permuter on it, or move to a
function that byte-matches more directly (e.g. `initialise_font` already does).
