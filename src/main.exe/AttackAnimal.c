#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short AttackAnimal(void);
 *     THINK_3.C:583, 26 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern long Distance;
 *     extern short Degree;
 * END PSX.SYM */

/*
 * AttackAnimal (0x8002f170, 0xe4 bytes) — animal-enemy attack-decision
 * think-helper, returning a synthesized pad word: while attacking or
 * jumping (status 7/9) it resets `actmode` and returns 0. Close AND
 * already facing the player (Distance < 2000, |Degree| < 200) it bites —
 * a plain Square press (PADRleft) without touching `actmode`. Otherwise
 * it bumps `actmode`, steers via turn_towards_player_, and escalates by
 * `actmode`'s run length: an early roll (<30) forces plain forward
 * (PADLup), the 30th call plays a warning Sound, up to 90 it masks the
 * steer to turn-only (PADLleft|PADLright), beyond that returns the full
 * steer word.
 *
 * Same "think" TU as Think1ninja.c/ThinkBasicHuman1.c/Think3chase.c/
 * Think3escape.c/Think3firstattack.c (Me_THINK_C, Distance, Degree, the
 * shared signed Humanoid status field).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `deg = Degree; if (deg < 0) deg = -deg;` re-reads its own destination
 *    for the negate (`negu $rX,$rX`, same source/dest register) — the
 *    "conditional negate must re-read its own destination" rule
 *    (Think1trace). `deg` itself must be `s32`, not `s16` like `Degree`:
 *    an `s16 deg` forces a separate HImode pseudo (extra `move` + a
 *    trailing `sll`/`sra` re-truncation before the `slti`), 3 instructions
 *    longer; `s32 deg` operates in place on the `lh`'s already-sign-
 *    extended register with no truncation needed until the caller compares
 *    it (which never narrows it further here).
 *  - `pad` (the `turn_towards_player_` result, later the function's return
 *    value) must ALSO be `s32`, not `s16`: main.exe.h's own prototype for
 *    `turn_towards_player_` returns `int` (disagreeing with the defining
 *    TU's actual `s16` — the "caller-side extern's return type is an
 *    extension-position lever" rule), so an `s16 pad` truncates the result
 *    immediately at the call (extra `move`+truncate pair), while `s32 pad`
 *    copies it straight into $s0 untruncated, truncating only once, at the
 *    final `return pad;` (matching the single trailing `sll`/`sra`).
 *  - `Me_THINK_C->actmode` (game_types.h, NEW field @0x88 — proven here by
 *    the raw `lbu`/`sb` and the `+1` arithmetic; Ghidra's own
 *    independently-built Humanoid names this exact offset `actmode`,
 *    right before the already-proven actflg/actcnt/actscnt run — replaces
 *    game_types.h's placeholder `field52_0x88`).
 *  - `pad` doubles as the `turn_towards_player_` result AND the eventual
 *    return value (matches $s0's dual role, callee-saved across the Sound
 *    call): a "default-then-override" ladder overrides it in 2 of 3
 *    branches and leaves it alone in the other 2 (the `== 0x1e` Sound
 *    branch and the implicit `>= 0x5a` else) — write it exactly as Ghidra's
 *    ladder shows, no `else` needed for the last arm.
 *  - `Sound` takes the shared `Humanoid *` and original signed-short sound
 *    id declaration from humanoid.h.
 */

short AttackAnimal(void)
{
    s32 deg;
    s32 pad;
    u8 am;

    if (Me_THINK_C->status == STAT_ATTACK || Me_THINK_C->status == STAT_JUMP)
    {
        Me_THINK_C->actmode = ANIMAL_ATTACK_TIMER_RESET;
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
            return PADRleft; /* bite */
        }
    }
    Me_THINK_C->actmode++;
    pad = turn_towards_player_(0, 0);
    am = Me_THINK_C->actmode;
    if (am < ANIMAL_ATTACK_NOTICE_FRAME)
    {
        pad = PADLup;
    }
    else if (am == ANIMAL_ATTACK_NOTICE_FRAME)
    {
        Sound(Me_THINK_C, CHAR_VOICE_NOTICE);
    }
    else if (am < ANIMAL_ATTACK_FULL_STEER_FRAME)
    {
        pad = pad & (PADLleft | PADLright);
    }
    return pad;
}
