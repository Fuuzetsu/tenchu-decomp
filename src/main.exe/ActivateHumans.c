#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActivateHumans(void);
 *     WORLD.C:752, 41 src lines, frame 48 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     reg   $s1       int i
 *     reg   $s0       struct Humanoid * target
 *     stack sp+16     struct VECTOR vc
 *     reg   $s0       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *StagePlayer;
 *     extern long GameClock;
 *     extern short SkipFrame;
 *     extern struct StageCharType StageChar[18];
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 *     extern int StageID;
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

extern TCameraStatus CamState;
extern Humanoid *StagePlayer;
extern s32 GameClock;
extern s16 SkipFrame;
extern s32 D_800976B8;
extern s16 D_80097F40;
extern s16 D_80097F42;
extern s16 D_80097F44;
extern StageCharType StageChar[18];
extern s16 Humans;
extern Humanoid *HumanGroup[32];
extern s32 StageID;
extern s16 VISIBLE_ENEMIES_;
extern Humanoid *VISIBLE_CHARACTERS_ON_STAGE_[];

extern s32 GetVectorDistance(VECTOR *a, VECTOR *b);
extern s32 GetAreaMapLevel(u_long *area, s32 x, s32 y, s32 z, s32 mode);

/*
 * MATCHED — 1,608 bytes / 402 instructions, 0x68-byte frame, exact CFG.
 *
 * This is one symbolic source for both the exact and normal links.  The direct
 * StageChar array expressions are important: cc1's loop pass shares them as a
 * long-lived HIGH/LO_SUM pair.  The object consequently records one
 * R_MIPS_HI16 at text+0x144 and two R_MIPS_LO16 records at text+0x148 and
 * text+0x3ec.  At the retail address they link to the target's
 * `lui s5,%hi(StageChar)`, `addiu s3,s5,%lo(StageChar)`, and sentinel load.
 *
 * The current global-allocation evidence (tools/regalloc.py) is:
 *   human 77/217 -> $s0, i 9/251 -> $s1,
 *   activate_distance 9/297 -> $s2, StageChar base 8/498 -> $s3,
 *   target 6/302 -> $s4, StageChar high 4/500 -> $s5.
 * The single-iteration loop boundary around the StageChar match block supplies the
 * base's eighth weighted reference.  Removing it leaves 7/498, swaps the base
 * and target between $s3/$s4, and differs by ten bytes.  This is evidence about
 * this cc1 graph, not a claim that the original author used the same spelling.
 *
 * The `active`/`final` join at 0x8003baec (retail `move v0,v1`) is another
 * compiler-sensitive boundary. `active`
 *    must be SImode: narrow locals are genuine QI/HI pseudos here (no
 *    PROMOTE_MODE), so a narrow `active` always widens via sll/andi and can
 *    never yield a bare move (s8 -> `sll v0,v1,0x18`, u8 -> `andi v0,v1,0xff`;
 *    nonzero_bits punts on the paradoxical subreg).  But a plain SImode
 *    `final = active; if (final)` is folded back to a direct test, because
 *    flow.c only builds a LOG_LINK when BLOCK_NUM(use) == BLOCK_NUM(set), and
 *    cse propagates the copy within one extended block.  The identical-arm
 *    fence supplies the block boundary that defeats both -- but jump1 HOISTS a
 *    common leading insn out of two identical arms, which destroys it.  The
 *    dead `final = 0;` makes the arm heads differ so the hoist cannot fire;
 *    jump2 (the only pass with cross_jump=1) then cross-jumps the identical
 *    tails and drops the branch, leaving exactly `move v0,v1; beqz v0`.
 *    Measured: drop `final = 0;` or hoist it out of the arm -> 1604 bytes (the
 *    copy vanishes); `human ? active : active` -> 1604 (fold-const collapses
 *    `a ? b : b`); `if (target)` instead of `if (human)` -> 10 bytes.
 */
