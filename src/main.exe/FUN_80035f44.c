#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 *     extern long GameClock;
 *
 * PSX.SYM suggests this may be `DrawGore` (LOW confidence, EFFECT.C) — NOT
 * adopted. Corroborate with `tools/callmatch.py --verify` before renaming.
 * END PSX.SYM */

extern void FUN_8003562c(TEffectSlot *ef);
extern void DrawImpact(TEffectSlot *ef);

/*
 * MATCH. Converts a model-space position and direction into a blood/gore
 * effect, then emits a larger impact particle every fourth frame.
 *
 * The two EffectSlot searches intentionally use distinct scoped locals.  The
 * first cursor coalesces with the BloodType pointer in $s0; keeping one cursor
 * variable live through both searches rotates nearly every scan register.
 * `rotated` is genuinely a two-VECTOR workspace: its second element holds the
 * three signed position captures at sp+0x58..0x60 while the second pool search
 * runs.  Independent scalar captures stay in registers, shorten the function,
 * and lose the target's 0x88-byte frame.  Finally, naming `final_py` immediately
 * after the px store preserves the target's early load and late py store.
 */
void FUN_80035f44(GsCOORDINATE2 *coord, SVECTOR *position, SVECTOR *vector)
{
    VECTOR world;
    MATRIX mat;
    SVECTOR local_vector;
    VECTOR rotated[2];
    long flag[2];
    u32 clock;

    GsGetLw(coord, &mat);
    GsSetLsMatrix(&mat);
    RotTrans(position, &world, flag);

    {
        int idx;
        TEffectSlot *base;
        TEffectSlot *slot;
        int count;
        TEffectSlot *ef;
        BloodType *param;

        count = 0;
        base = EffectSlot;
        idx = CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_;
        slot = base + idx;
        do
        {
            idx = idx + 1;
            slot = slot + 1;
            if (199 < idx)
            {
                slot = base;
                idx = 0;
            }
            if (slot->proc == 0)
            {
                CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = idx + 1;
                if (199 < idx + 1)
                {
                    CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = 0;
                }
                ef = slot;
                goto found;
            }
            count = count + 1;
        } while (count < 200);
        ef = &dmy;
    found:
        param = &ef->param.blood;
        param->unk22 = rand() % 2;
        param->scale = 0x2000;
        param->rotate = (rand() % 360) * 0x1000;
        param->px = world.vx;
        param->py = world.vy;
        param->pz = world.vz;
        local_vector = *vector;
        ApplyRotMatrix(&local_vector, rotated);
        param->vx = (short)rotated[0].vx;
        param->vy = (short)rotated[0].vy;
        param->vz = (short)rotated[0].vz;
        param->time = rand() % 15 + 10;
        param->hint = 0;
        *(u16 *)&param->mode = 0x80;
        param->unk23 = 0;
        clock = GameClock & 3;
        ef->proc = (void (*)())FUN_8003562c;
    }

    if (clock == 0)
    {
        int idx;
        TEffectSlot *base;
        TEffectSlot *slot;
        TEffectSlot *ef;
        int count;
        ImpactType *impact;
        long final_py;
        long start_color;
        long end_color;

        start_color = 0x808080;
        end_color = 0x808080;
        rotated[1].vx = position->vx;
        rotated[1].vy = position->vy;
        count = 0;
        rotated[1].vz = position->vz;
        base = EffectSlot;
        idx = CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_;
        slot = base + idx;
        do
        {
            idx = idx + 1;
            slot = slot + 1;
            if (199 < idx)
            {
                slot = base;
                idx = 0;
            }
            if (slot->proc == 0)
            {
                CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = idx + 1;
                if (199 < idx + 1)
                {
                    CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = 0;
                }
                ef = slot;
                goto impact_found;
            }
            count = count + 1;
        } while (count < 200);
        ef = &dmy;
    impact_found:
        ef->proc = (void (*)())DrawImpact;
        ef->param.impact.px = rotated[1].vx;
        impact = &ef->param.impact;
        impact->py = rotated[1].vy;
        final_py = rotated[1].vz;
        impact->rotate_speed = 0x50;
        impact->start_size = 0x2000;
        impact->end_size = 0x2000;
        impact->time = 3;
        impact->super = coord;
        impact->rotate = 0;
        impact->start_color.word = start_color;
        impact->end_color.word = end_color;
        impact->count = 0;
        impact->type = 2;
        impact->pz = final_py;
    }
}

