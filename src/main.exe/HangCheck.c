#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short HangCheck(void);
 *     MOTION.C:291, 51 src lines, frame 56 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     stack sp+24     struct SVECTOR vect
 *     reg   $s0       long yy
 *     reg   $s0       long y
 *     reg   $s1       long ry
 *     reg   $s4       long oy
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern short motID;
 *     extern struct VECTOR *dtL;
 *     extern struct SVECTOR *dtR;
 *     extern unsigned long *GlobalAreaMap;
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — draft is 1 instruction (4 bytes) short of the 1220-byte
 * target (304 vs 305 insns; 75 differing asmdiff lines). RIGHT STRUCTURE, wrong
 * register coloring — a genuine sub-C register-allocation residual.
 *
 * Root cause: the target commits a SEVENTH callee-saved register ($s6) that ours
 * does not. When the ledge is caught it must keep the committed ledge height
 * (`oy`, the first successful GetAreaMapLevel result) live across the facing-snap
 * plus the two re-probe calls, for the revert path `dtL->vy = dtL->vy - oy + 5`.
 * The target holds `oy` in $s0 through the first half, then RELOCATES it with
 * `move $s6,$s0` (scheduled into the ry&0xFF guard's delay slot) because it
 * reuses $s0 for the re-probe result `y`. Ours has fewer values live (the demo's
 * 6-local structure leaves a register free), so `oy` stays put in $s3 with no
 * relocation move — hence 1 instruction short — and, with a spare register, ours
 * CACHES `dtL` (in $a1) across the `status==0xA` check where the target RELOADS it
 * fresh (`lw $a0,%gp_rel(dtL)` + load-delay nop). That single coloring decision
 * cascades: dtL cache-vs-reload at three sites, the return-0 tails cross-jumping
 * to one shared `move v0,zero` vs ours emitting a couple inline, and the two
 * `dtL->vy = dtL->vy - 0x69 + result` accumulations scheduling their add/addiu in
 * a different order. All are register/schedule choices with no C-level lever.
 *
 * What was tried (progressively cut the residual from 972 bytes to 4):
 *  - `Me_MOTION_C->width >> 1` etc. read the field PLAIN (not via a `(u16)` cast):
 *    the fused `sll16/sra17` divergent-width read falls out of the `>> 1` on a
 *    signed field, and forcing `(u16)` gave a WRONG `srl` (972-byte regression).
 *  - The first re-probe reuses `yy` (`yy - 400`), NOT a recompute — the target's
 *    `addiu $a2,$s0,-0x190` and m2c's `temp_s0 - 0x190` both prove reuse (only the
 *    SECOND batch, after `dtL->vy` is modified, genuinely recomputes).
 *  - Consolidating to the demo's 6 locals (`yy`,`y`,`ry`,`oy`,`i` + `vect`): one
 *    reused `y` for every tested-then-dead GetAreaMapLevel result, `oy` for the
 *    surviving committed height. Cut 96→80→75 differing lines.
 *  - `ry` is a SIGNED `long` (the read is `lh`, not `lhu`) — a `u16 ry` compiles a
 *    wrong `lhu`.
 *  - `dtL->vy - oy + 5` (not `+ 5 - oy`): fold's dependent-first evaluation keeps
 *    the target's load-delay ordering; the `+ 5 - oy` spelling filled a delay slot
 *    with an independent op, running 1 further instruction short.
 * autorules found no width/&&/inline win; regalloc.py shows the cross-call
 * pressure ($s0/$s1/$s2/$s3 all live across calls) with no breakable copy-chain;
 * a 600s / ~19000-iteration `permute.py --stop-on-zero -j4` run held at 146 errors
 * and NEVER reached score 0 — the reload/coloring class the cookbook marks
 * permuter-immune. Parked per the Iteration-protocol attempt cap.
 *
 * HangCheck (0x8001ceb0, 0x4c4 bytes) — per-frame ledge-hang detector
 * (MOTION.C, called from ActCHASE/ActHANG/ActMOVE/HumanActionControl). Bails
 * immediately for a "type 0xA_" special character, or unless the player is
 * airborne with room below (map.height), not already recovering (motID !=
 * 0x901) and not mid-item-use (active_item != 0xB). Casts a short forward
 * probe (GetMoveSpeed) both ways from the character's feet: if the ground is
 * closer than the character's own Y (a wall/ledge edge underfoot), nudges
 * `*dtL` back away from it and bails. Otherwise probes further down (300,
 * then 400 units) for a ledge to catch: once a catchable ledge is found,
 * snaps `*dtL` onto it, and — unless already in the "hang" status (10) —
 * snaps the character's own facing (`dtR->vy`) to the nearest 90-degree
 * quadrant and re-validates the ledge is still there at the snapped facing
 * (bailing back to the pre-snap height if not); on success installs the
 * "hang" motion (0xa01), guarding against clobbering a motion mid-update on
 * another Humanoid the same way MotionAndMove.c does (CVAhuman[] scan),
 * plays the catch sound, and rumbles the pad if this is the player's own
 * Humanoid.
 *
 * Matching notes:
 *  - GetAreaMapLevel's real prototype takes 5 args (area, x, y, z, mode) —
 *    Ghidra's own decompilation of this call under-counts to 3 (drops z and
 *    mode); m2c's raw a0-a3-plus-stack dump has the true arg list (cookbook:
 *    "m2c and Ghidra disagree on a call's ARG COUNT").
 *  - `Me_MOTION_C->width`/`vect.vx`/`vect.vz` are read `lhu` + a fused
 *    `sll 16/sra 17` (a per-TU divergent-width read of item.h's proven
 *    signed `width`/SVECTOR fields — cookbook: "Unsigned halfword loads
 *    mean the field is u16 even when a neighbouring field is s16") whenever
 *    the expression is `>> 1` (halved); plain additions of the SAME fields
 *    elsewhere in this function read them signed (`lh`) instead — two
 *    un-CSE'd loads of one value, same divergence class as
 *    DeleteConflict's `ConflictObjects`.
 *  - The SECOND feet-height (`yy` reused, `(dtL->vy - height) - 400` for the y3/y4
 *    re-probes) genuinely recomputes because `dtL->vy` was modified by the
 *    intervening `dtL->vy = dtL->vy - 0x69 + oy` commit; the FIRST batch reuses the
 *    original `yy`. Both the target asm and m2c confirm this split.
 *  - The committed ledge height is its OWN local (`oy`) distinct from the reused
 *    `y`: it is read again for the revert `dtL->vy = dtL->vy - oy + 5` after the y3
 *    re-probe's own call has reused `y`'s register — a genuine cross-call data
 *    dependency (this is the value the target relocates into $s6; see STATUS).
 *  - The CVAhuman scan is IDENTICAL to MotionAndMove.c's proven shape
 *    (`tag_CVAHumanEntry`, `short i`, provably-true `i=0` entry test folded
 *    away leaving a bottom-tested do/while-equivalent `for`) — reused
 *    verbatim here, except the "found" case must `goto` past the
 *    `SetNowMotion` call (shared with the "not found" fallthrough) rather
 *    than returning outright, since both paths still reach the `Sound` call.
 *  - `GlobalAreaMap` and `StagePlayer` are accessed via ABSOLUTE `lui/lw`
 *    (not `%gp_rel`) in this TU — left as plain externs, not added to the
 *    gp-extern list (only Me_MOTION_C/motID/dtL/dtR/MotionUpdateMode/
 *    D_80097F0E are gp-relative here, confirmed by `tools/gpsyms.py`).
 */
typedef struct
{
    Humanoid *human;
    u8 pad4[4];
} tag_CVAHumanEntry; /* 0x8 */

extern s16 motID;
extern VECTOR *dtL;
extern SVECTOR *dtR;
extern u32 *GlobalAreaMap;
extern s16 MotionUpdateMode;
extern tag_CVAHumanEntry CVAhuman[5];
extern Humanoid *StagePlayer;
extern Humanoid *Me_MOTION_C;
extern s16 D_80097F0E;

extern void GetMoveSpeed(SVECTOR *vect, short ry, short ordr, short side);
extern long GetAreaMapLevel(unsigned long *area, long x, long y, long z, int mode);
extern short SetNowMotion(Humanoid *human, short mid, short move);
extern void Sound(Humanoid *h, int id);
extern void PadShockAR(int port, int pow, int attack, int release);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/HangCheck", HangCheck);
#else
short HangCheck(void)
{
    SVECTOR vect;
    long yy;
    long y;
    long ry;
    u16 uVar4;
    short rys;
    long oy;
    short i;

    if ((Me_MOTION_C->type & 0xF0) == 0xA0)
    {
        return 0;
    }
    if (0 < Me_MOTION_C->map.height && motID != 0x901 && Me_MOTION_C->active_item != 0xB)
    {
        yy = dtL->vy - Me_MOTION_C->height;
        GetMoveSpeed(&vect, dtR->vy, Me_MOTION_C->width >> 1, 0);
        y = GetAreaMapLevel(GlobalAreaMap, dtL->vx + vect.vx, yy - 0x122, dtL->vz + vect.vz, 0);
        if (y < dtL->vy)
        {
            y = GetAreaMapLevel(GlobalAreaMap, dtL->vx - vect.vx, yy - 0x122, dtL->vz - vect.vz, 0);
            if (y == 0x80000000)
            {
                return 0;
            }
            if (y <= dtL->vy)
            {
                return 0;
            }
            dtL->vx = dtL->vx - (vect.vx >> 1);
            dtL->vz = dtL->vz - (vect.vz >> 1);
            return 0;
        }
        else
        {
            y = GetAreaMapLevel(GlobalAreaMap, dtL->vx, yy - 300, dtL->vz, 0);
            if (y < dtL->vy - Me_MOTION_C->height)
            {
                return 0;
            }
            GetMoveSpeed(&vect, dtR->vy, (Me_MOTION_C->width >> 1) + 300, 0);
            oy = GetAreaMapLevel(GlobalAreaMap, dtL->vx + vect.vx, yy - 400, dtL->vz + vect.vz, 2);
            if (oy == 0x80000000 || oy >= 0x191)
            {
                return 0;
            }
            dtL->vy = dtL->vy - 0x69 + oy;
            if (Me_MOTION_C->status == 0xA)
            {
                return 1;
            }
            ry = dtR->vy;
            if (ry & 0xFF)
            {
                uVar4 = ry & 0x200;
                ry = ry & 0xC00;
                if (uVar4 != 0)
                {
                    ry = ry + 0x400;
                }
            }
            rys = ry;
            GetMoveSpeed(&vect, rys, (Me_MOTION_C->width >> 1) + 300, 0);
            yy = (dtL->vy - Me_MOTION_C->height) - 400;
            y = GetAreaMapLevel(GlobalAreaMap, dtL->vx + vect.vx, yy, dtL->vz + vect.vz, 2);
            if (y == 0x80000000 || y >= 0x191)
            {
                dtL->vy = dtL->vy - oy + 5;
                return 0;
            }
            dtR->vy = ry;
            GetMoveSpeed(&vect, rys, (Me_MOTION_C->width >> 1) + 100, 0);
            y = GetAreaMapLevel(GlobalAreaMap, dtL->vx + vect.vx, yy, dtL->vz + vect.vz, 2);
            if (y != 0x80000000 && y < 0x191)
            {
                GetMoveSpeed(&vect, dtR->vy, -200, 0);
                dtL->vx = dtL->vx + vect.vx;
                dtL->vy = dtL->vy - 0x69 + y;
                dtL->vz = dtL->vz + vect.vz;
            }
            motID = 0xA01;
            D_80097F0E = 1;
            if (MotionUpdateMode != 0)
            {
                for (i = 0; i < 5; i++)
                {
                    if (CVAhuman[i].human == Me_MOTION_C)
                    {
                        goto found;
                    }
                }
            }
            SetNowMotion(Me_MOTION_C, motID, D_80097F0E);
            D_80097F0E = -1;
        found:
            Sound(Me_MOTION_C, 0x1B);
            if (StagePlayer != Me_MOTION_C)
            {
                return -1;
            }
            PadShockAR(0, 0x7F, 0, 0x1E);
            return -1;
        }
    }
    return 0;
}
#endif /* NON_MATCHING */
