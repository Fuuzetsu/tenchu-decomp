#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SearchTarget(struct Humanoid *human, long *distance, short *degree);
 *     HUMAN.C:436, 46 src lines, frame 64 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     param $s1       struct Humanoid * human
 *     param $s3       long * distance
 *     param $s4       short * degree
 *     reg   $s2       struct VECTOR * head
 *     stack sp+16     struct VECTOR vect
 *     stack sp+32     struct SVECTOR svect
 *     reg   $s0       short mode
 *     reg   $a0       long dx
 *     reg   $a1       long dz
 *     reg   $s0       short roty
 *     reg   $a1       short degree
 *     reg   $a3       short n
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct Humanoid *StagePlayer;
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

/*
 * STATUS: MATCHING — exact 1128-byte / 282-instruction pure-C match.
 * The stack plan is frame 0x50, delta VECTOR at sp+0x10, position VECTOR at
 * sp+0x20, and passage SVECTOR at sp+0x30.
 *
 * The passage failure's SImode -2 is narrowed through `passage_pad`, crosses
 * a zero-code loop fence, and is widened through identical arms.  This strips
 * the literal equivalence until jump2 without emitting code, so retail's late
 * `j`/`li -2` island survives while the close-distance return folds directly
 * into its conditional branch.
 *
 * The three deltas must be one stack VECTOR; `mode` must remain s16 for the
 * target's repeated promotion/copy chains; and the vertical adjustment needs
 * distinct `initial_delta_y`, updated `delta_y`, and `base_y` identities.
 */

typedef struct
{
    s32 near_distance;
    s32 clear_distance;
    s32 far_distance;
} SearchSight;

extern SearchSight searchsight[];


short SearchTarget(Humanoid *human, long *distance, short *degree)
{
    VECTOR delta;
    VECTOR vect;
    SVECTOR svect;
    s32 raw_degree;
    s32 roty;
    s16 mode;
    s32 limit;
    s32 absolute;
    s32 y;
    s32 base_y;
    s32 own_height;
    s32 initial_delta_y;
    s32 delta_y;
    s32 full_height;
    s32 half_height;
    s32 adjusted_y;
    u16 player_height;
    s16 signed_degree;
    s16 result_degree;
    s16 n;

    vect = *human->locate;
    n = 1;
    if (human->target == 0)
    {
        return 0;
    }

    delta.vx = human->target->locate.coord.t[0] - vect.vx;
    delta.vy = human->target->locate.coord.t[1] - vect.vy;
    delta.vz = human->target->locate.coord.t[2] - vect.vz;
    *distance = SquareRoot0(delta.vx * delta.vx + delta.vy * delta.vy +
                            delta.vz * delta.vz);

    roty = (u16)human->rotate->vy;
    raw_degree = ratan2(-delta.vx, -delta.vz) - roty;
    signed_degree = raw_degree;
    result_degree = raw_degree;
    if (signed_degree < 0x801)
    {
        goto degree_nested;
    }
    result_degree = 0x1000 - raw_degree;
    goto degree_done;
degree_nested:
    if (signed_degree < -0x7ff)
    {
        result_degree = raw_degree + 0x1000;
    }
degree_done:
    *degree = result_degree;

    if (((GameClock + human->model->object[0]->id) & 0x1f) != 0)
    {
        return 0;
    }

    mode = (u16)(StagePlayer->status - 0xb) < 2;
    if (StagePlayer->status == 10)
    {
        if (delta.vy >= 0)
        {
            goto passage_failure;
        }
        if (delta.vy < -3000)
        {
            if (*distance < 4000)
            {
                return -2;
            }
        }
    }

    if (__builtin_abs(delta.vy) >= 3000)
    {
        if (FRAMES_UNTIL_END_OF_ALERT == 0)
        {
            return -2;
        }
        if (*distance < 4000)
        {
            goto passage_failure;
        }
    }

    if (*distance >= searchsight[mode].far_distance)
    {
        return -2;
    }

    absolute = __builtin_abs(*degree);
    if (absolute < 900)
    {
        if (absolute >= 450 && mode != 0)
        {
            return -1;
        }
        if (*distance >= searchsight[mode].near_distance)
        {
            return -1;
        }

        limit = 500;
        if (mode != 0)
        {
            limit = 300;
        }
        y = vect.vy;
        own_height = human->height;
        initial_delta_y = delta.vy;
        y += 300;
        y -= own_height;
        delta_y = initial_delta_y - 300;
        vect.vy = y;
        base_y = delta_y + human->height;
        delta.vy = base_y;
        player_height = StagePlayer->height;
        full_height = (s16)player_height;
        if (StagePlayer->status == 0xb)
        {
            half_height = (s16)player_height / 2;
            adjusted_y = base_y - half_height;
        }
        else
        {
            adjusted_y = base_y - full_height;
        }
        delta.vy = adjusted_y;

        while (limit < __builtin_abs(delta.vx) ||
               limit < __builtin_abs(delta.vy) ||
               limit < __builtin_abs(delta.vz))
        {
            n <<= 1;
            delta.vx >>= 1;
            delta.vy >>= 1;
            delta.vz >>= 1;
        }

        svect.vx = delta.vx;
        svect.vy = delta.vy;
        svect.vz = delta.vz;
        if (GetAreaMapPassage(GlobalAreaMap, &vect, &svect, n) != 0)
        {
passage_failure:
            {
                s32 passage_raw;
                s16 passage_pad;
                s32 passage_result;

                passage_raw = -2;
                passage_pad = passage_raw;
                do
                {
                } while (0);
                if (mode != 0)
                {
                    passage_result = (s32)passage_pad;
                }
                else
                {
                    passage_result = (s32)passage_pad;
                }
                return passage_result;
            }
        }
        return (*distance < searchsight[mode].clear_distance) ? 1 : 2;
    }
    return -1;
}