// triage: MEDIUM — 208 insns, mul/div, 2 loop, 5 callees, ~0.06 to ReqItemNingyo
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_80035f44(GsCOORDINATE2 *param_1,SVECTOR *param_2,undefined4 *param_3)
//
// {
//   short sVar1;
//   short sVar2;
//   short sVar3;
//   int iVar4;
//   uint uVar5;
//   int iVar6;
//   tag_EffectSlot *ptVar7;
//   VECTOR local_78;
//   MATRIX MStack_68;
//   SVECTOR local_48;
//   VECTOR local_40 [2];
//   long alStack_20 [2];
//
//   GsGetLw(param_1,&MStack_68);
//   GsSetLsMatrix(&MStack_68);
//   RotTrans(param_2,&local_78,alStack_20);
//   iVar6 = 0;
//   ptVar7 = EffectSlot + DAT_80097a3c;
//   iVar4 = DAT_80097a3c;
//   do {
//     iVar4 = iVar4 + 1;
//     ptVar7 = ptVar7 + 1;
//     if (199 < iVar4) {
//       iVar4 = 0;
//       ptVar7 = EffectSlot;
//     }
//     if (ptVar7->proc == (undefined **)0x0) {
//       DAT_80097a3c = iVar4 + 1;
//       if (199 < iVar4 + 1) {
//         DAT_80097a3c = 0;
//       }
//       goto LAB_80036014;
//     }
//     iVar6 = iVar6 + 1;
//   } while (iVar6 < 200);
//   ptVar7 = &dmy;
// LAB_80036014:
//   iVar4 = rand();
//   *(char *)((int)&ptVar7->param + 0x22) = (char)iVar4 + (char)(iVar4 / 2) * -2;
//   (ptVar7->param).blood.scale = 0x2000;
//   iVar4 = rand();
//   (ptVar7->param).blood.rotate = (iVar4 % 0x168) * 0x1000;
//   (ptVar7->param).blood.px = local_78.vx;
//   (ptVar7->param).blood.py = local_78.vy;
//   (ptVar7->param).blood.pz = local_78.vz;
//   local_48._0_4_ = *param_3;
//   local_48._4_4_ = param_3[1];
//   ApplyRotMatrix(&local_48,local_40);
//   (ptVar7->param).blood.vx = (short)local_40[0].vx;
//   (ptVar7->param).blood.vy = (short)local_40[0].vy;
//   (ptVar7->param).blood.vz = (short)local_40[0].vz;
//   iVar4 = rand();
//   (ptVar7->param).blood.time = (short)iVar4 + (short)(iVar4 / 0xf) * -0xf + 10;
//   (ptVar7->param).blood.hint = (AreaNodeType *)0x0;
//   *(undefined2 *)((int)&ptVar7->param + 0x20) = 0x80;
//   *(undefined1 *)((int)&ptVar7->param + 0x23) = 0;
//   uVar5 = GameClock & 3;
//   ptVar7->proc = (undefined **)FUN_8003562c;
//   if (uVar5 == 0) {
//     sVar1 = param_2->vx;
//     sVar2 = param_2->vy;
//     iVar6 = 0;
//     sVar3 = param_2->vz;
//     ptVar7 = EffectSlot + DAT_80097a3c;
//     iVar4 = DAT_80097a3c;
//     do {
//       iVar4 = iVar4 + 1;
//       ptVar7 = ptVar7 + 1;
//       if (199 < iVar4) {
//         iVar4 = 0;
//         ptVar7 = EffectSlot;
//       }
//       if (ptVar7->proc == (undefined **)0x0) {
//         DAT_80097a3c = iVar4 + 1;
//         if (199 < iVar4 + 1) {
//           DAT_80097a3c = 0;
//         }
//         goto LAB_80036208;
//       }
//       iVar6 = iVar6 + 1;
//     } while (iVar6 < 200);
//     ptVar7 = &dmy;
// LAB_80036208:
//     ptVar7->proc = (undefined **)DrawImpact;
//     *(int *)&ptVar7->param = (int)sVar1;
//     *(int *)((int)&ptVar7->param + 4) = (int)sVar2;
//     (ptVar7->param).blood.vz = 0x50;
//     (ptVar7->param).blood.time = 0x2000;
//     (ptVar7->param).blood.vx = 0x2000;
//     *(undefined1 *)((int)&ptVar7->param + 0x22) = 3;
//     (ptVar7->param).blood.pz = (long)param_1;
//     (ptVar7->param).blood.vy = 0;
//     (ptVar7->param).blood.scale = 0x808080;
//     (ptVar7->param).blood.rotate = 0x808080;
//     (ptVar7->param).blood.bright = '\0';
//     (ptVar7->param).blood.mode = '\x02';
//     *(int *)((int)&ptVar7->param + 8) = (int)sVar3;
//   }
//   return;
// }
