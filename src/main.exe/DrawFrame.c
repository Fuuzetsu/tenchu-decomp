#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawFrame(struct tag_EffectSlot *ef);
 *     EFFECT.C:1044, 49 src lines, frame 104 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $s3       struct tag_EffectSlot * ef
 *     reg   $s1       struct FrameType * param
 *     reg   $s2       struct GsSPRITE * spr
 *     stack sp+16     struct SVECTOR scr
 *     stack sp+24     struct SVECTOR sv
 *     stack sp+40     struct MATRIX mat
 *     stack sp+72     long p
 *     stack sp+76     long flag
 *     reg   $v0       long x
 *     reg   $a1       long y
 *     reg   $a2       long z
 *     reg   $s0       struct SVECTOR * scr
 *     reg   $v1       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE sprFrame[4];
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/*
 * MATCH.
 *
 * DrawFrame (0x80035094, EFFECT.C:1044) — the frame/flash effect's per-frame
 * draw: picks the animation slot `sprFrame[param->count % 4]`, runs a
 * mode-driven countdown state machine (mode 0: white flash fading via
 * `count`, resetting to mode 1 at 0; mode 1: fade using the LOW BYTE of
 * `count` as the RGB level, self-disposing at 0), then projects `param`'s
 * position through the FUN_8003a148-style hint-or-camera-relative dispatch
 * and, if visible, scales/positions the picked `sprFrame` slot and
 * GsSortSprite's it with the same `[0, 0x4e1]` OTZ-derived clamp.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `param = &ef->param.frame;` (FrameType* at ef+4) — `count` (s16 @
 *    0x12) doubles as BOTH the countdown AND (its raw low BYTE, mode 1) the
 *    fade level — `effect.h`'s proven FrameType layout, no new struct.
 *  - `idx = param->count % 4;` is plain C `%` by the constant 4 — cc1's
 *    own round-toward-zero remainder expansion (bgez-guarded `+3`, `sra 2`,
 *    `sll 2`, `subu`) is automatic, no manual shift/mask needed.
 *  - The mode dispatch is a plain `if (mode==0) {...} else if (mode==1)
 *    {...}` — both bodies converge on the draw code below (no `else`
 *    tail), matching the target's goto-ladder-to-a-shared-continuation
 *    shape (`beqz mode,body0; beq mode,1,body1; j draw;`).
 *  - Mode 1's RGB fill reads the LOW BYTE of `count` directly —
 *    `*(u8 *)&param->count` (a `lbu` at count's own address, the low byte
 *    of the little-endian s16) — not a distinct union member; m2c's raw
 *    `(u8)temp_a1->unk12` confirms it is literally `count`'s own byte, no
 *    new field.
 *  - Both mode bodies test `if (param->count <= 0) {...}` AFTER storing
 *    the decremented `count` back (`count -= 1`/`count -= 29` reproduces
 *    the target's own `addiu -1`/`addiu -0x1D`; Ghidra's `-0x1D` text is
 *    the real encoded immediate here, unlike DrawBleed's `+0xff` — verify
 *    the encoded byte before trusting Ghidra's rendering either way).
 *  - The camera-relative projection is FUN_8003a148's exact `if (hint !=
 *    0) { Scratchpad GsGetLs/GsSetLsMatrix/RotTransPers } else {
 *    GetScreenPosition }` shape and polarity (hint!=0 is the fall-through).
 *  - `otz = scr.vz;` re-reads FRESH for the clamp (`t = scr.vz - 0x32; t =
 *    t >> 2;`), matching FUN_8003a148's own re-read (opposite of
 *    DrawBleed's reuse) — the target's asm shows a second, independent
 *    `lh` there.
 *  - `size * 300` is a plain constant multiply; cc1's own strength
 *    reduction produces the target's shift/subtract/shift sequence
 *    automatically (same as FUN_8003a148's `size * 300`) — but ONLY once
 *    `size` is captured into a plain `s32` local (not `s16`): a `s16 size`
 *    local fed by a same-width struct-field copy triggers the "pure
 *    narrowing copy loads lhu" rule even though `size` is later used in
 *    signed arithmetic (the `*300` multiply and this same address
 *    computation both need the sign), costing a spurious `sll/sra`
 *    re-widen the target doesn't have — widen the CAPTURING LOCAL, not
 *    just the read.
 *  - `px`/`py`/`pz`/`hint`/`size` must be captured into plain locals
 *    UPFRONT (right after the mode dispatch converges), matching the
 *    target's own grouped `lw`×4 + `lh` — the "N adjacent loads, no use
 *    between them, are source temps" rule (also lets `hint`/`size` survive
 *    the RotTransPers/GetScreenPosition call in a callee-saved register without
 *    a fresh reload after).
 *  - The scratchpad `px/py/pz` store is the FLAT `*(s16*)0x1F8000xx = ...`
 *    per-store macro cast (DrawTarget's idiom), NOT a named `SVECTOR *sv`
 *    pointer local (FUN_8003a148's idiom) — even though the shape looks
 *    identical to FUN_8003a148 otherwise (same hint-vs-camera-relative
 *    dispatch, same RotTransPers call reusing the pointer): a named `sv`
 *    here combines the three stores through one materialized base register
 *    instead of three independent one-line macros, 4 bytes off. Read the
 *    actual asm before assuming a sibling's exact idiom carries over.
 *  - The final `t = (scr.vz - 0x32) >> 2;` clamp base needs the subtraction
 *    and the shift as TWO separate statements (`t = scr.vz - 0x32; t = t >>
 *    2;`) — fused into one expression, cc1 reuses the subtraction's own
 *    register for the shift's destination too; split, the shift gets a
 *    fresh register, matching the target's `addiu v1,v0,-50` (dest != src)
 *    + `sra v1,v1,2` exactly (a 2-byte pure register tie, the cookbook's
 *    "sub-C-level early-stop" class — but this one had a real source
 *    lever: statement splitting, not a permuter/RTL escalation).
 *  - This TU divides by a runtime value (`size*300/otz`): needs
 *    `--expand-div` (Build.hs maspsxGpExterns' `extra` list + permute.py's
 *    MASPSX_EXTRA), same as FUN_8003a148/DrawSpriteXYZ.
 */
