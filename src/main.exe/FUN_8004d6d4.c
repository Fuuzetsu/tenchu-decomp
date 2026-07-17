#include "common.h"
#include "main.exe.h"

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
 * function (message-style `(this, msg)`, no direct `jal` callers found —
 * reached through a function-pointer table). Same 3-way dispatch shape as
 * the neighbouring FUN_8004c59c (same TU): msg==0 resets a "played" byte
 * flag at `this+0x15`; msg<4 is otherwise ignored; msg>=4 fires once
 * `played==0`, then unconditionally spawns effects at a jittered position
 * (X and Z each jittered independently by a per-object range at
 * `this+0x1C`/`this+0x20`; Y is untouched) and, on even `GameClock` frames,
 * calls SetSmoke (a small upward puff) and SetSplash (a ground splash) at
 * that position — note this branch has NO reschedule/"next" field the way
 * FUN_8004c59c does; it fires on every qualifying call.
 *
 * Matching notes:
 *  - Dispatch shape identical to FUN_8004c59c: `if (arg1==0) goto reset;
 *    if (3<arg1) goto normal; return; reset: ...; normal: ...` — the
 *    explicit `goto`s (not `if/else`) are load-bearing: cc1 canonicalises
 *    a `return`-ending guard body as the FALLTHROUGH regardless of
 *    if/else/goto spelling for a 2-way split, so the "short returning
 *    case falls through, both real bodies are forward-jump targets"
 *    shape needs the "two independent if (cond) goto L;" cookbook recipe
 *    (test the ORIGINAL condition to `reset`, test the ORIGINAL
 *    complementary-range condition to `normal`, bare `return;` as the
 *    fallthrough short case) — an `if (arg1<4) return;` inline-return
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

void FUN_8004d6d4(u8 *arg0, u32 arg1)
{
    VECTOR pos;
    SVECTOR dir;
    s32 x, z;

    if (arg1 == 0) goto reset;
    if (3 < arg1) goto normal;
    return;

reset:
    *(arg0 + 0x15) = 0;
    return;

normal:
    if (*(arg0 + 0x15) != 0) return;

    x = *(s32 *)(arg0 + 4);
    pos.vx = (1 <= (*(s32 *)(arg0 + 0x1C) << 1))
                 ? x + (rand() % (*(s32 *)(arg0 + 0x1C) << 1) - *(s32 *)(arg0 + 0x1C))
                 : x - *(s32 *)(arg0 + 0x1C);

    pos.vy = *(s32 *)(arg0 + 8);
    z = *(s32 *)(arg0 + 0xC);
    pos.vz = (1 <= (*(s32 *)(arg0 + 0x20) << 1))
                 ? z + (rand() % (*(s32 *)(arg0 + 0x20) << 1) - *(s32 *)(arg0 + 0x20))
                 : z - *(s32 *)(arg0 + 0x20);

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
