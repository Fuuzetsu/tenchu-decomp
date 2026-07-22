#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawPause(void);
 *     INFOVIEW.C:1224, 35 src lines, frame 304 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     stack sp+16     struct DISPENV o_disp
 *     stack sp+40     struct DRAWENV o_draw
 *     stack sp+136    struct DRAWENV n_draw
 *     stack sp+232    struct POLY_GT4 ply
 *
 * Globals it touches, as the original declared them:
 *     extern enum TSystemFlag SystemFlag;
 * END PSX.SYM */

/*
 * DrawPause (0x8004b18c, 0x1C8 bytes) — draws the pause-screen "wobbling
 * brightness ring" overlay: swaps in a screen-sized clip/offset draw
 * environment (from the current display env), draws a pulsing quad using
 * the current stage's title image, then restores the original draw env.
 * Called by PauseProc (already matched) with the pause frame counter.
 *
 * MATCH. Matching notes (all verified against the original bytes):
 *  - PSX.SYM records `static void DrawPause(void);` (no parameter) but the
 *    body reads $a0 meaningfully (Ghidra's "in_a0" / m2c's arg0) for the
 *    sin-based wobble, and the ALREADY-MATCHED caller PauseProc.c declares
 *    `extern void DrawPause(int frame);` and calls `DrawPause(cnt);` — the
 *    asm wins over the stale void prototype (cookbook: "the asm wins").
 *  - `n_draw = o_draw;` is a PLAIN WHOLE-STRUCT ASSIGNMENT, not Ghidra's
 *    hand-unrolled field-by-field copy: DRAWENV is 0x5C bytes, and cc1's
 *    emit_block_move for a word-aligned struct that size compiles to
 *    EXACTLY this shape — a 5-iteration 4-word (0x10-byte) loop over the
 *    aligned 0x50-byte bulk, then a 3-word (0xC-byte) tail (cookbook
 *    "Stack objects": DeleteConflict's 0x78-byte struct assignment is the
 *    same mechanism at a different size). Ghidra's pDVar17/pDVar18 walking
 *    pointers and named temps are its own rendering of that generated loop,
 *    not source structure.
 *  - `n_draw.clip = o_disp.disp;` (a RECT-to-RECT struct assignment) is
 *    ALSO a plain struct copy: RECT is four `s16` fields (align 2), so cc1
 *    emits the align-2 lwl/lwr + swl/swr block-move idiom for it — matches
 *    m2c's own "(unaligned s32)" tag on this exact copy (cookbook "Stack
 *    objects": SVECTOR's align-2 lwl/lwr pattern is the same mechanism).
 *    `n_draw.ofs[0]/[1] = o_disp.disp.x/y;` stay two plain scalar copies
 *    (ofs is a separate 2-`s16` array, not part of the RECT copy).
 *  - The frame-based wobble angle `(s16)frame * 0x44` is computed ONCE into
 *    a named temp and reused for both `rsin` calls (`t` and `t + 0x200`,
 *    a quarter-turn apart) — m2c's own `temp_s0` shows this shared value.
 *  - RECT/DRAWENV/DISPENV and POLY_GT4 use their canonical PsyQ layouts,
 *    independently confirmed by reference/psxsym-types.h.
 *  - `t = (s16)frame * 0x44;` needed a `do{}while(0)` wrapper (byte-neutral
 *    scheduling lever, permuter-found) to make cc1 build the multiply chain
 *    into its own scratch register and copy the result into the persistent
 *    one, matching the target — without the wrapper, cc1 folds the
 *    assignment straight into the first `rsin` call's argument setup (`a0`)
 *    and only copies to the callee-saved home afterward, an 11-instruction
 *    register-identity swap for no byte-length change.
 *  - `(v >> 0xC) + 0x80` (permuter-found): the literal `+0x80` compiles to
 *    `addiu` with the immediate encoded as `-128` (byte-equivalent mod 256,
 *    since the result only ever feeds an 8-bit store) instead of the
 *    target's `+128` — going through a named `s32 bias = 0x80;` local used
 *    at both addition sites keeps the literal's encoding as `+128`. Neither
 *    an explicit narrowing cast on the shift result nor writing straight to
 *    the struct field (skipping a scratch var) affected this; only routing
 *    the ADDEND itself through a named variable did.
 */
extern TSystemFlag SystemFlag;

extern GsIMAGE *GetImage(s32 id);
extern void SetupImageToPolyGT4(GsIMAGE *image, POLY_GT4 *ply, s32 x, s32 y);

void DrawPause(int frame)
{
    DISPENV o_disp;
    DRAWENV o_draw;
    DRAWENV n_draw;
    POLY_GT4 ply;
    GsIMAGE *image;
    s32 t;
    s32 v;
    s32 bias;
    u8 far_col;

    if ((SystemFlag & SYSFLAG_DEBUGPRINT) == 0)
    {
        GetDrawEnv(&o_draw);
        GetDispEnv(&o_disp);
        n_draw = o_draw;
        n_draw.clip = o_disp.disp;
        n_draw.ofs[0] = o_disp.disp.x;
        n_draw.ofs[1] = o_disp.disp.y;
        PutDrawEnv(&n_draw);
        image = GetImage(0x3D);
        SetupImageToPolyGT4(image, &ply, (s16)(0xA0 - image->pw * 2), (s16)(0x78 - (image->ph >> 1)));
        do
        {
            t = (s16)frame * 0x44;
        } while (0);
        bias = 0x80;
        v = rsin(t) * 0x7D;
        if (v < 0)
        {
            v = v + 0xFFF;
        }
        far_col = (v >> 0xC) + bias;
        v = rsin(t + 0x200) * 0x7D;
        if (v < 0)
        {
            v = v + 0xFFF;
        }
        ply.r0 = (v >> 0xC) + bias;
        ply.g0 = ply.r0;
        ply.b0 = ply.r0;
        ply.r1 = ply.r0;
        ply.g1 = ply.r0;
        ply.b1 = ply.r0;
        ply.r2 = far_col;
        ply.g2 = far_col;
        ply.b2 = far_col;
        ply.r3 = far_col;
        ply.g3 = far_col;
        ply.b3 = far_col;
        DrawPrim(&ply);
        PutDrawEnv(&o_draw);
    }
}
