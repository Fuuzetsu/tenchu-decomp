#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawFlyWire(struct tag_EffectSlot *ef);
 *     EFFECT.C:1346, 36 src lines, frame 72 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
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
 *     param $s1       struct tag_EffectSlot * ef
 *     reg   $s0       struct FlyWireType * param
 *     stack sp+24     struct VECTOR pos
 * END PSX.SYM */


extern void *memset(void *s, int c, u32 n);

void DrawFlyWire(TEffectSlot *ef)
{
    enum { m = 5 };
    FlyWireType *param;
    VECTOR pos;

    param = &ef->param.flywire;
    switch (param->mode) {
    case 0: {
        s16 time;
        s32 sum;

        time = param->time;
        sum = (u16)param->count + 0x1000 / time;
        param->count = sum;
        if ((s16)sum >= 0x1001) {
            param->count = 0;
            param->mode = param->mode + 1;
            SetBleeds(&param->end, 0, 0x32, 0xA, 0x1E, 0xFFFF00);
            Sound(CamState.Owner, 0x31);
        } else {
            SetWire(&param->start, &param->end, &param->center, (s16)sum);
        }
        return;
    }
    case 1: {
        VECTOR tmp;
        s16 count;

        memset(&tmp, 0, sizeof(VECTOR));
        count = param->count;
        tmp.vx = ((param->center.vx * (m - count)) + (param->NCenter.vx * count)) / m;
        count = param->count;
        tmp.vy = ((param->center.vy * (m - count)) + (param->NCenter.vy * count)) / m;
        count = param->count;
        tmp.vz = ((param->center.vz * (m - count)) + (param->NCenter.vz * count)) / m;
        pos = tmp;
        SetWire(&param->start, &param->end, &pos, 0x1000);
        if (param->count >= m) {
            ef->proc = 0;
        }
        param->count = param->count + 1;
        return;
    }
    }
}

// triage: MEDIUM — 145 insns, mul/div, 4 callees, ~0.05 to EquipWeapon
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Removing unreachable block (ram,0x80036d1c) */
//
// void DrawFlyWire(tag_EffectSlot *ef)
//
// {
//   uchar uVar1;
//   int iVar2;
//   VECTOR local_30;
//   int local_20;
//   int local_1c;
//   int local_18;
//   long local_14;
//
//   uVar1 = (ef->param).flywire.mode;
//   if (uVar1 == '\0') {
//     iVar2 = (int)(ef->param).flywire.time;
//     if (iVar2 == 0) {
//       trap(0x1c00);
//     }
//     iVar2 = (uint)(ushort)(ef->param).flywire.count + 0x1000 / iVar2;
//     (ef->param).flywire.count = (short)iVar2;
//     iVar2 = iVar2 * 0x10000 >> 0x10;
//     if (iVar2 < 0x1001) {
//       SetWire((VECTOR *)&(ef->param).blood,(VECTOR *)&(ef->param).smoke.pos.vz,
//               &(ef->param).flywire.center,iVar2);
//     }
//     else {
//       (ef->param).flywire.count = 0;
//       (ef->param).flywire.mode = (ef->param).flywire.mode + '\x01';
//       SetBleeds((short)ef + 0x14,0,0x32);
//       Sound(CamState.Owner,0x31);
//     }
//   }
//   else if (uVar1 == '\x01') {
//     memset((uchar *)&local_20,'\0',0x10);
//     iVar2 = (int)(ef->param).flywire.count;
//     local_30.vx = ((ef->param).flywire.center.vx * (5 - iVar2) +
//                   (ef->param).flywire.NCenter.vx * iVar2) / 5;
//     iVar2 = (int)(ef->param).flywire.count;
//     local_30.vy = ((ef->param).flywire.center.vy * (5 - iVar2) +
//                   (ef->param).flywire.NCenter.vy * iVar2) / 5;
//     iVar2 = (int)(ef->param).flywire.count;
//     local_30.vz = ((ef->param).flywire.center.vz * (5 - iVar2) +
//                   (ef->param).flywire.NCenter.vz * iVar2) / 5;
//     local_30.pad = local_14;
//     local_20 = local_30.vx;
//     local_1c = local_30.vy;
//     local_18 = local_30.vz;
//     SetWire((VECTOR *)&(ef->param).blood,(VECTOR *)&(ef->param).smoke.pos.vz,&local_30,0x1000);
//     if (4 < (ef->param).flywire.count) {
//       ef->proc = (undefined **)0x0;
//     }
//     (ef->param).flywire.count = (ef->param).flywire.count + 1;
//   }
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? SetBleeds(void *, ?, ?, ?, s32, s32);             /* extern */
// ? SetWire(void *, void *, s32 *, s16);              /* extern */
// ? Sound(s32, ?);                                    /* extern */
// ? memset(s32 *, ?, ?);                              /* extern */
// extern TCameraStatus CamState;
//
// void DrawFlyWire(s32 *arg0) {
//     s32 sp18;
//     s32 sp1C;
//     s32 sp20;
//     s32 sp24;
//     s32 sp28;
//     s32 sp2C;
//     s32 sp30;
//     s16 temp_a0;
//     s16 temp_a0_2;
//     s16 temp_v0;
//     s16 temp_v1_2;
//     u8 temp_v1;
//     void *temp_s0;
//
//     temp_s0 = arg0 + 4;
//     temp_v1 = temp_s0->unk44;
//     switch (temp_v1) {                              /* irregular */
//     case 0:
//         temp_v0 = (u16) temp_s0->unk40 + (0x1000 / (s16) temp_s0->unk42);
//         temp_s0->unk40 = temp_v0;
//         if (temp_v0 >= 0x1001) {
//             temp_s0->unk40 = 0;
//             temp_s0->unk44 = (u8) (temp_s0->unk44 + 1);
//             SetBleeds(arg0 + 0x14, 0, 0x32, 0xA, 0x1E, 0xFFFF00);
//             Sound(CamState.Owner, 0x31);
//             return;
//         }
//         SetWire(temp_s0, arg0 + 0x14, arg0 + 0x24, temp_v0);
//         return;
//     case 1:
//         memset(&sp28, 0, 0x10);
//         temp_a0 = temp_s0->unk40;
//         sp28 = ((temp_s0->unk20 * (5 - temp_a0)) + (temp_s0->unk30 * temp_a0)) / 5;
//         temp_a0_2 = temp_s0->unk40;
//         sp2C = ((temp_s0->unk24 * (5 - temp_a0_2)) + (temp_s0->unk34 * temp_a0_2)) / 5;
//         temp_v1_2 = temp_s0->unk40;
//         sp30 = ((temp_s0->unk28 * (5 - temp_v1_2)) + (temp_s0->unk38 * temp_v1_2)) / 5;
//         sp18 = sp28;
//         sp1C = sp2C;
//         sp20 = sp30;
//         sp24 = sp34;
//         SetWire(temp_s0, arg0 + 0x14, &sp18, 0x1000);
//         if (temp_s0->unk40 >= 5) {
//             *arg0 = 0;
//         }
//         temp_s0->unk40 = (s16) ((u16) temp_s0->unk40 + 1);
//         return;
//     }
// }
