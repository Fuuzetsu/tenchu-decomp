#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "conflict.h"
#include "humanoid.h"
#include "game_globals.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GotoPosition(long vx, long vz);
 *     THINK.C:254, 11 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       long vx
 *     param $a1       long vz
 * END PSX.SYM */

/*
 * GotoPosition (0x8002b990, 0x1E0 bytes) — steer toward an x/z offset and
 * decide whether it is safe to step. It is called by nearly every AI think
 * handler in this TU (Think1sleep/Think2confirm/Think2contact and many
 * unmatched Think* siblings). Result is a bitmask of
 * button-like turn/walk bits (PADLup "close enough to stop turning",
 * PADLright/PADLleft for turning) truncated to s16 on return.
 *
 * The original name and `long vx, long vz` contract come from the demo's
 * THINK.C symbols. Its smaller body occupies the same StateTransition / this /
 * ChasetoTarget source slot and implements this function's direction, turn,
 * and pad-selection core; retail extends it with the obstacle probe below.
 *
 * Attrib and Degree keep their recovered signed object types.  This function's
 * `lhu` flag reads use the shared `Attrib` view, while the raw-angle path
 * below takes a localized unsigned view of Degree.
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
 *  - `d2 |= 30;` then store (in-place or) — NOT `... = d2 | 0x1e;`. The
 *    fresh or-temp of the compound-expression form is colored by local-alloc
 *    before the (longer-lived) Me_THINK_C pointer temp, stealing $v0 and
 *    pushing Me to $v1, which then pushes d2 off $v1 entirely (to $a0).
 *  - The `if (cached != LEVEL_NONE) return result;` guard is a LITERAL EARLY
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
extern s32 ProbeLevelLow;

s16 GotoPosition(s32 vx, s32 vz)
{
    u16 dir;
    s32 turn;
    s32 result;
    s32 adir;

    result = 0;
    if (vx != 0 || vz != 0)
    {
        dir = GetDirection(vx, vz,
                           Me_THINK_C->rotate->vy);
    }
    else
    {
        dir = Degree;
    }
    turn = Me_THINK_C->turn;
    if (turn < (s16)dir)
    {
        result = PADLright;
    }
    else if ((s16)dir < -turn)
    {
        result = (s16)PADLleft;
    }
    adir = (s16)dir;
    if (adir < 0)
    {
        adir = -adir;
    }
    if (adir < 500)
    {
        result |= PADLup;
    }
    if (!(Attrib & ATTR_PHASE))
    {
        if (Attrib & ATTR_WALL)
        {
            s32 cached;

            cached = ProbeLevelLow;
            if (cached != LEVEL_NONE)
            {
                return result;
            }
            if (Me_THINK_C->pad_hold == 0)
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
                                     AREA_LEVEL_RETURN_DELTA |
                                         AREA_LEVEL_FIRST_HIT |
                                         AREA_LEVEL_REUSE_CACHED);
                d2 = GetAreaMapLevel(GlobalAreaMap,
                                     Me_THINK_C->locate->vx - local.vx,
                                     Me_THINK_C->locate->vy - 500,
                                     Me_THINK_C->locate->vz - local.vz,
                                     AREA_LEVEL_RETURN_DELTA |
                                         AREA_LEVEL_FIRST_HIT |
                                         AREA_LEVEL_REUSE_CACHED);
                /* pad_hold packs (button << 16) | frames: latch a 30-frame
                 * sidestep toward the clearer flank. */
                if ((result & PADLright) && (d1 != cached))
                {
                    d2 = PADLright << 16;
                    goto apply;
                }
                if ((result & PADLleft) && (d2 != (u32)LEVEL_NONE))
                {
                    d2 = (u32)PADLleft << 16;
                apply:
                    d2 |= 30;
                    Me_THINK_C->pad_hold = d2;
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
