#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void MoveKorogari(struct tag_TItem *item, struct param_korogari *param);
 *     ITEM.C:663, 63 src lines, frame 64 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     param $s1       struct tag_TItem * item
 *     param $s2       struct param_korogari * param
 *     stack sp+24     struct MapVector mv
 *     stack sp+40     struct SVECTOR vec
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern short RefrectMove[16][2];
 * END PSX.SYM */

/*
 * MoveKorogari (0x8003da08, 1,484 bytes) — advances a rolling item, probes
 * terrain, reflects or damps its velocity, and handles water and bounce
 * state changes.
 *
 * Matching notes:
 *  - GetAreaMapVector writes retail's complete 0x18-byte MapVector here.
 *  - D_80097AD0 is declared as an array even though only element zero is
 *    copied.  That preserves the target's two-register absolute address.
 *  - The final bounce deliberately spells the sum as A + (B + 25).  GCC
 *    2.8.1's fold pass reassociates that tree to (A + 25) + B, placing the
 *    target addiu on the abs()/2 accumulator before expanding rand() % 25.
 */

extern s16 RefrectMove[16][2];
extern SVECTOR D_80097AD0[];

extern s32 CGetLevel(struct AreaNodeType **hint, s32 x, s32 y, s32 z, u32 flag);
extern s32 GetAreaMapVector(u32 *area, MapVector *map, VECTOR *position,
                            s32 width, s32 mode);
extern void SetSplash(VECTOR *position, s16 sx, s16 sy, s32 speed);

void MoveKorogari(TItem *item, param_korogari *param)
{
    MapVector mv;
    SVECTOR vec;
    s32 level;

    if (param->status == KORO_STAY)
    {
        return;
    }

    item->locate->locate.coord.t[0] += param->vx;
    item->locate->locate.coord.t[1] += param->vy;
    item->locate->locate.coord.t[2] += param->vz;

    level = CGetLevel((struct AreaNodeType **)&param->hint,
                      item->locate->locate.coord.t[0],
                      item->locate->locate.coord.t[1],
                      item->locate->locate.coord.t[2], 0);
    if (level < item->locate->locate.coord.t[1])
    {
        item->locate->locate.coord.t[0] -= param->vx;
        item->locate->locate.coord.t[1] -= param->vy;
        item->locate->locate.coord.t[2] -= param->vz;

        GetAreaMapVector(GlobalAreaMap, &mv,
                         (VECTOR *)item->locate->locate.coord.t, 500, 0);
        if (param->hint == 0)
        {
            level = CGetLevel((struct AreaNodeType **)&param->hint,
                              item->locate->locate.coord.t[0],
                              item->locate->locate.coord.t[1],
                              item->locate->locate.coord.t[2], 0);
            if (level == (s32)0x80000000)
            {
                if (param->status == 5)
                {
                    param->vx = rand() % 1600 - 800;
                    param->vy = rand() % 1600 - 800;
                    param->vz = rand() % 1600 - 800;
                }
                else
                {
                    param->vx = 0;
                    param->vy = 250;
                    param->vz = 0;
                }
                param->status = 5;
                return;
            }
        }

        if (mv.vector != 0 && mv.height > 500)
        {
            param->vx = RefrectMove[mv.vector][0] * (abs(param->vx) / 4);
            param->vy /= 2;
            param->vz = RefrectMove[mv.vector][1] * (abs(param->vz) / 4);
            param->status = KORO_WALL;
            return;
        }

        if (mv.attrib & 4)
        {
            param->vx = rand() % 20 - 10;
            param->vz = rand() % 20 - 10;
            if (param->vy > 20)
            {
                vec = D_80097AD0[0];
                SetSplash((VECTOR *)item->locate->locate.coord.t,
                          0x2000, 0x2000, 4);
                param->status = KORO_WATER;
            }
            param->vy = -param->vy / 8;
            return;
        }
        else
        {
            param->vx /= 2;
            param->vz /= 2;
            if (mv.level == (s32)0x80000000 || mv.height > 1500)
            {
                goto bounce;
            }

            item->locate->locate.coord.t[1] = mv.level;
            if (param->vy < 46)
            {
                param->status = KORO_STAY;
                return;
            }

            param->vx += RefrectMove[mv.vector][0] * (rand() % 25 + 25);
            param->vz += RefrectMove[mv.vector][1] * (rand() % 25 + 25);
            param->status = KORO_GRAND;
            param->vy = -abs(param->vy) / 2;
            return;
        }
bounce:
        param->vy = abs(param->vy) / 2 + (rand() % 25 + 25);
        param->status = KORO_WALL;
        return;
    }
    param->status = KORO_NORMAL;
    param->vy += 15;
}

