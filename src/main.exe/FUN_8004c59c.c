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
 * FUN_8004c59c (0x8004c59c, 0x19C bytes) — periodic-sound-emitter think
 * function (message-style: called with `(this, msg)`, no direct `jal`
 * callers found — reached through a function-pointer table like
 * ProcXXX/ReqXXX). Shares its object layout with the neighbouring
 * FUN_8004c350 (same TU: x/y/z position at 0x4/0x8/0xC, a "played" byte flag
 * at 0x15, GameClock-compared reschedule word at 0x18) — msg==0 resets,
 * msg<4 is otherwise ignored, msg>=4 fires once `played==0` and
 * `GameClock>=sched.next`, then reschedules.
 *
 * `sched` (`this+0x18`) is a 3-word (0xC byte) sub-struct: `next` (s32,
 * GameClock deadline), `min`/`max` (s16, the delay range for the next
 * reschedule) and `sndIdx` (u8, added to 0x44 as the SoundEx sound id).
 *
 * Matching notes:
 *  - The msg==0 reset is NOT a simple field clear — it's a whole-struct
 *    assignment `*sched = tmp;` through a freshly-built local `Schedule
 *    tmp`, which is why cc1 emits 3 WORD lw/sw pairs through the stack
 *    (emit_block_move on a 3-word-aligned struct) instead of narrow
 *    per-field stores. `tmp`'s own fields are read from DIFFERENT offsets
 *    than where they end up: `tmp.max` is read from `this+0x20` (sched's own
 *    `sndIdx` position, reinterpreted as u16) and `tmp.sndIdx` from
 *    `this+0x18` (sched's own `next`, truncated to its low byte) — the reset
 *    intentionally recycles the old `next`/`sndIdx` bytes into the new
 *    `sndIdx`/`max` slots. The copy-through reads use `lhu` regardless of
 *    `min`'s true s16-ness (cookbook: "a pure narrowing struct-field copy
 *    uses lhu/lbu even for signed fields") — only the `u16 *` cast does that.
 *  - The three early-return guards (msg<4, played!=0, GameClock<sched.next)
 *    are flat guard clauses (IsVisible's shape), not one nested `&&`-chain.
 *  - The position is copied through TWO separate VECTOR locals (a memset
 *    scratch, then a whole-struct-copied second local whose address is what
 *    is actually passed to SoundEx) — the leLayoutEnemy shape.
 *  - `sched->min`/`sched->max` in the reschedule arithmetic are read via the
 *    persistent `Schedule *sched` pointer (computed once, before the
 *    reset-vs-else branch); the reset branch instead addresses `this`
 *    directly with raw offset casts, since it never uses `sched`.
 *  - Needs maspsx's --expand-div (rand() % (max-min) divides by a runtime
 *    value); no explicit trap() calls belong in the C.
 *
 * Three ordering facts drove the last 29 bytes; each is a source-structure
 * lever, NOT a scheduler tie (an earlier park called all three un-matchable):
 *  1. RESET FIELD ORDER — `tmp.sndIdx` must be assigned BEFORE `tmp.next`.
 *     In `.sched`'s LOG_LINKS every VARYING-base load (`mem(reg80+N)`) takes
 *     a true dep on EVERY preceding store to `tmp`, while the FIXED-address
 *     `GameClock` load (`mem(symbol_ref)`) has LOG_LINKS `(nil)` — no deps at
 *     all — so it alone floats. With `next` written before `sndIdx`, the
 *     sndIdx load is pinned below next's store and cannot fill GameClock's
 *     load-use slot, so `max`'s load fills it instead and the max pair sinks.
 *     Writing sndIdx first frees it to fill the slot, reproducing the target
 *     exactly (both nops and the v0/v0/v0/v1 assignment).
 *  2. GUARD OPERAND ORDER — the `max`-before-`min` load order comes from
 *     writing the guard as the EXPRESSION `sched->max - sched->min > 0`, not
 *     from two pre-loaded locals. With `s16 hi = sched->max;` combine fuses
 *     the load into a `sign_extend` and RELOCATES it to its use, so neither
 *     declaration order nor an s32 flip moves it (both verified inert).
 *  3. FINAL STORE ADDRESSING — `sw v0,0(s1)` (not `sw v0,24(s0)`) requires
 *     the store to sit in a JOIN block, i.e. one `sched->next = lo;` AFTER
 *     the if/else rather than one store per arm. cse1's find_best_addr
 *     rewrites the offset-0 `MEM(reg82)` to `MEM(reg80+24)` using sched's
 *     defining insn `reg82 = reg80 + 24`, but only where that equivalence is
 *     in its table; cse1 rebuilds the table at a multi-predecessor label, so
 *     a store in the join block keeps `reg82`. The two GameClock loads still
 *     survive because jump2's cross-jump stops at the if-arm's extra
 *     `addu v1,v1,a0`, merging only `addu v0,v0,v1; sw`.
 */

typedef struct
{
    s32 next;  /* 0x00 (this+0x18) */
    s16 min;   /* 0x04 (this+0x1C) */
    s16 max;   /* 0x06 (this+0x1E) */
    u8 sndIdx; /* 0x08 (this+0x20) */
} Schedule;

extern s32 GameClock;
extern s16 SoundEx(VECTOR *loc, s32 id);
extern s32 rand(void);
extern void *memset(void *dst, s32 c, u32 n);

void FUN_8004c59c(u8 *arg0, u32 arg1)
{
    Schedule *sched;
    Schedule tmp;
    VECTOR pos;
    VECTOR snd;
    s32 lo;

    sched = (Schedule *)(arg0 + 0x18);

    if (arg1 == 0) goto reset;
    if (3 < arg1) goto normal;
    return;

reset:
    tmp.min = *(u16 *)(arg0 + 0x1C);
    tmp.max = *(u16 *)(arg0 + 0x20);
    tmp.sndIdx = *(u8 *)(arg0 + 0x18);
    tmp.next = GameClock;
    *sched = tmp;
    *(arg0 + 0x15) = 0;
    return;

normal:
    if (*(arg0 + 0x15) != 0) return;
    if (sched->next > GameClock) return;

    memset(&snd, 0, sizeof(snd));
    snd.vx = *(s32 *)(arg0 + 4);
    snd.vy = *(s32 *)(arg0 + 8);
    snd.vz = *(s32 *)(arg0 + 0xC);
    pos = snd;
    SoundEx(&pos, sched->sndIdx + 0x44);

    if (sched->max - sched->min > 0) {
        lo = GameClock + (rand() % (sched->max - sched->min) + sched->min);
    } else {
        lo = GameClock + sched->min;
    }
    sched->next = lo;
}
