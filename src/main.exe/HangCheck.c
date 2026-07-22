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
 *     extern short motMODE;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

/*
 * HangCheck (0x8001ceb0, 0x4c4 bytes) — per-frame ledge-hang detector
 * (MOTION.C, called from ActCHASE/ActHANG/ActMOVE/HumanActionControl). Bails
 * immediately for a "type 0xA_" special character, or unless the player is
 * airborne with room below (map.height), not already recovering (motID !=
 * 0x901) and not while Henshin is active (itmctl != ITEM_HENSHIN). Casts a short forward
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
 *  - EVERY early bail is a guard clause (`return 0;`), NOT an `if (a && b &&
 *    c) { body } return 0;` wrapper: each `return 0` expands to its own
 *    [v0=0; j return_label] island and jump2's cross-jump merges them all
 *    into the LAST plain island — the oy-guard's inline `return 0` at
 *    0x8001d0d4 — which is where every earlier guard branches. The wrapper
 *    form instead emits the shared `move v0,zero` at the very END (after the
 *    return-1 tail), retargeting every guard branch and reshaping the tail.
 *  - The wall-path nudge is a CONDITIONAL body falling into a SHARED return:
 *    `if (dtL->vy < y) { nudge stores } return 0;` — the label between the
 *    stores and the [v0=0; j] island stops sched1 from hoisting the v0=0
 *    into the stores' load-delay holes (it is a separate basic block), so
 *    cross-jump later merges this island into the oy island, giving the
 *    target's `j .L8001D0D4` with the last `sw` in the delay slot. Spelled
 *    as `... if (y <= dtL->vy) return 0; stores; return 0;` the v0=0 shares
 *    the stores' block, sched1 (reverse list scheduler, priorities =
 *    depth-from-top, so a depth-1 constant-set always loses the bottom slot)
 *    buries it mid-block, cross-jump then can't match the tail, and the
 *    function runs one insn long with a knock-on register cascade.
 *  - By contrast the y3-failure revert (`{ dtL->vy = ...; return 0; }`
 *    INSIDE the if-arm, same basic block as its stores) is the shape whose
 *    v0=0 legitimately DOES get hoisted into the `lw` delay hole and whose
 *    `j` goes straight to the return label — both shapes appear in this one
 *    function, distinguished only by block boundaries.
 *  - The probe result is always the SAME reused `y` ($s0), including the
 *    committed 400-probe; the surviving ledge height is a separate `oy = y;`
 *    COPY made after the commit (the target's `move $s6,$s0` in the ry&0xFF
 *    guard's delay slot). gcc 2.8.1 never splits a live range, so the copy
 *    insn PROVES two pseudos: `y` is clobbered by the y3 re-probe while `oy`
 *    stays live for the revert. Assigning the probe straight into `oy`
 *    (no copy) runs one insn short.
 *  - `dy` is ONE variable assigned twice — `dy = yy - 300;` (the y2 probe's
 *    height arg) and later `dy = (dtL->vy - height) - 400;` (the y3/y4
 *    base) — which is why it colors into callee-saved $s1 even though the
 *    first segment crosses no call. The first assignment must sit BEFORE
 *    `if (y < dtL->vy)`: as the first statement of the ledge path it lands
 *    in the same basic block as its single use and combine folds it into
 *    the call arg (`addiu a2,s0,-300`), never materializing $s1. Placed
 *    before the branch it is cross-block, survives combine, and reorg pulls
 *    it into the wall-branch delay slot exactly like the target.
 *  - The facing snap masks into a TEMP then assigns back (`yc = ry & 0xC00;
 *    if (ry & 0x200) yc += 0x400; ry = yc;`) — target has the `move s2,v1`
 *    join copy and tests `ry & 0x200` against the UNmodified ry. Masking
 *    `ry` in place compiles two insns shorter.
 *  - fold-const association dictates the commit/revert spellings:
 *    `dtL->vy - (0x69 - y)` hits split_tree's CON-VAR case (varsign=-1,
 *    MINUS flips to PLUS) building PLUS(PLUS(vy,-105), y) → the target's
 *    `addiu vy,-105; addu +y` with vy first; the plain `dtL->vy - 0x69 + y`
 *    is reassociated to vy + (y-105), computing `y-105` as an independent
 *    insn that drifts into a delay slot. Likewise `dtL->vy - (oy - 5)`
 *    negates the const and refolds the inner term to PLUS(vy,5), giving
 *    `addiu vy,+5; subu -oy`; both `+ 5 - oy` and `- oy + 5` get mangled to
 *    vy - (oy-5).
 *  - The success-path commit writes vx, vz, THEN vy (`dtL->vx += vect.vx;
 *    dtL->vz += vect.vz; dtL->vy = dtL->vy - (0x69 - y);`): `vect`'s stack
 *    slot is address-taken so its `lh` reads can NEVER be disambiguated
 *    from the `*dtL` stores (sched adds a true dependence on every earlier
 *    store) — with vy stored second, `lh vect.vz` is pinned below the vy
 *    store and the block needs a hazard nop (one insn long); with vy last,
 *    the vz read precedes it in expand order and the scheduler interleaves
 *    the vy chain into the load-delay holes byte-exactly.
 *  - The SECOND feet-height (`(dtL->vy - height) - 400` into `dy`) genuinely
 *    recomputes because `dtL->vy` was modified by the intervening commit;
 *    the FIRST batch reuses the original `yy` (`yy - 400` inline, target's
 *    `addiu $a2,$s0,-0x190`). Both the target asm and m2c confirm the split.
 *  - `ry` is a SIGNED `long` (the read is `lh`, not `lhu`); the s16 copy
 *    `rys` passed to GetMoveSpeed is its own CSE'd narrowing ($s3).
 *  - `Me_MOTION_C->width >> 1` reads the field PLAIN: the fused `sll16/sra17`
 *    falls out of `>> 1` on the signed field; a `(u16)` cast compiles a
 *    wrong `srl`.
 *  - The CVAhuman scan is IDENTICAL to MotionAndMove.c's proven shape
 *    (`HumanAnimType`, `short i`, provably-true `i=0` entry test folded
 *    away leaving a bottom-tested do/while-equivalent `for`) — reused
 *    verbatim here, except the "found" case must `goto` past the
 *    `SetNowMotion` call (shared with the "not found" fallthrough) rather
 *    than returning outright, since both paths still reach the `Sound` call.
 *  - `GlobalAreaMap` and `StagePlayer` are accessed via ABSOLUTE `lui/lw`
 *    (not `%gp_rel`) in this TU — left as plain externs, not added to the
 *    gp-extern list (only Me_MOTION_C/motID/dtL/dtR/MotionUpdateMode/
 *    motMODE are gp-relative here, confirmed by `tools/gpsyms.py`).
 */
