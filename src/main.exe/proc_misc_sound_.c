#include "common.h"
#include "main.exe.h"
#include "misc.h"

/*
 * proc_misc_sound_ (0x8004c59c, 0x19C bytes) — periodic-sound-emitter think
 * function (message-style: called with `(m, msg)`, no direct `jal` callers
 * found — reached through TMisc.proc). Shares TMisc's position and
 * mode fields with the neighbouring proc_misc_bonfire_; its retail-only parameter
 * overlay is a GameClock-based schedule. MM_CREATE reshapes the AddMisc
 * initialization payload into that schedule and clears mode. The other
 * lifecycle messages are ignored. MM_DO fires while `mode==0` once
 * `GameClock>=sched.next`, then reschedules.
 *
 * `sched` (overlaid on `m->param`) is the 3-word MiscSoundSchedule: `next`
 * (s32, GameClock deadline), `min_delay`/`max_delay` (s16, the delay range
 * for the next reschedule), and `sound_index` (u8, added to
 * MISC_SOUND_ID_BASE for the SoundEx id).
 *
 * Matching notes:
 *  - The MM_CREATE initialization is NOT a simple field clear — it's a
 *    whole-struct assignment `*sched = tmp;` through a freshly-built local
 *    `MiscSoundSchedule tmp`, which is why cc1 emits 3 WORD lw/sw pairs
 *    through the stack (emit_block_move on a 3-word-aligned struct) instead
 *    of narrow
 *    per-field stores. The AddMisc payload's `a` becomes `sound_index`, `b`
 *    becomes `min_delay`, and `c` becomes `max_delay`, while `GameClock`
 *    becomes `next`;
 *    some of those values consequently move to different union offsets.
 *    In the overlaid schedule view, `param.sound_init.max_delay` occupies
 *    `sound_index`'s offset and `param.sound_init.sound` occupies
 *    `next`'s offset; the whole-struct assignment then packs the reshaped
 *    values into their runtime slots. The delay-bound copies both use `lhu`
 *    despite their true s16 types (cookbook: "a
 *    pure narrowing struct-field copy uses lhu/lbu even for signed fields")
 *    — only the `u16 *` casts produce those loads.
 *  - The three early-return guards (msg<MM_DO, mode!=0,
 *    GameClock<sched.next)
 *    are flat guard clauses (IsVisible's shape), not one nested `&&`-chain.
 *  - The position is copied through TWO separate VECTOR locals (a memset
 *    scratch, then a whole-struct-copied second local whose address is what
 *    is actually passed to SoundEx) — the leLayoutEnemy shape.
 *  - The schedule delay bounds in the reschedule arithmetic are read via the
 *    persistent `MiscSoundSchedule *sched` pointer (computed once, before the
 *    reset-vs-else branch); the reset branch instead addresses `m->param`
 *    directly through its initialization view, since it never uses `sched`.
 *  - Needs maspsx's --expand-div (rand() % (max-min) divides by a runtime
 *    value); no explicit trap() calls belong in the C.
 *
 * Three ordering facts drove the last 29 bytes; each is a source-structure
 * lever, NOT a scheduler tie (an earlier park called all three un-matchable):
 *  1. RESET FIELD ORDER — `tmp.sound_index` must be assigned BEFORE
 *     `tmp.next`.
 *     In `.sched`'s LOG_LINKS every VARYING-base load (`mem(reg80+N)`) takes
 *     a true dep on EVERY preceding store to `tmp`, while the FIXED-address
 *     `GameClock` load (`mem(symbol_ref)`) has LOG_LINKS `(nil)` — no deps at
 *     all — so it alone floats. With `next` written before `sound_index`, the
 *     sound-index load is pinned below next's store and cannot fill
 *     GameClock's load-use slot, so the maximum-delay load fills it instead
 *     and that pair sinks. Writing sound_index first frees it to fill the
 *     slot, reproducing the target exactly (both nops and the v0/v0/v0/v1
 *     assignment).
 *  2. GUARD OPERAND ORDER — the max-before-min load order comes from writing
 *     `sched->max_delay - sched->min_delay > 0`, not from two pre-loaded
 *     locals. With `s16 hi = sched->max_delay;` combine fuses
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

extern s32 rand(void);
extern void *memset(void *dst, s32 c, u32 n);

void proc_misc_sound_(TMisc *m, TMiscMessage msg)
{
    MiscSoundSchedule *sched;
    MiscSoundSchedule tmp;
    VECTOR pos;
    VECTOR snd;
    s32 lo;

    sched = &m->param.sound;

    if (msg == MM_CREATE)
        goto reset;
    if (MM_DO <= msg)
        goto normal;
    return;

reset:
    tmp.min_delay = m->param.sound_init.min_delay;
    tmp.max_delay = m->param.sound_init.max_delay;
    tmp.sound_index = m->param.sound_init.sound.index;
    tmp.next = GameClock;
    *sched = tmp;
    m->mode = 0;
    return;

normal:
    if (m->mode != 0)
        return;
    if (sched->next > GameClock)
        return;

    memset(&snd, 0, sizeof(snd));
    snd.vx = m->x;
    snd.vy = m->y;
    snd.vz = m->z;
    pos = snd;
    SoundEx(&pos, sched->sound_index + MISC_SOUND_ID_BASE);

    if (sched->max_delay - sched->min_delay > 0)
    {
        lo = GameClock +
             (rand() % (sched->max_delay - sched->min_delay) +
              sched->min_delay);
    }
    else
    {
        lo = GameClock + sched->min_delay;
    }
    sched->next = lo;
}
