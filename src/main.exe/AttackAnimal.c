#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short AttackAnimal(void);
 *     THINK_3.C:583, 26 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern long Distance;
 *     extern short Degree;
 * END PSX.SYM */

/*
 * AttackAnimal (0x8002f170, 0xe4 bytes) — animal-enemy attack-decision
 * think-helper: while jumping/landing (character_status 7/9) resets
 * `actmode` and returns 0. Otherwise, when close (Distance>=2000) or facing
 * roughly at the player already (|Degree|>=200) it bumps `actmode`, turns
 * toward the player (turn_towards_player_), and escalates the result by
 * `actmode`'s run length: an early roll (<30) forces 0x1000 (turn only), the
 * 30th call plays a warning Sound, otherwise (up to 90) masks the turn
 * result to 0xA000 (start attacking), beyond that returns it unmasked. When
 * NOT close/facing yet it early-returns a fixed 0x80 (small correction turn)
 * without touching `actmode`.
 *
 * Same "think" TU as Think1ninja.c/ThinkBasicHuman1.c/Think3chase.c/
 * Think3escape.c/Think3firstattack.c (Me_THINK_C, Distance, Degree, the
 * per-TU `(s16)Me_THINK_C->status` load-width cast).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `(s16)Me_THINK_C->status == 7 || == 9` follows Think1ninja.c's
 *    established per-TU cast (the shared field is u16; this TU's asm reads
 *    it with `lh`).
 *  - `deg = Degree; if (deg < 0) deg = -deg;` re-reads its own destination
 *    for the negate (`negu $rX,$rX`, same source/dest register) — the
 *    "conditional negate must re-read its own destination" rule
 *    (Think1trace). `deg` itself must be `s32`, not `s16` like `Degree`:
 *    an `s16 deg` forces a separate HImode pseudo (extra `move` + a
 *    trailing `sll`/`sra` re-truncation before the `slti`), 3 instructions
 *    longer; `s32 deg` operates in place on the `lh`'s already-sign-
 *    extended register with no truncation needed until the caller compares
 *    it (which never narrows it further here).
 *  - `ret` (the `turn_towards_player_` result, later the function's return
 *    value) must ALSO be `s32`, not `s16`: main.exe.h's own prototype for
 *    `turn_towards_player_` returns `int` (disagreeing with the defining
 *    TU's actual `s16` — the "caller-side extern's return type is an
 *    extension-position lever" rule), so an `s16 ret` truncates the result
 *    immediately at the call (extra `move`+truncate pair), while `s32 ret`
 *    copies it straight into $s0 untruncated, truncating only once, at the
 *    final `return ret;` (matching the single trailing `sll`/`sra`).
 *  - `Me_THINK_C->actmode` (game_types.h, NEW field @0x88 — proven here by
 *    the raw `lbu`/`sb` and the `+1` arithmetic; Ghidra's own
 *    independently-built Humanoid names this exact offset `actmode`,
 *    right before the already-proven actflg/actcnt/actscnt run — replaces
 *    game_types.h's placeholder `field52_0x88`).
 *  - `ret` doubles as the `turn_towards_player_` result AND the eventual
 *    return value (matches $s0's dual role, callee-saved across the Sound
 *    call): a "default-then-override" ladder overrides it in 2 of 3
 *    branches and leaves it alone in the other 2 (the `== 0x1e` Sound
 *    branch and the implicit `>= 0x5a` else) — write it exactly as Ghidra's
 *    ladder shows, no `else` needed for the last arm.
 *  - `Sound` is called with `Me_THINK_C` (`Humanoid *`), NOT
 *    item.h's `Humanoid *` — this TU's own view of the object (same
 *    per-TU-prototype convention as every other Sound call site).
 */
extern s32 Distance;
extern s16 Degree;
extern void Sound(Humanoid *cs, int seid);

short AttackAnimal(void)
{
    s32 deg;
    s32 ret;
    u8 am;

    if ((s16)Me_THINK_C->status == 7 || (s16)Me_THINK_C->status == 9)
    {
        Me_THINK_C->actmode = 0;
        return 0;
    }
    if (Distance < 2000)
    {
        deg = Degree;
        if (deg < 0)
        {
            deg = -deg;
        }
        if (deg < 200)
        {
            return 0x80;
        }
    }
    Me_THINK_C->actmode = Me_THINK_C->actmode + 1;
    ret = turn_towards_player_(0, 0);
    am = Me_THINK_C->actmode;
    if (am < 0x1e)
    {
        ret = 0x1000;
    }
    else if (am == 0x1e)
    {
        Sound(Me_THINK_C, 0xc);
    }
    else if (am < 0x5a)
    {
        ret = ret & 0xA000;
    }
    return ret;
}