extern s16 motID;
extern Humanoid *Me_MOTION_C;
extern s16 motMODE;

extern void GetMoveSpeed(SVECTOR *vect, short ry, short ordr, short side);
extern void PadShockAR(int port, int pow, int attack, int release);

short HangCheck(void)
{
    SVECTOR vect;
    long yy;
    long y;
    long ry;
    long dy;
    long oy;
    long yc;
    short rys;
    short i;

    if ((Me_MOTION_C->type & 0xF0) == 0xA0)
    {
        return 0;
    }
    if (Me_MOTION_C->map.height <= 0 || motID == 0x901 || Me_MOTION_C->itmctl == ITEM_HENSHIN)
    {
        return 0;
    }
    yy = dtL->vy - Me_MOTION_C->height;
    GetMoveSpeed(&vect, dtR->vy, Me_MOTION_C->width >> 1, 0);
    y = GetAreaMapLevel(GlobalAreaMap, dtL->vx + vect.vx, yy - 0x122, dtL->vz + vect.vz, 0);
    dy = yy - 300;
    if (y < dtL->vy)
    {
        y = GetAreaMapLevel(GlobalAreaMap, dtL->vx - vect.vx, yy - 0x122, dtL->vz - vect.vz, 0);
        if (y == 0x80000000)
        {
            return 0;
        }
        if (dtL->vy < y)
        {
            dtL->vx = dtL->vx - (vect.vx >> 1);
            dtL->vz = dtL->vz - (vect.vz >> 1);
        }
        return 0;
    }
    y = GetAreaMapLevel(GlobalAreaMap, dtL->vx, dy, dtL->vz, 0);
    if (y < dtL->vy - Me_MOTION_C->height)
    {
        return 0;
    }
    GetMoveSpeed(&vect, dtR->vy, (Me_MOTION_C->width >> 1) + 300, 0);
    y = GetAreaMapLevel(GlobalAreaMap, dtL->vx + vect.vx, yy - 400, dtL->vz + vect.vz, 2);
    if (y == 0x80000000 || y >= 0x191)
    {
        return 0;
    }
    dtL->vy = dtL->vy - (0x69 - y);
    if (Me_MOTION_C->status == 0xA)
    {
        return 1;
    }
    ry = dtR->vy;
    oy = y;
    if (ry & 0xFF)
    {
        yc = ry & 0xC00;
        if (ry & 0x200)
        {
            yc = yc + 0x400;
        }
        ry = yc;
    }
    rys = ry;
    GetMoveSpeed(&vect, rys, (Me_MOTION_C->width >> 1) + 300, 0);
    dy = (dtL->vy - Me_MOTION_C->height) - 400;
    y = GetAreaMapLevel(GlobalAreaMap, dtL->vx + vect.vx, dy, dtL->vz + vect.vz, 2);
    if (y == 0x80000000 || y >= 0x191)
    {
        dtL->vy = dtL->vy - (oy - 5);
        return 0;
    }
    dtR->vy = ry;
    GetMoveSpeed(&vect, rys, (Me_MOTION_C->width >> 1) + 100, 0);
    y = GetAreaMapLevel(GlobalAreaMap, dtL->vx + vect.vx, dy, dtL->vz + vect.vz, 2);
    if (y != 0x80000000 && y < 0x191)
    {
        GetMoveSpeed(&vect, dtR->vy, -200, 0);
        dtL->vx = dtL->vx + vect.vx;
        dtL->vz = dtL->vz + vect.vz;
        dtL->vy = dtL->vy - (0x69 - y);
    }
    motID = 0xA01;
    motMODE = 1;
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
    SetNowMotion(Me_MOTION_C, motID, motMODE);
    motMODE = -1;
found:
    Sound(Me_MOTION_C, 0x1B);
    if (StagePlayer != Me_MOTION_C)
    {
        return -1;
    }
    PadShockAR(0, 0x7F, 0, 0x1E);
    return -1;
}
