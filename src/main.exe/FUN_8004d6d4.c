#include "common.h"
#include "main.exe.h"
#include "misc.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 * END PSX.SYM */

/*
 * FUN_8004d6d4 (0x8004d6d4, 0x1A0 bytes) — periodic smoke/splash-puff think
 * function (message-style `(m, msg)`, no direct `jal` callers found —
 * reached through TMisc.proc). Same 3-way dispatch shape as the
 * neighbouring FUN_8004c59c (same TU): MM_CREATE resets `m->mode`; other
 * lifecycle messages are ignored; MM_DO computes a jittered position while
 * `mode==0` (X and Z each use the ranges initially supplied as
 * `m->param.init.b/c`; Y is untouched), then on even `GameClock` frames
 * emits a small upward smoke puff and a ground splash there. Unlike
 * FUN_8004c59c, this handler has no reschedule/"next" field and repeats the
 * position calculation on every active tick.
 *
 * Matching notes:
 *  - Dispatch shape identical to FUN_8004c59c: `if (msg==MM_CREATE) goto
 *    reset; if (MM_DO<=msg) goto normal; return; reset: ...; normal: ...` — the
 *    explicit `goto`s (not `if/else`) are load-bearing: cc1 canonicalises
 *    a `return`-ending guard body as the FALLTHROUGH regardless of
 *    if/else/goto spelling for a 2-way split, so the "short returning
 *    case falls through, both real bodies are forward-jump targets"
 *    shape needs the "two independent if (cond) goto L;" cookbook recipe
 *    (test the ORIGINAL condition to `reset`, test the ORIGINAL
 *    complementary-range condition to `normal`, bare `return;` as the
 *    fallthrough short case) — an `if (msg<MM_DO) return;` inline-return
 *    spelling for the second test does NOT reproduce it (verified on
 *    FUN_8004c59c: only `if (3 < arg1) goto normal;` placed reset between
 *    the two guard tests and normal's continuation, matching the target).
 *  - Each axis's jitter is a TERNARY, so both arms compute into ONE merge
 *    pseudo and the field is stored exactly ONCE at the join:
 *      `j L; addu v0,s1,v1; L_degen: subu v0,s1,v1; L: sw v0,16(sp)`
 *    An `if/else` whose arms each assign `pos.vx` directly gives each arm
 *    its OWN `sw` (two stores, no join), which cascades into both visible
 *    residuals: reorg then steals the degenerate arm's `subu` into the
 *    `blez`'s delay slot (target keeps a bare `nop` there and retargets one
 *    insn earlier), and the Z store is no longer available to fill the
 *    GameClock `bnez`'s delay slot the way the target does
 *    (`bnez v0,...; sw v1,24(sp)`). Ghidra's shared trailing
 *    `local_28.vx = iVar3 + local_28.vx;` was reporting this join correctly.
 *  - The degenerate test is `(dx << 1) < 1`, not `dx <= 0` — the target
 *    tests the DOUBLED value (feeding the same shift into the `blez`),
 *    not the raw field. Written here as the DE MORGAN'd `1 <= (dx << 1)`
 *    with the rand-path as the "then" (so it's the fallthrough/inline
 *    body, matching the target's placement) and the degenerate case as
 *    the "else" (the `blez`-reached forward-jump target) — the plain
 *    `if ((dx<<1)<1) {degenerate} else {rand}` spelling swaps which body
 *    is inline vs jump-target (same instructions, wrong layout, wrong
 *    length by the register-pressure cascade this cookbook already notes
 *    for FUN_8004c59c's dispatch).
 *  - Needs maspsx's --expand-div (rand() % (dx<<1) divides by a runtime
 *    value, twice, once per axis); no explicit trap() calls belong in the
 *    C.
 */

extern s32 GameClock;
extern s32 rand(void);
extern void SetSmoke(VECTOR *pos, SVECTOR *vect, s16 n, s16 time);
extern void SetSplash(VECTOR *pos, s16 sx, s16 sy, s32 speed);

void FUN_8004d6d4(TMisc *m, TMiscMessage msg)
{
    VECTOR pos;
    SVECTOR dir;
    s32 x, z;

    if (msg == MM_CREATE) goto reset;
    if (MM_DO <= msg) goto normal;
    return;

reset:
    m->mode = 0;
    return;

normal:
    if (m->mode != 0) return;

    x = m->x;
    pos.vx = (1 <= (m->param.init.b << 1))
                 ? x + (rand() % (m->param.init.b << 1) - m->param.init.b)
                 : x - m->param.init.b;

    pos.vy = m->y;
    z = m->z;
    pos.vz = (1 <= (m->param.init.c << 1))
                 ? z + (rand() % (m->param.init.c << 1) - m->param.init.c)
                 : z - m->param.init.c;

    if ((GameClock & 1) == 0) {
        dir.vx = 0;
        dir.vy = -400;
        dir.vz = 0;
        pos.vy = pos.vy + 2000;
        SetSmoke(&pos, &dir, 1, 1);
        pos.vy = pos.vy - 2000;
        SetSplash(&pos, 0x4000, 0x2000, 10);
    }
}