void ActivateHumans(void)
{
    s32 i;
    Humanoid *target;
    Humanoid *human;
    VECTOR camera;
    VECTOR query;
    VECTOR work;
    s32 active;
    s32 final;
    s32 initial_active;
    s32 computed_active;
    s32 distance;
    s32 activate_distance;
    s32 n;
    s32 level;
    s16 j;
    ModelType *model;

    target = CamState.Owner;
    camera = *target->locate;
    activate_distance = 26000;
    if (StagePlayer->motion->mid != 0xf05)
    {
        do { do { do { activate_distance = 13000; } while (0); } while (0); } while (0);
    }

    if (GameClock != (GameClock / 30) * 30 || SkipFrame != 0)
    {
        return;
    }

    n = (0xec78 - D_800976B8) / 5000 - 1;
    D_80097F40 = n;
    if ((s16)n < 2)
    {
        n = (u16)D_80097F44 - 1;
    }
    D_80097F44 = n;
    if ((s16)n < 7)
    {
        if ((s16)n < 3)
        {
            n = 3;
        }
    }
    else
    {
        n = 6;
    }
    i = 0;
    D_80097F44 = n;
    D_80097F42 = 0;
    while (1)
    {
        if ((s16)i >= Humans)
        {
            return;
        }
        human = HumanGroup[(s16)i];
        do {
            do {
                if (human == target)
                {
                    goto next_human;
                }
            } while (0);
        } while (0);

    distance = GetVectorDistance(human->locate, &camera);
    if (distance >= 17001)
    {
        goto set_inactive;
    }
    if (((u16)human->type & 0xf0) == 0x80)
    {
        goto set_active;
    }
    initial_active = 1;
    if (human->type == 0xa9 || human->life < 0)
    {
        active = initial_active;
        goto active_done;
    }
    if (GameClock == 30 || StageID == 8)
    {
        goto set_active;
    }
    if (VISIBLE_ENEMIES_ < D_80097F44)
    {
        active = 1;
        if (D_80097F42 < D_80097F44)
        {
            goto active_done;
        }
        computed_active = distance < activate_distance;
        goto computed_active_done;
    }
    if (distance < activate_distance)
    {
        goto near_human;
    }

set_inactive:
    active = 0;
    goto active_done;

near_human:
    if (((u16)human->attribute & 0x80) == 0 &&
        D_80097F42 < D_80097F44)
    {
        goto set_active;
    }
    goto search_visible;

set_active:
    active = 1;
    goto active_done;

search_visible:
    j = 0;
    while (VISIBLE_CHARACTERS_ON_STAGE_[j] != human)
    {
        if (VISIBLE_ENEMIES_ <= j)
        {
            break;
        }
        j++;
    }
    computed_active = j != VISIBLE_ENEMIES_;

computed_active_done:
    active = computed_active;
active_done:
    if (human)
    {
        final = 0;
        final = active;
    }
    else
    {
        final = active;
    }
    if (final)
    {
        if (((u16)human->attribute & 0x80) == 0)
        {
            D_80097F42++;
            goto next_human;
        }
        if (StageID != 8 && human->life >= 0 && GameClock != 30 &&
            (D_80097F42 >= D_80097F44 || distance <= 13000))
        {
            goto next_human;
        }
        human->attribute = (u16)human->attribute & 0xff7f;
        D_80097F42++;
        model = *human->model->object;
        model->attribute |= 0x4000;
        goto next_human;
    }

    if (((u16)human->attribute & 0x80) != 0 || human->type == 0x84)
    {
        goto next_human;
    }
    if ((human->type == 0x87 && (u32)(StageID - 6) < 2) ||
        human->type == 0x83)
    {
        j = 0;
        while (StageChar[(s16)j].stage != -1)
        {
            do {
                if (StageChar[(s16)j].stage == StageID + 1 &&
                    StageChar[(s16)j].chrid == human->type)
                {
                    human->model->locate.coord.t[0] = StageChar[(s16)j].position.vx * 1000;
                    human->model->locate.coord.t[1] = StageChar[(s16)j].position.vy * 1000;
                    human->model->locate.coord.t[2] = StageChar[(s16)j].position.vz * 1000;
                }
            } while (0);
            j++;
        }
        if (human->type == 0x83 && human->life == 0)
        {
            human->life = 1;
        }
    }
    else if (human->status != 0x11 && ((u16)human->attribute & 0x20) == 0)
    {
        memset(&work, 0, sizeof(work));
        work.vx = human->point[0];
        work.vy = human->locate->vy - 1500;
        work.vz = human->point[1];
        query = work;
        if (GetVectorDistance(&query, &camera) > 17000)
        {
            level = GetAreaMapLevel(GlobalAreaMap, query.vx, query.vy,
                                    query.vz, 1);
            if (level != 0x80000000)
            {
                human->model->locate.coord.t[0] = query.vx;
                human->model->locate.coord.t[1] = level;
                human->model->locate.coord.t[2] = query.vz;
            }
        }
    }

    human->attribute = (u16)human->attribute | 0x80;
    model = *human->model->object;
    model->attribute &= 0xbfff;

next_human:
    do { i++; } while (0);
    }
}

