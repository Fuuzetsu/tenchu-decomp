#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern short RefrectMove[16][2];
 * END PSX.SYM */

/*
 * FUN_80030644 (0x80030644, 0x128 bytes) — nudges a position's x/z (a VECTOR,
 * only vx/vz are touched, vy is left alone) away from an area-map wall: it
 * samples the area map at `pos` with GetAreaMapVector, bails out if the
 * sample is off-map (`level == INT_MIN`) or reports no wall (`vector == 0`),
 * then re-samples at HALF the requested `amount` and uses that sample's
 * `vector` code instead if it also came back non-wall (falling back to the
 * first sample's code and the halved amount). `RefrectMove[vector]` is a
 * 16-entry `[xAdjust, zAdjust]` sign table: each component's sign (not
 * magnitude — only compared `<1`/`>-1`) decides whether to nudge that axis
 * by `+amount`, `-amount`, or leave it alone. Called once from the
 * still-unmatched CameraDirection.c (`FUN_80030644(&CamLoc, 1000);`) to push
 * a candidate camera position off a wall it's testing against — a camera
 * wall-avoidance helper, not player collision. No candidate name in
 * reference/psxsym-candidates.tsv; not in the demo's PSX.SYM (CameraDirection
 * itself IS in the demo per CAMERA.C:898, so this helper is either a
 * retail-only addition or was inlined/differently-named in the demo build).
 *
 * The out-parameter GetAreaMapVector fills is NOT the 16-byte `MapVector`
 * from reference/psxsym-types.h (game_types.h's own shared `MapVector`
 * deliberately stays 8 bytes, level+height only, per its header comment) —
 * GetAreaMapVector.c's own Ghidra decompilation writes through it as two
 * back-to-back VECTORs (`pos[1].vx`/`pos[1].vy`, offsets 0x10/0x14, hold
 * FieldArea/FieldIndex pointers no caller reads), and StickonCheck.c's
 * comment independently names it "a much larger 0x20-byte record". This
 * function's own stack layout (the two `GetAreaMapVector` out-buffers sit
 * 0x18 bytes apart, sp+0x18 and sp+0x30) only needs a LOCAL, offsets-only
 * struct sized to reproduce that spacing — same convention as StickonCheck's
 * own local `AreaMapVectorResult` truncated view.
 *
 * STATUS: NON_MATCHING — 87 of 296 bytes differ (72 vs 74 instructions,
 * ours 2 short). The stack layout above IS proven correct (v1/v2's
 * level/vector offsets match exactly, zero diff in that whole region).
 * The residual is entirely in the RefrectMove-lookup tail:
 *  - `RefrectMove[vec][0]` (the "x" entry) re-read inline (not cached) at
 *    both its comparisons, cast `(s16)` each time — matches Ghidra's own
 *    literal re-reference (it never names this one, only the "z" entry gets
 *    `sVar1`). Even so, our cc1 folds the `lhu`-then-sign-extend into a
 *    single `lh` (semantically identical bit pattern, one instruction)
 *    whenever the value's ONLY use is a nearby signed compare; the target
 *    keeps a separate `lhu` + `sll`/`sra` pair. Tried: naming it a variable
 *    (u16 cast at compare, s16 cast at assign, s16 straight assign — all
 *    three fold to `lh` the same way); reading through an explicit
 *    `struct { u16 x, z; } *r = &RefrectMove[vec];` pointer instead of
 *    2D-array indexing (WORSE — 134 bytes differ; a pointer dereference
 *    doesn't even keep the two lhu's in one combined load, cc1 re-issues a
 *    fresh `lh`/`lh` per field through the pointer).
 *  - The "z" entry's sign-extension (`sll v0,a0,0x10` / `sra v0,v0,0x10`)
 *    appears TWICE in the target at different addresses (0x8003070c and
 *    0x80030720) — cookbook "Shared tails": a cheap expression reached by
 *    two predecessors (the `xAdj==0` skip-store path vs. the store-happened
 *    merge path) gets duplicated into both rather than computed once at the
 *    join. Our code (whether `zAdj` is cast at the read or deferred to each
 *    compare) only ever emits it once — the natural join point after
 *    `skip_x:` does not trigger cc1's tail-duplication here.
 *  - `tools/autorules.py` reports `half: s32→s16` as a "win" (26→25 on its
 *    own score) — REJECTED: `tools/matchdiff.py` shows it makes things much
 *    WORSE (158 vs 87 bytes differ) by introducing an extra 3-instruction
 *    sign-extend/scale in the `amount/2` computation. Textbook case of the
 *    cookbook's "autorules moves the wrong way against matchdiff" warning.
 *  - Root cause not fully pinned: likely the ORIGINAL source shapes the
 *    x/z lookup and the merge-point differently than a straight nested-if
 *    reading a 2D array (maybe via a helper macro/inline function per axis,
 *    given the x/z handling is otherwise a near-exact structural mirror of
 *    itself) — not chased further within budget. `tools/permute.py` was
 *    started but not completed (background run killed to unblock other
 *    functions in this batch; the residual is NOT same-length as the
 *    target, 72 vs 74 instructions, so it is not a strong permuter
 *    candidate per the cookbook's iteration protocol anyway).
 */
typedef struct
{
    s32 level; /* +0x00 */
    u8 pad4[8];
    u8 vector; /* +0x0c */
    u8 pad13[0xb];
} MapVectorResult; /* 0x18 — offsets-only view; true record is bigger, see above */

extern void *GlobalAreaMap;
extern u16 RefrectMove[16][2];
extern s32 GetAreaMapVector(void *area, void *mvp, void *pos, s32 wide, s32 mode);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_80030644", FUN_80030644);
#else
void FUN_80030644(VECTOR *pos, s32 amount)
{
    MapVectorResult v1;
    MapVectorResult v2;
    s32 half;
    u32 vec;
    u16 zAdj;
    s32 newx;

    GetAreaMapVector(GlobalAreaMap, &v1, pos, amount, 0);
    if (v1.level == (s32)0x80000000)
    {
        return;
    }
    if (v1.vector == 0)
    {
        return;
    }
    half = amount / 2;
    GetAreaMapVector(GlobalAreaMap, &v2, pos, half, 0);
    vec = v2.vector;
    if (vec == 0)
    {
        vec = v1.vector;
        amount = half;
    }
    zAdj = RefrectMove[vec][1];
    if ((s16)RefrectMove[vec][0] < 1)
    {
        if (-1 < (s16)RefrectMove[vec][0])
        {
            goto skip_x;
        }
        newx = pos->vx - amount;
    }
    else
    {
        newx = pos->vx + amount;
    }
    pos->vx = newx;
skip_x:
    if ((s16)zAdj < 1)
    {
        if (-1 < (s16)zAdj)
        {
            return;
        }
        amount = pos->vz - amount;
    }
    else
    {
        amount = pos->vz + amount;
    }
    pos->vz = amount;
}
#endif
