#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"

/*
 * MATCH.
 *
 * Creates the persistent ninken character, then snapshots the selected
 * character's model and a temporary table-selected character model for the
 * disguise logic consumed by ProcItemHenshin.
 *
 * Matching notes:
 *  - Each output buffer has the saved `waist` value followed by ordinary
 *    12-byte model-part snapshots (`tmd`, `x`, `y`, and `z`).
 *  - The two model-copy phases need separate block-scoped model, saved, and
 *    index locals. Reusing one set across both phases joins their pseudos,
 *    rotates the caller-saved registers, and fills three target load-delay
 *    nops; distinct source identities reproduce the exact allocation.
 *  - The selected character model is read through the recovered shared
 *    `CamState.Owner` field.
 */
extern Humanoid *NINKEN_CHARACTER_PTR;

void create_ninken_character_(s16 type, s32 stage)
{
    NINKEN_CHARACTER_PTR = BreedLife(0xa9, 999000, 999000, 999000, 0);
    NINKEN_CHARACTER_PTR->attribute |= 0x80;

    {
        ModelArchiveType *model;
        HenshinModelSnapshot *saved;
        s32 i;

        model = CamState.Owner->model;
        i = 0;
        saved = &Item_save;
        saved->waist = model->rotate.pad;
        if (model->n > 0)
        {
            do
            {
                saved->p[i].tmd = model->object[i]->object.tmd;
                saved->p[i].x =
                    model->object[i]->locate.coord.t[0];
                saved->p[i].y =
                    model->object[i]->locate.coord.t[1];
                saved->p[i].z =
                    model->object[i]->locate.coord.t[2];
                i++;
            } while (i < model->n);
        }
    }

    {
        Humanoid *human;
        ModelArchiveType *model;
        HenshinModelSnapshot *saved;
        s32 i;
        s32 flag;

        flag = (type == 1);
        human = BreedLife(HensinT[(s16)stage].type[flag],
                          999000, 999000, 999000, 0);
        model = human->model;
        i = 0;
        saved = &D_800C06F0;
        saved->waist = model->rotate.pad;
        if (model->n > 0)
        {
            do
            {
                saved->p[i].tmd = model->object[i]->object.tmd;
                saved->p[i].x =
                    model->object[i]->locate.coord.t[0];
                saved->p[i].y =
                    model->object[i]->locate.coord.t[1];
                saved->p[i].z =
                    model->object[i]->locate.coord.t[2];
                i++;
            } while (i < model->n);
        }
        KillHumanoid(human);
    }
}

