#include "common.h"
#include "sound.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActSYURI(void);
 *     MOTION.C:1908, 37 src lines, frame 64 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * human->status). Two motion ids: MOT_SYURI (throw) spawns the shuriken item at
 * the weapon-hand model's absolute position on frame 1 (count == 1), or —
 * past that frame — checks the spare-shuriken slot (spare_item_slot_) and
 * drops to recover when it runs out or the player cancels via pad.trig &
 * (PADRleft | PADRdown | PADRright); MOT_SYURI_RECOVER restocks the AI's
 * shuriken (ReqItemDefault) and returns to motion 0 or the weapon-drawn
 * engage stance (MOT_ENGAGE_STANCE, attribute & ATTR_WEAPON_DRAWN) when the
 * motion runs out.
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
 *  - `motMODE = MOTION_MOVE_APPLY;` is DUPLICATED into both arms of each motID=0x501/0
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

extern Humanoid *Me_MOTION_C;

extern s32 spare_item_slot_(s32 mode, Humanoid *human);
extern int ReqItemUse(PARAM_ITEM_LAUNCH *p);

void ActSYURI(void)
{
    VECTOR *p;
    PARAM_ITEM_LAUNCH item;

    switch (dtM->mid)
    {
    case MOT_SYURI:
        if (Me_MOTION_C != StagePlayer)
        {
            if (dtM->count != 0)
                return;
            if (dtM->loop == 0)
                return;
            SET_MOTION(MOT_SYURI_RECOVER, MOTION_MOVE_APPLY);
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            dtM->loop = MOTION_LOOP_DISABLED;
        }
        if (dtM->count == 1)
        {
            item.type = ITEM_SHURIKEN;
            item.user = Me_MOTION_C;
            p = GetAbsolutePosition(
                Me_MOTION_C->model->object[MODEL_PART_HEAD], 0, 0, 0);
            item.start.vx = p->vx;
            item.start.vy = p->vy;
            item.start.vz = p->vz;
            item.end = item.start;
            ReqItemUse(&item);
            Sound(Me_MOTION_C, SE_THROW_WEAPON);
        }
        else if (spare_item_slot_(1, Me_MOTION_C) == 0)
        {
            SET_MOTION(MOT_SYURI_RECOVER, MOTION_MOVE_APPLY);
            Sound(Me_MOTION_C, SE_WEAPON_RECOVER);
        }
        else if (Me_MOTION_C->pad.trig & (PADRleft | PADRdown | PADRright))
        {
            spare_item_slot_(0, 0);
            SELECT_RETURN_MOTION();
        }
        break;
    case MOT_SYURI_RECOVER:
        if (dtM->count == 1 && Me_MOTION_C != StagePlayer)
        {
            ReqItemDefault(Me_MOTION_C, ITEM_SHURIKEN);
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            SELECT_RETURN_MOTION();
        }
        break;
    }
}