// Ghidra decompilation (reference):
//
//
// /* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void ActivateHumans(void)
//
// {
//   bool bVar1;
//   Humanoid *pHVar2;
//   short sVar3;
//   ushort uVar4;
//   VECTOR *pVVar5;
//   int iVar6;
//   long lVar7;
//   int iVar8;
//   Humanoid *pHVar9;
//   ModelType *pMVar10;
//   int iVar11;
//   Humanoid *pHVar12;
//   short sVar13;
//   int iVar14;
//   VECTOR local_50;
//   VECTOR local_40;
//   long local_30;
//   int local_2c;
//   long local_28;
//   long local_24;
//
//   pHVar2 = CamState.Owner;
//   pVVar5 = (CamState.Owner)->locate;
//   local_50.vx = pVVar5->vx;
//   local_50.vy = pVVar5->vy;
//   local_50.vz = pVVar5->vz;
//   local_50.pad = pVVar5->pad;
//   iVar14 = 26000;
//   if (StagePlayer->motion->mid != 0xf05) {
//     iVar14 = 13000;
//   }
//   if ((GameClock == (GameClock / 0x1e) * 0x1e) && (SkipFrame == 0)) {
//     iVar8 = (0xec78 - DAT_800976b8) / 5000 + -1;
//     DAT_80097f40 = (short)iVar8;
//     sVar3 = DAT_80097f40;
//     if (iVar8 * 0x10000 >> 0x10 < 2) {
//       sVar3 = DAT_80097f44 + -1;
//     }
//     if (sVar3 < 7) {
//       if (sVar3 < 3) {
//         sVar3 = 3;
//       }
//     }
//     else {
//       sVar3 = 6;
//     }
//     sVar13 = 0;
//     DAT_80097f42 = 0;
//     DAT_80097f44 = sVar3;
//     while ((int)sVar13 < (int)Humans) {
//       pHVar12 = HumanGroup[sVar13];
//       if (pHVar12 == pHVar2) {
// LAB_8003be28:
//         sVar13 = sVar13 + 1;
//       }
//       else {
//         iVar8 = GetVectorDistance(pHVar12->locate,&local_50);
//         if (iVar8 < 0x4269) {
//           if ((pHVar12->type & 0xf0U) == 0x80) {
// LAB_8003ba7c:
//             bVar1 = true;
//           }
//           else {
//             bVar1 = true;
//             if ((pHVar12->type != 0xa9) && (-1 < pHVar12->life)) {
//               if ((GameClock == 0x1e) || (StageID == 8)) goto LAB_8003ba7c;
//               if (DAT_80097726 < DAT_80097f44) {
//                 bVar1 = true;
//                 if (DAT_80097f44 <= DAT_80097f42) {
//                   bVar1 = iVar8 < iVar14;
//                 }
//               }
//               else {
//                 if (iVar14 <= iVar8) goto LAB_8003ba4c;
//                 if (((pHVar12->attribute & 0x80U) == 0) && (DAT_80097f42 < DAT_80097f44))
//                 goto LAB_8003ba7c;
//                 iVar6 = 0;
//                 sVar3 = 0;
//                 pHVar9 = _DAT_800be858;
//                 while (pHVar9 != pHVar12) {
//                   sVar3 = (short)iVar6;
//                   iVar6 = iVar6 + 1;
//                   if (DAT_80097726 <= sVar3) break;
//                   sVar3 = (short)iVar6;
//                   pHVar9 = *(Humanoid **)(&DAT_800be858 + (iVar6 * 0x10000 >> 0xe));
//                 }
//                 bVar1 = sVar3 != DAT_80097726;
//               }
//             }
//           }
//         }
//         else {
// LAB_8003ba4c:
//           bVar1 = false;
//         }
//         if (!bVar1) {
//           if (((pHVar12->attribute & 0x80U) == 0) && (sVar3 = pHVar12->type, sVar3 != 0x84)) {
//             if (((sVar3 == 0x87) && (StageID - 6U < 2)) || (sVar3 == 0x83)) {
//               iVar8 = 0;
//               if (StageChar[0].stage != -1) {
//                 iVar11 = StageID + 1;
//                 iVar6 = 0;
//                 do {
//                   iVar6 = iVar6 >> 0x10;
//                   if ((StageChar[iVar6].stage == iVar11) &&
//                      (StageChar[iVar6].chrid == pHVar12->type)) {
//                     (pHVar12->model->locate).coord.t[0] = StageChar[iVar6].position.vx * 1000;
//                     (pHVar12->model->locate).coord.t[1] = StageChar[iVar6].position.vy * 1000;
//                     (pHVar12->model->locate).coord.t[2] = StageChar[iVar6].position.vz * 1000;
//                   }
//                   iVar8 = iVar8 + 1;
//                   iVar6 = iVar8 * 0x10000;
//                 } while (StageChar[iVar8 * 0x10000 >> 0x10].stage != -1);
//               }
//               if ((pHVar12->type == 0x83) && (pHVar12->life == 0)) {
//                 pHVar12->life = 1;
//               }
//             }
//             else if ((pHVar12->status != 0x11) && ((pHVar12->attribute & 0x20U) == 0)) {
//               memset((uchar *)&local_30,'\0',0x10);
//               local_40.vx = pHVar12->point[0];
//               local_40.vy = pHVar12->locate->vy + -0x5dc;
//               local_40.vz = pHVar12->point[1];
//               local_40.pad = local_24;
//               local_30 = local_40.vx;
//               local_2c = local_40.vy;
//               local_28 = local_40.vz;
//               iVar8 = GetVectorDistance(&local_40,&local_50);
//               if ((17000 < iVar8) &&
//                  (lVar7 = GetAreaMapLevel(GlobalAreaMap,local_40.vx,local_40.vy),
//                  lVar7 != -0x80000000)) {
//                 (pHVar12->model->locate).coord.t[0] = local_40.vx;
//                 (pHVar12->model->locate).coord.t[1] = lVar7;
//                 (pHVar12->model->locate).coord.t[2] = local_40.vz;
//               }
//             }
//             pHVar12->attribute = pHVar12->attribute | 0x80;
//             pMVar10 = *pHVar12->model->object;
//             uVar4 = pMVar10->attribute & 0xbfff;
// LAB_8003be24:
//             pMVar10->attribute = uVar4;
//           }
//           goto LAB_8003be28;
//         }
//         if ((pHVar12->attribute & 0x80U) != 0) {
//           if ((((StageID == 8) || (pHVar12->life < 0)) || (GameClock == 0x1e)) ||
//              ((DAT_80097f42 < DAT_80097f44 && (13000 < iVar8)))) {
//             pHVar12->attribute = pHVar12->attribute & 0xff7f;
//             DAT_80097f42 = DAT_80097f42 + 1;
//             pMVar10 = *pHVar12->model->object;
//             uVar4 = pMVar10->attribute | 0x4000;
//             goto LAB_8003be24;
//           }
//           goto LAB_8003be28;
//         }
//         DAT_80097f42 = DAT_80097f42 + 1;
//         sVar13 = sVar13 + 1;
//       }
//     }
//   }
//   return;
// }
