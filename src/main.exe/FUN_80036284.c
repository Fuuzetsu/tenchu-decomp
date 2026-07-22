#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct GsOT *OTablePt;
 *
 * PSX.SYM suggests this may be `SetGore` (LOW confidence, EFFECT.C) — NOT
 * adopted. Corroborate with `tools/callmatch.py --verify` before renaming.
 * END PSX.SYM */

extern GsOT *OTablePt;
extern void SetPolyXF4(POLY_XF4 *ply, short attrib);
extern void AddXF4(void *ot, POLY_XF4 *ply);

/*
 * STATUS: MATCH (exact).
 *
 * The three interpolated channels are ordinary byte-sized temporaries. Their
 * natural QImode pseudos give the target's caller-register conflicts; the old
 * mixed u32/u16/u32 draft created a false a0/a2 coloring problem.
 *
 * Case 2 writes the inverse time expression at each channel:
 * `(duration - elapsed) * color / duration`. GCC's CSE shares the repeated
 * subtraction into the target's separate v1 pseudo. Assigning the subtraction
 * back to `elapsed`, or naming a conventional `remaining` local, instead
 * ties it to a1 and leaves a 14-byte register-only residual.
 *
 * This human-scale source matches all 728 bytes without fences or donor copies.
 */

void FUN_80036284(TEffectSlot *ef)
{
    XF4Type *param;
    POLY_XF4 local;
    POLY_XF4 *ply;
    long elapsed;
    u32 duration;
    u8 r;
    u8 g;
    u8 b;
    u8 mode;

    param = &ef->param.xf4;
    SetPolyXF4(&local, 1);
    local.ply.x0 = -0xA0;
    local.ply.y0 = -0x78;
    local.ply.x1 = 0xA0;
    local.ply.y1 = -0x78;
    local.ply.x2 = -0xA0;
    local.ply.y2 = 0x78;
    local.ply.x3 = 0xA0;
    local.ply.y3 = 0x78;

    mode = param->unk10;
    elapsed = GameClock - param->py;
    duration = param->pz - param->py;
    switch (mode)
    {
    case 0:
        r = (elapsed * param->unk0) / duration;
        g = (elapsed * param->unk1) / duration;
        b = (elapsed * param->unk2) / duration;
        local.ply.r0 = r;
        local.ply.g0 = g;
        local.ply.b0 = b;
        if ((u32)GameClock >= (u32)param->pz)
        {
            param->unk10 = param->unk10 + 1;
            param->pz = param->pz + 3;
        }
        break;
    case 1:
        local.ply.r0 = param->unk0;
        local.ply.g0 = param->unk1;
        local.ply.b0 = param->unk2;
        if ((u32)GameClock >= (u32)param->pz)
        {
            param->unk10 = param->unk10 + 1;
            param->pz = param->pz + 0x28;
        }
        break;
    case 2:
        if ((u32)GameClock >= (u32)param->pz)
        {
            ef->proc = 0;
            return;
        }
        r = ((duration - elapsed) * param->unk0) / duration;
        g = ((duration - elapsed) * param->unk1) / duration;
        b = ((duration - elapsed) * param->unk2) / duration;
        local.ply.r0 = r;
        local.ply.g0 = g;
        local.ply.b0 = b;
        break;
    }

    ply = (POLY_XF4 *)GsGetWorkBase();
    GsSetWorkBase(ply + 1);
    *ply = local;
    AddXF4(OTablePt->org + param->px, ply);
}

