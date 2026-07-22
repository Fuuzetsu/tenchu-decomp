#include "common.h"
#include "main.exe.h"
#include "item.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ControlHumanoid(struct Humanoid *human);
 *     HUMAN.C:107, 44 src lines, frame 72 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     param $s0       struct Humanoid * human
 *     reg   $s1       struct ModelArchiveType * model
 *     reg   $s2       long m
 *     stack sp+24     struct MATRIX mat
 *
 * Globals it touches, as the original declared them:
 *     extern enum TSystemFlag SystemFlag;
 *     extern short SkipFrame;
 *     extern struct Humanoid *StagePlayer;
 *     extern short ActionHalt;
 * END PSX.SYM */

/*
 * MATCHED.  394 instructions / 1576 bytes, 0 differing bytes.
 *
 * Three source facts are load-bearing here; all three were verified against the
 * pinned gcc-2.8.1 source (global.c) and the .lreg/.greg dumps.
 *
 * 1. GetDirection's third parameter is `short` (reference/psxsym-protos.h), and
 *    the three-term rotation sum must be narrowed through a VAR_DECL.
 *    convert_to_integer distributes an outer (s16) cast into a PLUS_EXPR's
 *    operands, making every halfword leaf narrow-use-only (`lhu`) and folding
 *    into the last operand's register; it cannot distribute into a VAR_DECL.
 *    Naming the full sum (`rotation_pair`) is the fix.
 * 2. The player-yaw sum and the enemy rotation sum are ONE reused variable
 *    (`rotation_pair`); retail keeps both in $v0.  Splitting them per branch
 *    drops both sums to 2 refs each, which hands them to local_alloc and
 *    perturbs the whole player block (measured: 68 bytes).
 * 3. The abs magnitude is THREE separate variables, one per site -- spelled as
 *    the PSX.SYM header describes ("a repeated name is a nested-block scope"),
 *    i.e. the two player sites shadow the enemy one.  Only the DISTINCT VAR_DECLs
 *    matter; the names do not.  This is what puts each magnitude in $a0, and it
 *    is pure global-alloc priority:
 *
 *      global.c:allocno_compare ranks allocnos by
 *          floor_log2(n_refs) * n_refs / live_length * 10000 * size
 *      (descending), and `.lreg` prints both inputs as
 *      "Register N used R times across L insns".
 *
 *    `direction` is 20 refs / 45 insns = 17777.  A SINGLE `magnitude` covering
 *    all three sites is 16 refs / 26 insns = 24615, so it is coloured FIRST.
 *    It has no hard-reg preference of its own (set_preference strips one level
 *    of the src expression and takes operand 0; for `(set mag (abs dir))` that
 *    is a global pseudo, and every other donating site yields $v0, which
 *    magnitude hard-conflicts with).  Meanwhile `direction` INHERITS $a0 from
 *    `rotation_pair`: expand_preferences unions hard-reg preferences BOTH WAYS
 *    between a single_set's global dest and any non-conflicting global carrying
 *    a REG_DEAD note on that insn, and `direction = CamState.DirectionRY -
 *    rotation_pair` kills rotation_pair.  rotation_pair prefers $a0 because its
 *    player-branch sum `(set rp (plus obj0_load obj1_load))` has operand 0 =
 *    the obj[0] load, which local_alloc colours $a0 (`lh $a0,0x52($v1)`).
 *    prune_preferences then puts $a0 into regs_someone_prefers[magnitude] (the
 *    union of LOWER-priority conflicting allocnos' preferences), so the single
 *    magnitude avoids $a0 and lands in $a1 -- 22 bytes, a pure 19-instruction
 *    a0->a1 rename.
 *
 *    Splitting per site drops each magnitude through a floor_log2 step and
 *    below `direction`, so `direction` is coloured first, takes its own $v1,
 *    and leaves regs_someone_prefers empty for each magnitude -- whose fallback
 *    scan then hits $a0 (it conflicts with hard $v0, and $v1 is taken):
 *        enemy magnitude   6 refs / 12 insns = 10000
 *        yaw   magnitude   5 refs /  7 insns = 14285
 *        pitch magnitude   5 refs /  7 insns = 14285
 *    all < direction's 17777.  The three sites are disjoint, so all three share
 *    $a0.  Each half still spans basic blocks (the abs itself branches), so
 *    unlike rotation_pair they remain GLOBAL allocnos -- which is why splitting
 *    magnitude wins where splitting rotation_pair loses.
 *
 * Dead ends, measured, so they are not retried: a `do{}while(0)` weighting
 * fence on the enemy-vertical block does add loop-depth-weighted refs to
 * `direction` (20 -> 24) and leaves live_length alone, but its LOOP_END note
 * lands next to the cross-jumped `UpdateCoordinate` tail and blocks the merge:
 * 282 bytes.  There is no free ref to remove from magnitude either -- its 16
 * refs correspond exactly to 19 emitted instructions (3 abs defs of 2 insns
 * each + 13 single-insn uses), so none is combine-folded.
 */