// Historical Ghidra decompilation reference:
//
//
// void MoveKorogari(tag_TItem *item,param_korogari *param)
//
// {
//   short sVar1;
//   ModelType *pMVar2;
//   long lVar3;
//   int iVar4;
//   int iVar5;
//   VECTOR local_30;
//   undefined4 local_18;
//   undefined4 local_14;
//
//   if (param->status == '\x04') {
//     return;
//   }
//   (item->locate->locate).coord.t[0] = (item->locate->locate).coord.t[0] + (int)param->vx;
//   (item->locate->locate).coord.t[1] = (item->locate->locate).coord.t[1] + (int)param->vy;
//   (item->locate->locate).coord.t[2] = (item->locate->locate).coord.t[2] + (int)param->vz;
//   pMVar2 = item->locate;
//   lVar3 = CGetLevel(&param->hint,(pMVar2->locate).coord.t[0],(pMVar2->locate).coord.t[1],
//                     (pMVar2->locate).coord.t[2]);
//   pMVar2 = item->locate;
//   if (lVar3 < (pMVar2->locate).coord.t[1]) {
//     (pMVar2->locate).coord.t[0] = (pMVar2->locate).coord.t[0] - (int)param->vx;
//     (item->locate->locate).coord.t[1] = (item->locate->locate).coord.t[1] - (int)param->vy;
//     (item->locate->locate).coord.t[2] = (item->locate->locate).coord.t[2] - (int)param->vz;
//     GetAreaMapVector((MapVector *)GlobalAreaMap,&local_30,(long)(item->locate->locate).coord.t);
//     if (param->hint == (AreaNodeType *)0x0) {
//       pMVar2 = item->locate;
//       lVar3 = CGetLevel(&param->hint,(pMVar2->locate).coord.t[0],(pMVar2->locate).coord.t[1],
//                         (pMVar2->locate).coord.t[2]);
//       if (lVar3 == -0x80000000) {
//         if (param->status == '\x05') {
//           iVar4 = rand();
//           param->vx = (short)iVar4 + (short)(iVar4 / 0x640) * -0x640 + -800;
//           iVar4 = rand();
//           param->vy = (short)iVar4 + (short)(iVar4 / 0x640) * -0x640 + -800;
//           iVar4 = rand();
//           param->vz = (short)iVar4 + (short)(iVar4 / 0x640) * -0x640 + -800;
//         }
//         else {
//           param->vx = 0;
//           param->vy = 0xfa;
//           param->vz = 0;
//         }
//         param->status = '\x05';
//         return;
//       }
//     }
//     if (((byte)local_30.pad != 0) && (500 < local_30.vy)) {
//       iVar4 = abs((int)param->vx);
//       sVar1 = RefrectMove[0][(uint)(byte)local_30.pad * 2];
//       if (iVar4 < 0) {
//         iVar4 = iVar4 + 3;
//       }
//       iVar5 = (uint)(ushort)param->vy << 0x10;
//       param->vy = (short)((iVar5 >> 0x10) - (iVar5 >> 0x1f) >> 1);
//       param->vx = sVar1 * (short)(iVar4 >> 2);
//       iVar4 = abs((int)param->vz);
//       sVar1 = RefrectMove[0][(uint)(byte)local_30.pad * 2 + 1];
//       if (iVar4 < 0) {
//         iVar4 = iVar4 + 3;
//       }
//       param->status = '\x03';
//       param->vz = sVar1 * (short)(iVar4 >> 2);
//       return;
//     }
//     if (((ushort)local_30.vz & 4) == 0) {
//       iVar4 = (uint)(ushort)param->vx << 0x10;
//       iVar5 = (uint)(ushort)param->vz << 0x10;
//       param->vx = (short)((iVar4 >> 0x10) - (iVar4 >> 0x1f) >> 1);
//       param->vz = (short)((iVar5 >> 0x10) - (iVar5 >> 0x1f) >> 1);
//       if ((local_30.vx == -0x80000000) || (0x5dc < local_30.vy)) {
//         iVar4 = abs((int)param->vy);
//         iVar5 = rand();
//         param->status = '\x03';
//         param->vy = (short)(iVar4 / 2) + 0x19 + (short)iVar5 + (short)(iVar5 / 0x19) * -0x19;
//         return;
//       }
//       (item->locate->locate).coord.t[1] = local_30.vx;
//       if (param->vy < 0x2e) {
//         param->status = '\x04';
//         return;
//       }
//       iVar4 = rand();
//       param->vx = param->vx +
//                   RefrectMove[0][(uint)(byte)local_30.pad * 2] *
//                   ((short)iVar4 + (short)(iVar4 / 0x19) * -0x19 + 0x19);
//       iVar4 = rand();
//       sVar1 = RefrectMove[0][(uint)(byte)local_30.pad * 2 + 1];
//       param->status = '\x02';
//       param->vz = param->vz + sVar1 * ((short)iVar4 + (short)(iVar4 / 0x19) * -0x19 + 0x19);
//       iVar4 = abs((int)param->vy);
//       sVar1 = (short)(-iVar4 / 2);
//     }
//     else {
//       iVar4 = rand();
//       param->vx = (short)iVar4 + (short)(iVar4 / 0x14) * -0x14 + -10;
//       iVar4 = rand();
//       param->vz = (short)iVar4 + (short)(iVar4 / 0x14) * -0x14 + -10;
//       if (0x14 < param->vy) {
//         local_18 = DAT_80097ad0;
//         local_14 = DAT_80097ad4;
//         SetSplash((VECTOR *)(item->locate->locate).coord.t,0x2000,0x2000,4);
//         param->status = '\x01';
//       }
//       iVar4 = -(int)param->vy;
//       if (iVar4 < 0) {
//         iVar4 = iVar4 + 7;
//       }
//       sVar1 = (short)(iVar4 >> 3);
//     }
//   }
//   else {
//     param->status = '\0';
//     sVar1 = param->vy + 0xf;
//   }
//   param->vy = sVar1;
//   return;
// }