// triage: MEDIUM — 144 insns, 2 loop, 2 callees, ~0.09 to FUN_8004a598
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
//
// void FUN_8003d528(short param_1,int param_2)
//
// {
//   Humanoid *human;
//   undefined2 *puVar1;
//   ModelType **ppMVar2;
//   ModelArchiveType *pMVar3;
//   int iVar4;
//
//   DAT_80097f58 = BreedLife(0xa9,0xf3e58,0xf3e58,0xf3e58,0);
//   *(ushort *)(DAT_80097f58 + 4) = *(ushort *)(DAT_80097f58 + 4) | 0x80;
//   pMVar3 = (CamState.Owner)->model;
//   iVar4 = 0;
//   _DAT_800c0630 = (int)(pMVar3->rotate).pad;
//   puVar1 = &DAT_800c0630;
//   if (0 < pMVar3->n) {
//     do {
//       *(ulong **)(puVar1 + 2) = (pMVar3->object[iVar4]->object).tmd;
//       puVar1[4] = (short)(pMVar3->object[iVar4]->locate).coord.t[0];
//       puVar1[5] = (short)(pMVar3->object[iVar4]->locate).coord.t[1];
//       ppMVar2 = pMVar3->object + iVar4;
//       iVar4 = iVar4 + 1;
//       puVar1[6] = (short)((*ppMVar2)->locate).coord.t[2];
//       puVar1 = puVar1 + 6;
//     } while (iVar4 < pMVar3->n);
//   }
//   human = (Humanoid *)
//           BreedLife((&DAT_8008e3ec)[(uint)(param_1 == 1) + ((param_2 << 0x10) >> 0xf)],0xf3e58,
//                     0xf3e58,0xf3e58,0);
//   pMVar3 = human->model;
//   iVar4 = 0;
//   _DAT_800c06f0 = (int)(pMVar3->rotate).pad;
//   puVar1 = &DAT_800c06f0;
//   if (0 < pMVar3->n) {
//     do {
//       *(ulong **)(puVar1 + 2) = (pMVar3->object[iVar4]->object).tmd;
//       puVar1[4] = (short)(pMVar3->object[iVar4]->locate).coord.t[0];
//       puVar1[5] = (short)(pMVar3->object[iVar4]->locate).coord.t[1];
//       ppMVar2 = pMVar3->object + iVar4;
//       iVar4 = iVar4 + 1;
//       puVar1[6] = (short)((*ppMVar2)->locate).coord.t[2];
//       puVar1 = puVar1 + 6;
//     } while (iVar4 < pMVar3->n);
//   }
//   KillHumanoid(human);
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// void *BreedLife(u8, ?, ?, ?, s32);                  /* extern */
// ? KillHumanoid(void *);                             /* extern */
// extern TCameraStatus CamState;
// extern ? D_8008E3EC;
// extern s32 D_800C0630;
// extern s32 D_800C06F0;
// extern void *NINKEN_CHARACTER_PTR;
//
// void create_ninken_character_(s16 arg0, s32 arg1) {
//     s32 *var_a0;
//     s32 *var_a1;
//     s32 temp_v1;
//     s32 temp_v1_2;
//     s32 var_a2;
//     s32 var_a3;
//     void *temp_a1;
//     void *temp_a2;
//     void *temp_v0;
//     void *temp_v0_2;
//
//     temp_v0 = BreedLife(0xA9U, 0xF3E58, 0xF3E58, 0xF3E58, 0);
//     temp_v0->unk4 = (u16) (temp_v0->unk4 | 0x80);
//     temp_a1 = CamState.Owner->unk58;
//     var_a2 = 0;
//     NINKEN_CHARACTER_PTR = temp_v0;
//     D_800C0630 = (s32) temp_a1->unk56;
//     if (temp_a1->unk64 > 0) {
//         var_a0 = &D_800C0630;
//         do {
//             temp_v1 = var_a2 * 4;
//             var_a0->unk4 = (s32) (*(temp_v1 + temp_a1->unk68))->unk6C;
//             var_a0->unk8 = (u16) (*(temp_v1 + temp_a1->unk68))->unk18;
//             var_a0->unkA = (u16) (*(temp_v1 + temp_a1->unk68))->unk1C;
//             var_a2 += 1;
//             var_a0->unkC = (u16) (*(temp_v1 + temp_a1->unk68))->unk20;
//             var_a0 += 0xC;
//         } while (var_a2 < temp_a1->unk64);
//     }
//     temp_v0_2 = BreedLife(*((arg0 == 1) + ((s32) (arg1 << 0x10) >> 0xF) + &D_8008E3EC), 0xF3E58, 0xF3E58, 0xF3E58, 0);
//     temp_a2 = temp_v0_2->unk58;
//     var_a3 = 0;
//     D_800C06F0 = (s32) temp_a2->unk56;
//     if (temp_a2->unk64 > 0) {
//         var_a1 = &D_800C06F0;
//         do {
//             temp_v1_2 = var_a3 * 4;
//             var_a1->unk4 = (s32) (*(temp_v1_2 + temp_a2->unk68))->unk6C;
//             var_a1->unk8 = (u16) (*(temp_v1_2 + temp_a2->unk68))->unk18;
//             var_a1->unkA = (u16) (*(temp_v1_2 + temp_a2->unk68))->unk1C;
//             var_a3 += 1;
//             var_a1->unkC = (u16) (*(temp_v1_2 + temp_a2->unk68))->unk20;
//             var_a1 += 0xC;
//         } while (var_a3 < temp_a2->unk64);
//     }
//     KillHumanoid(temp_v0_2);
// }
