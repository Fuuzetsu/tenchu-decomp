#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8005d1fc", FUN_8005d1fc);

// triage: VERY-HARD — 86 insns, 3 GTE CMD — UN-SPLITTABLE (as can't assemble), 1 loop, 0 callees, ~0.04 to character_balma_around_main_routine_
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Toolchain gotchas: GTE command opcodes — `as` rejects them, so the per-function split does not even assemble; needs the GTE->.word pass first

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_8005d1fc(void)
//
// {
//   int in_v0;
//   uint uVar1;
//   int iVar2;
//   SVECTOR *r0;
//   SVECTOR *r1;
//   u_long uVar3;
//   SVECTOR *r2;
//   u_long uVar4;
//   u_long *in_t0;
//   int in_t2;
//   u_long *in_t3;
//   int in_t4;
//   int in_t5;
//   int iVar5;
//   int iVar6;
//   int unaff_s0;
//   int in_t9;
//
//   iVar6 = 1;
//   if (in_v0 != 0x607) {
//     iVar6 = in_v0;
//   }
//   do {
//     iVar5 = in_t5;
//     r0 = (SVECTOR *)((uint)*(ushort *)(iVar5 + 0x14) * 8 + in_t4);
//     r1 = (SVECTOR *)((uint)*(ushort *)(iVar5 + 0x16) * 8 + in_t4);
//     r2 = (SVECTOR *)((uint)*(ushort *)(iVar5 + 0x18) * 8 + in_t4);
//     gte_ldv3(r0,r1,r2);
//     gte_rtpt_b();
//     if (unaff_s0 != 0) {
//       *in_t0 = (u_long)in_t3;
//       *(undefined1 *)((int)in_t0 + 3) = 0;
//       gte_stRGB2();
//       *(char *)((int)in_t3 + 7) = (char)unaff_s0;
//       r0 = *(SVECTOR **)(iVar5 + -0x18);
//       r1 = *(SVECTOR **)(iVar5 + -0x14);
//       r2 = *(SVECTOR **)(iVar5 + -0x10);
//       in_t3[3] = (u_long)r0;
//       in_t3[5] = (u_long)r1;
//       in_t3[7] = (u_long)r2;
//       in_t3 = in_t3 + 8;
//     }
//     uVar1 = gte_stFLAG();
//     unaff_s0 = 0;
//     if ((uVar1 & 0x1c66000) == 0) {
//       gte_nclip_b();
//       read_sz_fifo3(r0,r1,r2);
//       uVar1 = (int)&r2->vx + (int)&r1->vx + (int)&r0->vx;
//       iVar2 = gte_stMAC0();
//       in_t0 = (u_long *)((uVar1 >> 4) + (uVar1 >> 2) >> 4);
//       if (((0 < iVar2) && (iVar2 = (int)in_t0 - in_t9, in_t0 != (u_long *)0x0)) &&
//          (in_t0 = (u_long *)((int)in_t0 * 4), iVar2 < 0)) {
//         gte_ldRGB(iVar5 + 0x10);
//         in_t0 = (u_long *)((int)in_t0 + in_t2);
//         gte_dpcs_b();
//         *in_t3 = *in_t0;
//         *(undefined1 *)((int)in_t3 + 3) = 7;
//         gte_stsxy3_g3(in_t3);
//         unaff_s0 = 0x24;
//       }
//     }
//     iVar6 = iVar6 + -1;
//     in_t5 = iVar5 + 0x1c;
//   } while (0 < iVar6);
//   if (unaff_s0 != 0) {
//     *in_t0 = (u_long)in_t3;
//     *(undefined1 *)((int)in_t0 + 3) = 0;
//     gte_stRGB2();
//     *(char *)((int)in_t3 + 7) = (char)unaff_s0;
//     uVar3 = *(u_long *)(iVar5 + 8);
//     uVar4 = *(u_long *)(iVar5 + 0xc);
//     in_t3[3] = *(u_long *)(iVar5 + 4);
//     in_t3[5] = uVar3;
//     in_t3[7] = uVar4;
//   }
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// void FUN_8005d1fc(void) {
//     s32 var_t7;
//     s8 var_s0;
//     u32 temp_t0;
//     u32 temp_t0_2;
//     void *temp_t5;
//
//     var_s0 = saved_reg_s0;
//     var_t7 = 1;
//     if (M2C_ERROR(/* Read from unset register $v0 */) != 0x607) {
//         var_t7 = M2C_ERROR(/* Read from unset register $v0 */);
//     }
//     do {
//         // GTE: lwc2 $0, ($a0)
//         // GTE: lwc2 $1, 0x4($a0)
//         // GTE: lwc2 $2, ($a1)
//         // GTE: lwc2 $3, 0x4($a1)
//         // GTE: lwc2 $4, ($a2)
//         // GTE: lwc2 $5, 0x4($a2)
//         if (var_s0 != 0) {
//             *M2C_ERROR(/* Read from unset register $t0 */) = M2C_ERROR(/* Read from unset register $t3 */);
//             M2C_ERROR(/* Read from unset register $t0 */)->unk3 = 0;
//             M2C_ERROR(/* Read from unset register $t3 */)->unk4 = (s32) GTE_MFC2(22);
//             M2C_ERROR(/* Read from unset register $t3 */)->unk7 = var_s0;
//             M2C_ERROR(/* Read from unset register $t3 */)->unkC = (s32) M2C_ERROR(/* Read from unset register $t5 */)->unk-18;
//             M2C_ERROR(/* Read from unset register $t3 */)->unk14 = (s32) M2C_ERROR(/* Read from unset register $t5 */)->unk-14;
//             M2C_ERROR(/* Read from unset register $t3 */)->unk1C = (s32) M2C_ERROR(/* Read from unset register $t5 */)->unk-10;
//         }
//         var_s0 = 0;
//         if (!(GTE_CFC2(31) & 0x01C66000)) {
//             temp_t0 = GTE_MFC2(17) + GTE_MFC2(18) + GTE_MFC2(19);
//             temp_t0_2 = (u32) ((temp_t0 >> 4) + (temp_t0 >> 2)) >> 4;
//             if ((GTE_MFC2(24) > 0) && ((s32) temp_t0_2 > 0) && ((temp_t0_2 - M2C_ERROR(/* Read from unset register $t9 */)) < 0)) {
//                 // GTE: lwc2 $6, 0x10($t5)
//                 *M2C_ERROR(/* Read from unset register $t3 */) = *((temp_t0_2 * 4) + M2C_ERROR(/* Read from unset register $t2 */));
//                 M2C_ERROR(/* Read from unset register $t3 */)->unk3 = 7;
//                 M2C_ERROR(/* Read from unset register $t3 */)->unk8 = (s32) GTE_MFC2(12);
//                 M2C_ERROR(/* Read from unset register $t3 */)->unk10 = (s32) GTE_MFC2(13);
//                 M2C_ERROR(/* Read from unset register $t3 */)->unk18 = (s32) GTE_MFC2(14);
//                 var_s0 = 0x24;
//             }
//         }
//         var_t7 -= 1;
//         temp_t5 = M2C_ERROR(/* Read from unset register $t5 */) + 0x1C;
//     } while (var_t7 > 0);
//     if (var_s0 != 0) {
//         *M2C_ERROR(/* Read from unset register $t0 */) = M2C_ERROR(/* Read from unset register $t3 */);
//         M2C_ERROR(/* Read from unset register $t0 */)->unk3 = 0;
//         M2C_ERROR(/* Read from unset register $t3 */)->unk4 = (s32) GTE_MFC2(22);
//         M2C_ERROR(/* Read from unset register $t3 */)->unk7 = var_s0;
//         M2C_ERROR(/* Read from unset register $t3 */)->unkC = (s32) temp_t5->unk-18;
//         M2C_ERROR(/* Read from unset register $t3 */)->unk14 = (s32) temp_t5->unk-14;
//         M2C_ERROR(/* Read from unset register $t3 */)->unk1C = (s32) temp_t5->unk-10;
//     }
// }