// Historical Ghidra decompilation retained as a reconstruction reference:
//
//
// /* WARNING: Type propagation algorithm not settling */
//
// short SearchTarget(Humanoid *human,long *distance,short *degree)
//
// {
//   ushort uVar1;
//   VECTOR *pVVar2;
//   int iVar3;
//   long lVar4;
//   uint uVar5;
//   int iVar6;
//   int iVar7;
//   short sVar8;
//   short n;
//   int local_40;
//   int local_3c;
//   int local_38;
//   VECTOR local_30;
//   SVECTOR local_20;
//
//   pVVar2 = human->locate;
//   local_30.vx = pVVar2->vx;
//   local_30.vy = pVVar2->vy;
//   local_30.vz = pVVar2->vz;
//   local_30.pad = pVVar2->pad;
//   n = 1;
//   if (human->target == (ModelType *)0x0) {
//     return 0;
//   }
//   local_40 = (human->target->locate).coord.t[0] - local_30.vx;
//   iVar3 = (human->target->locate).coord.t[1] - local_30.vy;
//   local_38 = (human->target->locate).coord.t[2] - local_30.vz;
//   lVar4 = SquareRoot0(local_40 * local_40 + iVar3 * iVar3 + local_38 * local_38);
//   *distance = lVar4;
//   uVar1 = human->rotate->vy;
//   lVar4 = ratan2(-local_40,-local_38);
//   iVar6 = lVar4 - (uint)uVar1;
//   iVar7 = iVar6 * 0x10000 >> 0x10;
//   sVar8 = (short)iVar6;
//   if (iVar7 < 0x801) {
//     if (iVar7 < -0x7ff) {
//       sVar8 = sVar8 + 0x1000;
//     }
//   }
//   else {
//     sVar8 = 0x1000 - sVar8;
//   }
//   *degree = sVar8;
//   if ((GameClock + (*human->model->object)->id & 0x1fU) != 0) {
//     return 0;
//   }
//   uVar5 = (uint)((ushort)StagePlayer->status - 0xb < 2);
//   if (StagePlayer->status == 10) {
//     if (-1 < iVar3) {
//       return -2;
//     }
//     if ((iVar3 < -3000) && (*distance < 4000)) {
//       return -2;
//     }
//   }
//   iVar6 = iVar3;
//   if (iVar3 < 0) {
//     iVar6 = -iVar3;
//   }
//   if (2999 < iVar6) {
//     if (DAT_800979c0 == 0) {
//       return -2;
//     }
//     if (*distance < 4000) {
//       return -2;
//     }
//   }
//   if (*(int *)((int)searchsight + uVar5 * 0xc + 8) <= *distance) {
//     return -2;
//   }
//   iVar6 = (int)*degree;
//   if (iVar6 < 0) {
//     iVar6 = -iVar6;
//   }
//   if (iVar6 < 900) {
//     if ((0x1c1 < iVar6) && (uVar5 != 0)) {
//       return -1;
//     }
//     if (*(int *)((int)searchsight + uVar5 * 0xc) <= *distance) {
//       return -1;
//     }
//     iVar6 = 500;
//     if (uVar5 != 0) {
//       iVar6 = 300;
//     }
//     local_30.vy = (local_30.vy + 300) - (int)human->height;
//     iVar7 = (uint)(ushort)StagePlayer->height << 0x10;
//     local_3c = iVar7 >> 0x10;
//     if (StagePlayer->status == 0xb) {
//       local_3c = local_3c - (iVar7 >> 0x1f) >> 1;
//     }
//     local_3c = (iVar3 + -300 + (int)human->height) - local_3c;
//     iVar3 = local_40;
//     if (local_40 < 0) {
//       iVar3 = -local_40;
//     }
//     if (iVar6 < iVar3) goto LAB_80028ea4;
//     iVar3 = local_3c;
//     if (local_3c < 0) {
//       iVar3 = -local_3c;
//     }
//     if (iVar6 < iVar3) goto LAB_80028ea4;
//     n = 1;
//     iVar3 = local_38;
//     if (local_38 < 0) {
//       iVar3 = -local_38;
//     }
//     while (iVar6 < iVar3) {
// LAB_80028ea4:
//       do {
//         do {
//           n = n << 1;
//           local_40 = local_40 >> 1;
//           local_3c = local_3c >> 1;
//           local_38 = local_38 >> 1;
//           iVar3 = local_40;
//           if (local_40 < 0) {
//             iVar3 = -local_40;
//           }
//         } while (iVar6 < iVar3);
//         iVar3 = local_3c;
//         if (local_3c < 0) {
//           iVar3 = -local_3c;
//         }
//       } while (iVar6 < iVar3);
//       iVar3 = local_38;
//       if (local_38 < 0) {
//         iVar3 = -local_38;
//       }
//     }
//     local_20.vz = (short)local_38;
//     local_20.vx = (short)local_40;
//     local_20.vy = (short)local_3c;
//     pVVar2 = GetAreaMapPassage(GlobalAreaMap,&local_30,&local_20,n);
//     if (pVVar2 != (VECTOR *)0x0) {
//       return -2;
//     }
//     if (*distance < *(int *)((int)searchsight + uVar5 * 0xc + 4)) {
//       return 1;
//     }
//     return 2;
//   }
//   return -1;
// }
