#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3escape(void);
 *     THINK_3.C:103, 21 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern long Distance;
 *     extern short SR;
 *     extern short Degree;
 *     extern short Attrib;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 84 of 280 bytes differ (right length: matchdiff's
 * whole-image count equals the function-local count, so nothing downstream
 * is shifted).
 *
 * Think3escape (0x8002d520, 0x118 bytes) — think-handler, same TU as
 * Think3chase.c/Think3firstattack.c/Think1trace.c (s16 return convention;
 * gp-relative Distance/Degree/SR/Attrib/Me_THINK_C). Deliberately does NOT
 * include main.exe.h: this file reads Attrib as u16 (`lhu`), conflicting
 * with main.exe.h's `s16 Attrib` — same per-file respelling as
 * think_setting_go_towards_player.c/turn_towards_player_.c.
 *
 * Clears SR when close (< 0x4074) and not already in the "-2" state. Picks a
 * turn direction from Degree's sign (-0x8000 if positive, 0x2000 if
 * negative) — reading Degree TWICE (m2c's `var_v0_2`): the direct-jump path
 * (Degree > 0) reuses the FIRST read; the else path (Degree <= 0) re-reads
 * Degree fresh after the inner `< 0` test, since that's a textually separate
 * reference in source (BYTE-PROVEN — this reload is present and correct in
 * the current draft). ORs in a walk/run signal (0x4000 near / 0x1000 far)
 * from abs(Degree) vs 1000.
 *
 * If Attrib bit 0x400 is set and abs(Degree) > 1000 and no escape is already
 * armed (Me_THINK_C->field76_0xb0 == 0): arms one, encoding a turn direction
 * flag (0x80000000 if Degree <= 0, 0x20000000 if Degree > 0 — reusing the
 * SAME cached Degree register for this sign test, not a fresh reload) OR'd
 * with `1000 / character_rotation_speed` (a genuine RUNTIME division —
 * needs maspsx --expand-div, added to Build.hs/permute.py's extra list).
 * `character_rotation_speed` (game_types.h, proven @0x6) is Humanoid.turn's
 * (item.h, same offset) cross-TU twin, matching Ghidra's own
 * `Me_THINK_C->turn` naming for this call site.
 *
 * `degree2`/`abs_degree2` split (the conditional-negate-rereads-its-own-
 * destination idiom): `abs_degree2 = degree2; if (abs_degree2 < 0)
 * abs_degree2 = -abs_degree2;` — the asm's `negu $v0,$v0` is self-referencing
 * (dest==src==v0), while `degree2` (a2) stays the untouched raw signed value,
 * read again for the later `Degree > 0` hint-sign test. Ghidra's rendering
 * (`iVar1 = -iVar4`) obscures this — trust the asm's register reuse instead
 * (same family as Think1trace's negate idiom). `degree2` specifically as
 * `s16` (not s32) was an autorules-found, load-bearing width: it fixed the
 * function's LENGTH (was 8 bytes short at s32 — matches a scheduling gap
 * this cookbook entry couldn't otherwise close) at the cost of a smaller,
 * accepted side effect (see RESIDUAL). `quotient` as an explicit temp
 * (computed before `hint`, not inlined into the final `| hint` expression)
 * shrank the diff further (177->171 bytes) before the degree2 width fix
 * (171->84).
 *
 * RESIDUAL (84 bytes / ~21 instructions, all register-color or scheduling,
 * not wrong instructions):
 *  1. `result` lands in $a1 here vs the target's $a0 throughout (and the
 *     cached `Me_THINK_C` pointer takes $a0 here vs the target's $a1) — a
 *     clean, total swap of the SAME two roles, not a wrong value. Tried:
 *     reordering `result`/`degree` declarations, hoisting `result = 0`
 *     after the Distance/SR guard, explicitly caching `Me_THINK_C` into a
 *     named pointer local — none changed the allocation (`tools/regalloc.py`
 *     shows no copy-chain to break; this looks like straight global.c
 *     priority ordering — `result`'s long live range vs the cached
 *     pointer's short one — that the target's real source must invert via
 *     some structural difference not yet found).
 *  2. `degree2`'s `s16` width (needed for the length, see above) makes its
 *     OWN read narrow-store shaped (`lhu`, since the immediate destination
 *     is s16), forcing a separate explicit resign (`sll`/`sra`) when it's
 *     WIDENED into `abs_degree2` (s32) — the target reads Degree once
 *     (`lh`, signed) and uses that single value for both the abs-compute
 *     AND the later sign test with no resign step. Tried `abs_degree2` also
 *     `s16` (matching widths, avoiding the widen): fixed the `lhu`->`lh`
 *     load but ADDED a different resign after the negate (89 bytes, worse
 *     net) — rejected.
 * `tools/autorules.py` found no further win at this state (30+ combinations
 * tried across every local's width). One bounded permuter run (~90s,
 * --stop-on-zero -j4): every candidate scored in the hundreds with no
 * visible correlation to the actual ~84-byte gap (same harness-scoring
 * mismatch already seen on Think3firstattack in this batch — flagged, not
 * chased further per the attempt-cap guidance). decomp.me (psyq4.3 preset)
 * would be the next arbiter if revisited.
 */
extern character_state *Me_THINK_C;
extern s32 Distance;
extern s16 Degree;
extern s16 SR;
extern u16 Attrib;

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/Think3escape", Think3escape);
#else /* NON_MATCHING */

s16 Think3escape(void)
{
    s32 result;
    s32 degree;

    result = 0;
    if (Distance < 0x4074 && SR != -2)
    {
        SR = 0;
    }
    degree = Degree;
    if (degree > 0)
    {
        result = -0x8000;
    }
    else
    {
        if (degree < 0)
        {
            result = 0x2000;
        }
        degree = Degree;
    }
    if (degree < 0)
    {
        degree = -degree;
    }
    if (degree < 1000)
    {
        result |= 0x4000;
    }
    else
    {
        result |= 0x1000;
    }
    if (Attrib & 0x400)
    {
        s16 degree2;
        s32 abs_degree2;

        degree2 = Degree;
        abs_degree2 = degree2;
        if (abs_degree2 < 0)
        {
            abs_degree2 = -abs_degree2;
        }
        if (abs_degree2 > 1000 && Me_THINK_C->field76_0xb0 == 0)
        {
            s32 quotient;
            s32 hint;

            quotient = 1000 / Me_THINK_C->character_rotation_speed;
            hint = 0x80000000;
            if (degree2 > 0)
            {
                hint = 0x20000000;
            }
            Me_THINK_C->field76_0xb0 = quotient | hint;
        }
    }
    return result;
}

#endif /* NON_MATCHING */
