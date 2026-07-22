#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActSYURI(void);
 *     MOTION.C:1908, 37 src lines, frame 64 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     reg   $v0       struct VECTOR * p
 *     stack sp+16     struct PARAM_ITEM_LAUNCH item
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern struct Humanoid *StagePlayer;
 *     extern short motID;
 *     extern short motMODE;
 * END PSX.SYM */

/*
 * ActSYURI (0x80025dd0, 0x2a4 bytes) — the shuriken-throw action state
 * (MOTION.C's ActionFunc[] table, dispatched by HumanActionControl on
 * human->status). Two motion ids: 0xE00 (throw) spawns the shuriken item at
 * the weapon-hand model's absolute position on frame 1 (count == 1), or —
 * past that frame — holds the aim lock (FUN_8004a368) until it breaks or the
 * player cancels via pad.trig & 0xE0; 0xE01 (recover) restocks the AI's
 * shuriken (ReqItemDefault) and returns to motion 0 or 0x501 (attribute &
 * 0x40 = crouching?) when the motion runs out.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `switch (dtM->mid)` (a real switch: sequential beqs + `j default`,
 *    bodies in source order). The E01 case's `count == 1` compare constant
 *    is reorg-stolen into the dispatch beq's delay slot — automatic.
 *  - The `1` in the E00 case is ONE cse-unified pseudo (callee-saved $s0):
 *    the `count == 1` compare constant, `item.type = ITEM_SHURIKEN`
 *    (=1) word store, and all three `motMODE = 1` halfword stores fold
 *    onto it via cse's taken-edge path following. No named variable needed
 *    (PSX.SYM lists only `p` and `item` — consistent).
 *  - `motMODE = 1;` is DUPLICATED into both arms of each motID=0x501/0
 *    if/else (the cookbook's "duplicate the shared trailing statement"
 *    shared-tails rule): in E01 sched2 (which runs BEFORE jump2 here) then
 *    hoists the else-arm's `li v0,1` above its store and cross-jump merges
 *    only the `sh` into the epilogue-adjacent island; in E00 nothing merges
 *    (the fall-in predecessor of the end label stores v0, these arms s0).
 *  - `item.end = item.start;` is a whole-VECTOR (align-4) struct assignment
 *    → the batched 4×lw/4×sw t0–t3 block move.
 *  - `attribute`@0x4 is read `lhu` in this TU (item.h proves it s16):
 *    the same `*(u16 *)&` memory-reinterpret cast HumanActionControl.c
 *    documents for this TU's attribute/attrib fields.
 *  - gp-externs (MOTION.C's own smalls): dtM, Me_MOTION_C, motID,
 *    motMODE. StagePlayer stays absolute (defined elsewhere).
 */

extern MotionManager *dtM;
extern Humanoid *Me_MOTION_C;
extern s16 motID;
extern s16 motMODE;

extern s32 FUN_8004a368(s32 arg0, Humanoid *arg1);
extern int ReqItemUse(PARAM_ITEM_LAUNCH *p);
extern short Sound(Humanoid *human, int seid);

void ActSYURI(void)
{
    VECTOR *p;
    PARAM_ITEM_LAUNCH item;

    switch (dtM->mid)
    {
    case 0xE00:
        if (Me_MOTION_C != StagePlayer)
        {
            if (dtM->count != 0)
                return;
            if (dtM->loop == 0)
                return;
            motID = 0xE01;
            motMODE = 1;
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            dtM->loop = -1;
        }
        if (dtM->count == 1)
        {
            item.type = ITEM_SHURIKEN;
            item.user = Me_MOTION_C;
            p = GetAbsolutePosition(Me_MOTION_C->model->object[2], 0, 0, 0);
            item.start.vx = p->vx;
            item.start.vy = p->vy;
            item.start.vz = p->vz;
            item.end = item.start;
            ReqItemUse(&item);
            Sound(Me_MOTION_C, 0x1E);
        }
        else if (FUN_8004a368(1, Me_MOTION_C) == 0)
        {
            motID = 0xE01;
            motMODE = 1;
            Sound(Me_MOTION_C, 0x1F);
        }
        else if (Me_MOTION_C->pad.trig & 0xE0)
        {
            FUN_8004a368(0, 0);
            if (Me_MOTION_C == StagePlayer)
            {
                SetCameraMode(CMODE_NORMAL);
            }
            if (*(u16 *)&Me_MOTION_C->attribute & 0x40)
            {
                motID = 0x501;
                motMODE = 1;
            }
            else
            {
                motID = 0;
                motMODE = 1;
            }
        }
        break;
    case 0xE01:
        if (dtM->count == 1 && Me_MOTION_C != StagePlayer)
        {
            ReqItemDefault(Me_MOTION_C, ITEM_SHURIKEN);
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            if (Me_MOTION_C == StagePlayer)
            {
                SetCameraMode(CMODE_NORMAL);
            }
            if (*(u16 *)&Me_MOTION_C->attribute & 0x40)
            {
                motID = 0x501;
                motMODE = 1;
            }
            else
            {
                motID = 0;
                motMODE = 1;
            }
        }
        break;
    }
}