extern TSystemFlag SystemFlag;
extern s16 SkipFrame;
extern s16 ActionHalt;
extern s16 VISIBLE_ENEMIES_;
extern s16 D_800BE768[];
extern Humanoid *VISIBLE_CHARACTERS_ON_STAGE_[];
extern s16 DrawTMDmode;
extern char D_80011668[];
extern char D_80011684[];
extern char D_80011694[];
extern char D_800116A4[];

extern s16 DefaultActionHumanoid(Humanoid *human);
extern void StateTransition(Humanoid *human);
extern void DrawShadow(Humanoid *human);
extern void register_character_death(Humanoid *human);
extern void death_camera_something_(Humanoid *human);
extern void HumanActionControl(Humanoid *human);
extern s32 DrawClip(ModelType *model, s32 *xy);
extern s16 PlayMotion(MotionManager *motion, s16 mode);
extern void UpdateCoordinate(ModelType *model);
extern s16 GetDirection(s32 dx, s32 dz, s16 roty);

void ControlHumanoid(Humanoid *human)
{
    ModelArchiveType *model;
    s32 m;
    MATRIX mat;
    ModelType *head;
    s32 direction;
    s32 magnitude;
    s32 rotation_pair;

    model = human->model;
    m = 1;
    if (model->object[0]->id >= 0)
    {
        DefaultActionHumanoid(human);
        StateTransition(human);
        DrawShadow(human);
    }
    else
    {
        if (human->status == 0x11)
        {
            register_character_death(human);
            death_camera_something_(human);
        }
    }
    HumanActionControl(human);
    if ((SystemFlag & SYSFLAG_DEBUGMODE) != 0)
    {
        if (SkipFrame != 0)
        {
            goto skip_draw;
        }
        if (human == StagePlayer)
        {
            FntPrint(D_80011668, human->type,
                     human->locate->vx / 1000,
                     human->locate->vy / 1000,
                     human->locate->vz / 1000);
            FntPrint(D_80011684, (u16)human->attribute, (u8)human->status);
            FntPrint(D_80011694, (u8)human->motion->mid,
                     human->motion->loop, human->motion->count);
            FntPrint(D_800116A4, human->rotate->vy,
                     human->model->object[0]->id);
        }
    }

    if (SkipFrame == 0)
    {
        goto do_draw;
    }
skip_draw:
    m = 0;
    goto draw_done;
do_draw:
    if (human != StagePlayer)
    {
        s32 clip;

        GsGetLs((GsCOORDINATE2 *)model, &mat);
        GsSetLsMatrix(&mat);
        clip = DrawClip((ModelType *)model, 0);
        m = 0;
        if (clip >= 0)
        {
            m = -1;
        }
    }
draw_done:

    PlayMotion(human->motion, human->status == 7 ? -1 : m);
    human->slocate = *human->locate;
    human->locate->vx += human->vector.vx;
    human->locate->vz += human->vector.vz;
    human->locate->vy += human->vector.vy;
    UpdateCoordinate((ModelType *)model);

    if (m == 0)
    {
        return;
    }

    D_800BE768[VISIBLE_ENEMIES_] = DrawTMDmode;
    VISIBLE_CHARACTERS_ON_STAGE_[VISIBLE_ENEMIES_] = human;
    VISIBLE_ENEMIES_++;
    if (ActionHalt != 0 || human->life <= 0)
    {
        return;
    }

    if (human == StagePlayer)
    {
        if (human->status == 0xc)
        {
            return;
        }
        head = human->model->object[2];
        if (CamState.Mode != CMODE_DIRECTION && CamState.Mode != CMODE_SIGHT)
        {
            MotionElementType *rotation;

            if (human->motion->loop != -1)
            {
                return;
            }
            rotation = human->motion->motion->rotate[2];
            if (head->rotate.vx == rotation->x &&
                head->rotate.vy == rotation->y)
            {
                return;
            }
            head->rotate.vx = rotation->x;
            head->rotate.vy = human->motion->motion->rotate[2]->y;
            UpdateCoordinate(head);
            return;
        }
        else
        {
            rotation_pair = human->model->object[0]->rotate.vy +
                            human->model->object[1]->rotate.vy;
            {
                s32 magnitude;

                direction = CamState.DirectionRY - rotation_pair;
                magnitude = direction >= 0 ? direction : -direction;
                if (magnitude >= 0x385)
                {
                    head->rotate.vy = magnitude * 900 / direction;
                }
                else
                {
                    head->rotate.vy = direction;
                }
            }
            {
                s32 magnitude;

                direction = CamState.DirectionRX;
                magnitude = direction >= 0 ? direction : -direction;
                if (magnitude > 500)
                {
                    head->rotate.vx = magnitude * 500 / direction;
                }
                else
                {
                    head->rotate.vx = direction;
                }
            }
        }
        UpdateCoordinate(head);
        return;
    }
    else
    {
        if ((human->attribute & 3) != 2)
        {
            if (human->target == StagePlayer->model)
            {
                return;
            }
            if (human->motion->mid == 0x100)
            {
                return;
            }
        }

        rotation_pair = human->model->object[0]->rotate.vy +
                        human->model->object[1]->rotate.vy +
                        human->rotate->vy;
        direction = GetDirection(
            human->target->locate.coord.t[0] - human->locate->vx,
            human->target->locate.coord.t[2] - human->locate->vz,
            rotation_pair);
        magnitude = direction >= 0 ? direction : -direction;
        if (magnitude >= 0x708)
        {
            return;
        }

        head = human->model->object[2];
        if (magnitude >= 0x385)
        {
            head->rotate.vy = magnitude * 900 / direction;
        }
        else
        {
            head->rotate.vy = direction;
        }

        direction = (human->target->locate.coord.t[1] - human->locate->vy) / 2;
        if (direction != 0)
        {
            if (direction >= -500)
            {
                if (direction < 101)
                {
                    head->rotate.vx = direction;
                }
                else
                {
                    head->rotate.vx = 100;
                }
            }
            else
            {
                head->rotate.vx = -500;
            }
        }
        UpdateCoordinate(head);
    }
}

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
//
// void ControlHumanoid(Humanoid *human)
//
// {
//   short sVar1;
//   MotionManager *pMVar2;
//   VECTOR *pVVar3;
//   ModelType **ppMVar4;
//   short *psVar5;
//   int iVar6;
//   long lVar7;
//   long lVar8;
//   long lVar9;
//   int iVar10;
//   ModelType *pMVar11;
//   MATRIX MStack_30;
//
//   pMVar11 = (ModelType *)human->model;
//   iVar10 = 1;
//   if (*(short *)(((pMVar11->object).coord2)->flg + 0x58) < 0) {
//     if (human->status == 0x11) {
//       FUN_8002bcb8(human);
//       FUN_8003818c(human);
//     }
//   }
//   else {
//     DefaultActionHumanoid(human);
//     StateTransition(human);
//     DrawShadow(human);
//   }
//   HumanActionControl(human);
//   if ((SystemFlag & 2) == 0) {
// LAB_80027c6c:
//     if (SkipFrame != 0) goto LAB_80027c80;
//     if (human != StagePlayer) {
//       GsGetLs((GsCOORDINATE2 *)pMVar11,&MStack_30);
//       GsSetLsMatrix(&MStack_30);
//       lVar7 = DrawClip(pMVar11,(long *)0x0);
//       iVar10 = 0;
//       if (-1 < lVar7) {
//         iVar10 = -1;
//       }
//     }
//   }
//   else {
//     if (SkipFrame == 0) {
//       if (human == StagePlayer) {
//         iVar6 = human->locate->vy / 1000;
//         FntPrint("~c800%02x~c888(%d,%d,%d) ",(char *)(int)human->type,
//                  (char *)(human->locate->vx / 1000),iVar6);
//         FntPrint("~c880%04x=%02x ",(char *)(uint)(ushort)human->attribute,
//                  (char *)(uint)(byte)human->status,iVar6);
//         pMVar2 = human->motion;
//         iVar6 = (int)pMVar2->count;
//         FntPrint("~c080%02x/%d%d ",(char *)(uint)(byte)pMVar2->mid,(char *)(int)pMVar2->loop,iVar6);
//         FntPrint("~c088%03x ~c888%d\n",(char *)(int)human->rotate->vy,
//                  (char *)(int)(*human->model->object)->id,iVar6);
//       }
//       goto LAB_80027c6c;
//     }
// LAB_80027c80:
//     iVar10 = 0;
//   }
//   iVar6 = -1;
//   if (human->status != 7) {
//     iVar6 = iVar10;
//   }
//   PlayMotion(human->motion,(short)iVar6);
//   pVVar3 = human->locate;
//   lVar7 = pVVar3->vy;
//   lVar8 = pVVar3->vz;
//   lVar9 = pVVar3->pad;
//   (human->slocate).vx = pVVar3->vx;
//   (human->slocate).vy = lVar7;
//   (human->slocate).vz = lVar8;
//   (human->slocate).pad = lVar9;
//   human->locate->vx = human->locate->vx + (int)(human->vector).vx;
//   human->locate->vz = human->locate->vz + (int)(human->vector).vz;
//   human->locate->vy = human->locate->vy + (int)(human->vector).vy;
//   UpdateCoordinate(pMVar11);
//   if (iVar10 == 0) {
//     return;
//   }
//   iVar10 = (int)DAT_80097726;
//   (&DAT_800be768)[iVar10] = _DrawTMDmode;
//   sVar1 = ActionHalt;
//   *(Humanoid **)(&DAT_800be858 + iVar10 * 4) = human;
//   DAT_80097726 = DAT_80097726 + 1;
//   if (sVar1 != 0) {
//     return;
//   }
//   if (human->life < 1) {
//     return;
//   }
//   if (human == StagePlayer) {
//     if (human->status == 0xc) {
//       return;
//     }
//     pMVar11 = human->model->object[2];
//     if ((CamState.Mode != CMODE_DIRECTION) && (CamState.Mode != CMODE_SIGHT)) {
//       if (human->motion->loop != -1) {
//         return;
//       }
//       psVar5 = *(short **)&human->motion->motion[1].time;
//       if (((pMVar11->rotate).vx == *psVar5) && ((pMVar11->rotate).vy == psVar5[1])) {
//         return;
//       }
//       (pMVar11->rotate).vx = *psVar5;
//       (pMVar11->rotate).vy = *(short *)(*(int *)&human->motion->motion[1].time + 2);
//       goto LAB_800280e8;
//     }
//     ppMVar4 = human->model->object;
//     iVar6 = (int)CamState.DirectionRY -
//             ((int)((*ppMVar4)->rotate).vy + (int)(ppMVar4[1]->rotate).vy);
//     iVar10 = iVar6;
//     if (iVar6 < 0) {
//       iVar10 = -iVar6;
//     }
//     if (iVar10 < 0x385) {
//       (pMVar11->rotate).vy = (short)iVar6;
//     }
//     else {
//       if (iVar6 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar6 == -1) && (iVar10 * 900 == -0x80000000)) {
//         trap(0x1800);
//       }
//       (pMVar11->rotate).vy = (short)((iVar10 * 900) / iVar6);
//     }
//     iVar10 = (int)CamState.DirectionRX;
//     iVar6 = iVar10;
//     if (iVar10 < 0) {
//       iVar6 = -iVar10;
//     }
//     if (500 < iVar6) {
//       if (iVar10 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar10 == -1) && (iVar6 * 500 == -0x80000000)) {
//         trap(0x1800);
//       }
//       (pMVar11->rotate).vx = (short)((iVar6 * 500) / iVar10);
//       goto LAB_800280e8;
//     }
//   }
//   else {
//     if ((human->attribute & 3U) != 2) {
//       if (human->target == (ModelType *)StagePlayer->model) {
//         return;
//       }
//       if (human->motion->mid == 0x100) {
//         return;
//       }
//     }
//     ppMVar4 = human->model->object;
//     sVar1 = GetDirection((human->target->locate).coord.t[0] - human->locate->vx,
//                          (human->target->locate).coord.t[2] - human->locate->vz,
//                          ((*ppMVar4)->rotate).vy + (ppMVar4[1]->rotate).vy + human->rotate->vy);
//     iVar6 = (int)sVar1;
//     iVar10 = iVar6;
//     if (iVar6 < 0) {
//       iVar10 = -iVar6;
//     }
//     if (0x707 < iVar10) {
//       return;
//     }
//     pMVar11 = human->model->object[2];
//     if (iVar10 < 0x385) {
//       (pMVar11->rotate).vy = sVar1;
//     }
//     else {
//       if (iVar6 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar6 == -1) && (iVar10 * 900 == -0x80000000)) {
//         trap(0x1800);
//       }
//       (pMVar11->rotate).vy = (short)((iVar10 * 900) / iVar6);
//     }
//     iVar10 = ((human->target->locate).coord.t[1] - human->locate->vy) / 2;
//     if (iVar10 == 0) goto LAB_800280e8;
//     sVar1 = -500;
//     if ((iVar10 < -500) || (sVar1 = 100, 100 < iVar10)) {
//       (pMVar11->rotate).vx = sVar1;
//       goto LAB_800280e8;
//     }
//   }
//   (pMVar11->rotate).vx = (short)iVar10;
// LAB_800280e8:
//   UpdateCoordinate(pMVar11);
//   return;
// }