// triage: MEDIUM — 182 insns, mul/div, 4 callees, ~0.07 to FUN_80038c0c
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_80036284(undefined4 *param_1)
//
// {
//   byte bVar1;
//   char cVar2;
//   GsOT *pGVar3;
//   POLY_XF4 *ply;
//   int iVar4;
//   uint uVar5;
//   POLY_XF4 local_30;
//
//   SetPolyXF4(&local_30,1);
//   local_30.ply.x0 = -0xa0;
//   local_30.ply.y0 = -0x78;
//   local_30.ply.x1 = 0xa0;
//   local_30.ply.y1 = -0x78;
//   local_30.ply.x2 = -0xa0;
//   local_30.ply.y2 = 0x78;
//   local_30.ply.x3 = 0xa0;
//   local_30.ply.y3 = 0x78;
//   bVar1 = *(byte *)(param_1 + 5);
//   iVar4 = GameClock - param_1[3];
//   uVar5 = param_1[4] - param_1[3];
//   if (bVar1 == 1) {
//     local_30.ply._4_3_ = *(undefined3 *)(param_1 + 1);
//     if ((uint)GameClock < (uint)param_1[4]) goto LAB_800364d4;
//     cVar2 = *(char *)(param_1 + 5);
//     iVar4 = param_1[4] + 0x28;
//   }
//   else {
//     if (1 < bVar1) {
//       if (bVar1 == 2) {
//         iVar4 = uVar5 - iVar4;
//         if ((uint)param_1[4] <= (uint)GameClock) {
//           *param_1 = 0;
//           return;
//         }
//         if (uVar5 == 0) {
//           trap(0x1c00);
//         }
//         if (uVar5 == 0) {
//           trap(0x1c00);
//         }
//         if (uVar5 == 0) {
//           trap(0x1c00);
//         }
//         local_30.ply.g0 = (u_char)((iVar4 * (uint)*(byte *)((int)param_1 + 5)) / uVar5);
//         local_30.ply.r0 = (u_char)((iVar4 * (uint)*(byte *)(param_1 + 1)) / uVar5);
//         local_30.ply.b0 = (u_char)((iVar4 * (uint)*(byte *)((int)param_1 + 6)) / uVar5);
//       }
//       goto LAB_800364d4;
//     }
//     if (bVar1 != 0) goto LAB_800364d4;
//     if (uVar5 == 0) {
//       trap(0x1c00);
//     }
//     if (uVar5 == 0) {
//       trap(0x1c00);
//     }
//     if (uVar5 == 0) {
//       trap(0x1c00);
//     }
//     local_30.ply.g0 = (u_char)((iVar4 * (uint)*(byte *)((int)param_1 + 5)) / uVar5);
//     local_30.ply.r0 = (u_char)((iVar4 * (uint)*(byte *)(param_1 + 1)) / uVar5);
//     local_30.ply.b0 = (u_char)((iVar4 * (uint)*(byte *)((int)param_1 + 6)) / uVar5);
//     if ((uint)GameClock < (uint)param_1[4]) goto LAB_800364d4;
//     cVar2 = *(char *)(param_1 + 5);
//     iVar4 = param_1[4] + 3;
//   }
//   *(char *)(param_1 + 5) = cVar2 + '\x01';
//   param_1[4] = iVar4;
// LAB_800364d4:
//   ply = (POLY_XF4 *)GsGetWorkBase();
//   GsSetWorkBase(ply + 1);
//   pGVar3 = OTablePt;
//   (ply->tpage).tag = local_30.tpage.tag;
//   (ply->tpage).code[0] = local_30.tpage.code[0];
//   (ply->ply).tag = local_30.ply.tag;
//   (ply->ply).r0 = local_30.ply.r0;
//   (ply->ply).g0 = local_30.ply.g0;
//   (ply->ply).b0 = local_30.ply.b0;
//   (ply->ply).code = local_30.ply.code;
//   (ply->ply).x0 = local_30.ply.x0;
//   (ply->ply).y0 = local_30.ply.y0;
//   (ply->ply).x1 = local_30.ply.x1;
//   (ply->ply).y1 = local_30.ply.y1;
//   (ply->ply).x2 = local_30.ply.x2;
//   (ply->ply).y2 = local_30.ply.y2;
//   (ply->ply).x3 = local_30.ply.x3;
//   (ply->ply).y3 = local_30.ply.y3;
//   AddXF4((undefined *)(pGVar3->org + param_1[2]),ply);
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? AddXF4(s32, void *);                              /* extern */
// void *GsGetWorkBase(u32, u32, u32, u32);            /* extern */
// ? GsSetWorkBase(s32);                               /* extern */
// ? SetPolyXF4(s32 *, ?);                             /* extern */
// extern u32 GameClock;
// extern void *OTablePt;
//
// void FUN_80036284(void *arg0) {
//     s32 sp10;
//     u8 sp1C;
//     u8 sp1D;
//     u8 sp1E;
//     s16 sp20;
//     s16 sp22;
//     s16 sp24;
//     s16 sp26;
//     s16 sp28;
//     s16 sp2A;
//     s16 sp2C;
//     s16 sp2E;
//     s32 temp_v0;
//     s32 temp_v1_2;
//     u32 temp_a2;
//     u32 var_a0;
//     u32 var_a1;
//     u32 var_a3;
//     u32 var_v1;
//     u8 temp_v1;
//     u8 var_v0;
//     void *temp_s1;
//     void *temp_v0_2;
//
//     temp_s1 = arg0 + 4;
//     SetPolyXF4(&sp10, 1);
//     var_a3 = GameClock;
//     sp22 = -0x78;
//     sp26 = -0x78;
//     sp20 = -0xA0;
//     sp24 = 0xA0;
//     sp28 = -0xA0;
//     sp2A = 0x78;
//     sp2C = 0xA0;
//     sp2E = 0x78;
//     temp_v0 = temp_s1->unk8;
//     var_a0 = temp_s1->unkC;
//     temp_v1 = temp_s1->unk10;
//     var_a1 = var_a3 - temp_v0;
//     temp_a2 = var_a0 - temp_v0;
//     switch (temp_v1) {                              /* irregular */
//     case 0:
//         var_a0 = (u32) (var_a1 * arg0->unk4) / temp_a2;
//         sp1C = (u8) var_a0;
//         sp1D = (u8) ((u32) (var_a1 * temp_s1->unk1) / temp_a2);
//         sp1E = (u8) ((u32) (var_a1 * temp_s1->unk2) / temp_a2);
//         if (var_a3 >= (u32) temp_s1->unkC) {
//             var_v0 = temp_s1->unk10 + 1;
//             var_v1 = temp_s1->unkC + 3;
// block_10:
//             temp_s1->unk10 = var_v0;
//             temp_s1->unkC = var_v1;
//         }
//     default:
// block_14:
//         temp_v0_2 = GsGetWorkBase(var_a0, var_a1, temp_a2, var_a3);
//         GsSetWorkBase(temp_v0_2 + 0x20);
//         temp_v0_2->unk0 = sp10;
//         temp_v0_2->unk4 = sp14;
//         temp_v0_2->unk8 = sp18;
//         temp_v0_2->unkC = (s32) sp1C;
//         temp_v0_2->unk10 = (s32) sp20;
//         temp_v0_2->unk14 = (s32) sp24;
//         temp_v0_2->unk18 = (s32) sp28;
//         temp_v0_2->unk1C = (s32) sp2C;
//         AddXF4(OTablePt->unk4 + (temp_s1->unk4 * 4), temp_v0_2);
//         return;
//     case 1:
//         sp1C = arg0->unk4;
//         sp1D = temp_s1->unk1;
//         sp1E = temp_s1->unk2;
//         if (var_a3 >= (u32) temp_s1->unkC) {
//             var_v0 = temp_s1->unk10 + 1;
//             var_v1 = temp_s1->unkC + 0x28;
//             goto block_10;
//         }
//         goto block_14;
//     case 2:
//         temp_v1_2 = temp_a2 - var_a1;
//         if (var_a3 >= var_a0) {
//             arg0->unk0 = 0;
//             return;
//         }
//         var_a1 = (u32) (temp_v1_2 * arg0->unk4) / temp_a2;
//         var_a3 = temp_v1_2 * temp_s1->unk1;
//         var_a0 = var_a3 / temp_a2;
//         sp1C = (u8) var_a1;
//         sp1D = (u8) var_a0;
//         sp1E = (u8) ((u32) (temp_v1_2 * temp_s1->unk2) / temp_a2);
//         goto block_14;
//     }
// }