extern GsOT *OTablePt;
extern void GetScreenPosition(s32 x, s32 y, s32 z, s32 *out);

void DrawFrame(TEffectSlot *ef)
{
    FrameType *param = &ef->param.frame;
    GsSPRITE *spr;
    SVECTOR scr;
    s16 idx;
    s32 otz;
    s16 sc;
    s32 t;
    s32 pri;
    u8 rgb;
    s32 px, py, pz;
    GsCOORDINATE2 *hint;
    s32 size;

    idx = param->count % MaxFrames;
    spr = &sprFrame[idx];

    if (param->mode == 0)
    {
        goto mode0;
    }
    if (param->mode == 1)
    {
        goto mode1;
    }
    goto draw;
mode0:
    spr->b = 0x80;
    spr->g = 0x80;
    spr->r = 0x80;
    param->count = param->count - 1;
    if (param->count <= 0)
    {
        param->count = 0x80;
        param->mode = param->mode + 1;
    }
    goto draw;
mode1:
    rgb = *(u8 *)&param->count;
    spr->b = rgb;
    spr->g = rgb;
    spr->r = rgb;
    param->count = param->count - 29;
    if (param->count <= 0)
    {
        ef->proc = 0;
    }
draw:
    px = param->px;
    py = param->py;
    pz = param->pz;
    hint = param->super;
    size = param->size;

    if (hint != 0)
    {
        *(s16 *)TENCHU_SCRATCHPAD(0x20) = px;
        *(s16 *)TENCHU_SCRATCHPAD(0x22) = py;
        *(s16 *)TENCHU_SCRATCHPAD(0x24) = pz;
        GsGetLs(hint, (MATRIX *)TENCHU_SCRATCHPAD_ADDRESS);
        GsSetLsMatrix((MATRIX *)TENCHU_SCRATCHPAD_ADDRESS);
        scr.vz = (s16)RotTransPers(
            (SVECTOR *)TENCHU_SCRATCHPAD(0x20), (s32 *)&scr,
            (void *)TENCHU_SCRATCHPAD(0x28),
            (void *)TENCHU_SCRATCHPAD(0x2c));
    }
    else
    {
        GetScreenPosition(px, py, pz, (s32 *)&scr);
    }

    otz = scr.vz;
    if (otz > 0x24)
    {
        sc = (s16)((size * 300) / otz) + 1;
        spr->scaley = sc;
        spr->scalex = sc;
        spr->x = scr.vx;
        spr->y = scr.vy;
        t = scr.vz - 0x32;
        t = t >> 2;
        if (t < 0)
        {
            goto zero;
        }
        pri = 0x4e1;
        if (t < 0x4e2)
        {
            pri = t;
        }
        goto done;
    zero:
        pri = 0;
    done:
        GsSortSprite(spr, OTablePt, (u16)pri);
    }
}
