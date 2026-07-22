#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "game_globals.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * PSX.SYM suggests this may be `GotoPosition` (LOW confidence, THINK.C) — NOT
 * adopted. Corroborate with `tools/callmatch.py --verify` before renaming.
 * END PSX.SYM */

/*
 * turn_towards_player_ (0x8002b990, 0x1E0 bytes) — the shared "which way to
 * turn, and is it safe to step" helper called by nearly every AI think
 * handler in this TU (Think1sleep/Think2confirm/think_setting_go_towards_-
 * player and many unmatched Think* siblings). Result is a bitmask of
 * button-like turn/walk bits (0x1000 "close enough to stop turning",
 * 0x2000/0x8000 "turn right/left") truncated to s16 on return.
 *
 * Deliberately does NOT include main.exe.h: this file needs `Attrib` and
 * `Degree` read as u16 (`lhu`), conflicting with main.exe.h's `s16 Attrib`
 * (used by Think1sleep.c/Think2confirm.c) — same per-file respelling as
 * Think2contact.c.
 *
 * Matching notes (all byte-proven):
 *  - `dir` is u16: raw GetDirection()/Degree bits, given an explicit `(s16)`
 *    cast at each of its 3 separate comparison sites rather than a cached
 *    temp — the asm shows ONE shared truncation for the first two compares
 *    (same extended basic block) but a FRESH one for the third (past the
 *    if/else-if join's CODE_LABEL, outside cse's window).
 *  - The two GetAreaMapLevel calls each re-read
 *    `Me_THINK_C->locate` fresh (no cached pointer).
 *  - The blocked-direction flag is D2 REUSED (one variable, double duty):
 *    the target keeps the flag in $v1 — the same register that holds d2 —
 *    and gcc 2.8.1 never splits a live range, so the source overwrote `d2`
 *    with the 0x20000000/0x80000000 flag value instead of naming a `flag`
 *    local. The apparently "unconditional" delay-slot `move $v1,$v0` after
 *    `beq $v1,$v0` is NOT a comma-expression pre-assignment: it is reorg
 *    stealing the fallthrough arm's first insn (`d2 = 0x80000000`, cse'd to
 *    a copy of the compare constant) into the branch's delay slot, safe
 *    because $v1 is dead on the taken (else) path. With d2 live at the
 *    second if's compare, reorg also can't invert the first if's
 *    `beq s0,s2` (stealing `lui $v1,0x2000` would clobber live $v1), which
 *    reproduces the target's beq/nop/j/lui shape and the single early
 *    `andi 0x8000` shared by both paths.
 *  - `d2 |= 0x1e;` then store (in-place or) — NOT `... = d2 | 0x1e;`. The
 *    fresh or-temp of the compound-expression form is colored by local-alloc
 *    before the (longer-lived) Me_THINK_C pointer temp, stealing $v0 and
 *    pushing Me to $v1, which then pushes d2 off $v1 entirely (to $a0).
 *  - The `if (cached != 0x80000000) return result;` guard is a LITERAL EARLY
 *    RETURN, and it is load-bearing for the epilogue schedule: a second
 *    `return` statement makes expand emit a jump to return_label, so at
 *    sched2 time (which runs BEFORE jump2/cross-jump in this cc1 — verified
 *    via -da dumps: the .jump2 dump already carries sched2's dep lists) the
 *    label pins the return truncation's `sra` ABOVE the epilogue restore
 *    loads. With a single structured return the sll/sra/use flow straight
 *    into the threaded epilogue as one block and sched2 sinks the sra below
 *    all four `lw`s (loads have longer latency chains to the blockage insn).
 *    Cross-jump then merges the duplicate [sll][sra][use][jump] return body
 *    away and jump2 re-inverts the guard branch, so the early return costs
 *    zero bytes elsewhere. Guard 3 is the only safe site: guards 1/2 share
 *    one `lhu Attrib` and guard 4 shares the `Me_THINK_C` load with the
 *    body, and an early-return body between them would break those cse
 *    windows (cse follows the fallthrough path only).
 */
extern Humanoid *Me_THINK_C;
extern u16 Attrib;
extern u16 Degree;
extern s32 D_80097F10;

extern s16 GetDirection(s32 dx, s32 dz, s32 roty);
extern void GetMoveSpeed(SVECTOR *out, s32 roty, s32 b, s32 width);
extern s32 GetAreaMapLevel(unsigned long *area, s32 x, s32 y, s32 z,
                           u16 mode);

s16 turn_towards_player_(s32 x_diff, s32 z_diff)
{
    u16 dir;
    s32 turn;
    s32 result;
    s32 adir;

    result = 0;
    if (x_diff != 0 || z_diff != 0)
    {
        dir = GetDirection(x_diff, z_diff,
                            Me_THINK_C->rotate->vy);
    }
    else
    {
        dir = Degree;
    }
    turn = Me_THINK_C->turn;
    if (turn < (s16)dir)
    {
        result = 0x2000;
    }
    else if ((s16)dir < -turn)
    {
        result = -0x8000;
    }
    adir = (s16)dir;
    if (adir < 0)
    {
        adir = -adir;
    }
    if (adir < 500)
    {
        result |= 0x1000;
    }
    if (!(Attrib & 3))
    {
        if (Attrib & 0x400)
        {
            s32 cached;

            cached = D_80097F10;
            if (cached != 0x80000000)
            {
                return result;
            }
            if (Me_THINK_C->field76_0xb0 == 0)
            {
                SVECTOR local;
                s32 d1, d2;

                GetMoveSpeed(&local,
                             Me_THINK_C->rotate->vy,
                             0, Me_THINK_C->width);
                d1 = GetAreaMapLevel(GlobalAreaMap,
                                     Me_THINK_C->locate->vx + local.vx,
                                     Me_THINK_C->locate->vy - 500,
                                     Me_THINK_C->locate->vz + local.vz,
                                     0x1a);
                d2 = GetAreaMapLevel(GlobalAreaMap,
                                     Me_THINK_C->locate->vx - local.vx,
                                     Me_THINK_C->locate->vy - 500,
                                     Me_THINK_C->locate->vz - local.vz,
                                     0x1a);
                if ((result & 0x2000) && (d1 != cached))
                {
                    d2 = 0x20000000;
                    goto apply;
                }
                if ((result & 0x8000) && (d2 != 0x80000000))
                {
                    d2 = 0x80000000;
                apply:
                    d2 |= 0x1e;
                    Me_THINK_C->field76_0xb0 = d2;
                }
                else
                {
                    result = 0;
                }
            }
        }
    }
    return result;
}
