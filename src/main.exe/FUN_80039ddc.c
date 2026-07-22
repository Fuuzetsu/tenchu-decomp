#include "common.h"
#include "main.exe.h"

/*
 * FUN_80039ddc (0x80039ddc, 0x1D4 bytes) — ground-following raycast between
 * two points. Calls CGetLevel and GetVectorLength directly, both EFFECT.C
 * functions (retail address sits right after CGetLevel's own), so this is
 * almost certainly the same original TU. No PSX.SYM prototype is recorded
 * for it.
 *
 * Steps from `from` toward `to` in units of 0x1F4000 / GetVectorLength(delta)
 * (i.e. a fixed number of world-space steps along the ray, up to 0x1000 of
 * them), sampling the ground level at each step via CGetLevel; stops as soon
 * as the ground rises above the ray's height there (CGetLevel's result <
 * the step's own y). Writes the last step that stayed above ground into
 * `*out` (skipped when NULL) and returns the step index reached — callers
 * use it as a crude "how far can you see/walk" measure (SetCameraMode
 * treats a return > 0x7FF as "clear").
 *
 * Confirmed 4-parameter signature (Ghidra's own decompilation only shows 3):
 * SearchItemTarget2.c and SetCameraMode.c already document the trailing
 * `u32 flag` argument (passed straight through to CGetLevel's own trailing
 * flag parameter) that Ghidra's decompiler never reads in the body — the
 * raw .s spills a3 to its own stack home (sp+0x64) and reloads it every
 * loop iteration purely to hand to CGetLevel: the "Ghidra under-counts
 * trailing args" class (cookbook, Expressions).
 *
 * Matching notes:
 *  - Division by GetVectorLength's runtime result needs maspsx's
 *    --expand-div guards (ASPSX's break 7 / break 6); no explicit trap()
 *    call belongs in the C — maspsx inserts the guards around a plain `/`
 *    automatically (see IsVisible.c).
 *  - Top-test-then-unconditional-back-jump loop shape (loop.c's
 *    duplicate_loop_exit_test does not fire: the first insn after the loop
 *    note is already a condjump) — write `while (1) { if (!(cond)) break;
 *    ...; t += step; }`, not a plain `for`.
 *  - `x`/`y` (from `from`) get real stack homes and are reloaded every
 *    iteration, while `z` stays in a callee-saved register the whole
 *    function — a register-pressure artifact of all 9 callee-saved
 *    registers (s0-s7, fp) being live across the loop; no source lever
 *    needed, it falls out of plain separate locals.
 *  - `lx`/`ly`/`lz`/`hint`/`t` are assigned in that order right before the
 *    loop (asm order), not the `hint=0` first / lx,ly,lz second order
 *    Ghidra's SSA rendering suggests.
 *  - Each per-axis step needs a SEPARATE raw-product temp (`rawx`/`rawy`/
 *    `rawz`) distinct from the final `tx`/`ty`/`tz`, even though Ghidra
 *    renders both roles through the SAME reassigned variable
 *    (`iVar2 = dx*t; if (iVar2<0) iVar2+=0xFFF; iVar2 = x + (iVar2>>0xC);`).
 *    Reusing one variable for both roles gives it ONE pseudo for its whole
 *    life (gcc never splits a live range) — since the final value is used
 *    across the CGetLevel call and the loop back-edge, that single pseudo
 *    gets promoted to a callee-saved register for the RAW computation too
 *    (wrong: 47 bytes, a register rotation across the whole per-axis
 *    block). The target keeps each raw product in a short-lived
 *    caller-saved register ($v0/$v1/$a3) and only the final add's result
 *    in the persistent callee-saved one ($s3/$s0/$s1) — proof the source
 *    had two variables, and Ghidra's reassignment was just its own
 *    decompiler coalescing an SSA rename, not evidence of one C variable
 *    (cookbook: "trust the assembly over Ghidra's statement order").
 */

extern s32 GetVectorLength(s32 dx, s32 dy, s32 dz);
extern s32 CGetLevel(AreaNodeType **hint, s32 x, s32 y, s32 z, u32 flag);

s32 FUN_80039ddc(VECTOR *from, VECTOR *to, VECTOR *out, u32 flag)
{
    AreaNodeType *hint;
    s32 x, y, z;
    s32 dx, dy, dz;
    s32 step;
    s32 t;
    s32 lx, ly, lz;
    s32 tx, ty, tz;
    s32 rawx, rawy, rawz;

    x = from->vx;
    dx = to->vx - x;
    y = from->vy;
    dy = to->vy - y;
    z = from->vz;
    dz = to->vz - z;
    step = 0x1F4000 / GetVectorLength(dx, dy, dz);
    lx = x;
    ly = y;
    lz = z;
    hint = 0;
    t = step;
    while (1) {
        if (!(t < 0x1000)) break;
        rawx = dx * t;
        if (rawx < 0) rawx += 0xFFF;
        rawy = dy * t;
        tx = x + (rawx >> 0xC);
        if (rawy < 0) rawy += 0xFFF;
        rawz = dz * t;
        ty = y + (rawy >> 0xC);
        if (rawz < 0) rawz += 0xFFF;
        tz = z + (rawz >> 0xC);
        if (CGetLevel(&hint, tx, ty, tz, flag) < ty) break;
        lx = tx;
        ly = ty;
        lz = tz;
        t += step;
    }
    if (out != 0) {
        out->vx = lx;
        out->vy = ly;
        out->vz = lz;
    }
    return t;
}
