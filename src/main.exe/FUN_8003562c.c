#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE sprBlood;
 *     extern struct GsOT *OTablePt;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct AreaNodeType *FieldArea;
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

typedef union FUN_8003562cScratch
{
    SVECTOR screen;
    struct
    {
        SVECTOR velocity;
        VECTOR position;
        union
        {
            VECTOR position;
            SVECTOR velocity;
        } temporary;
    } bleed;
} FUN_8003562cScratch;

extern GsSPRITE sprBlood[4];
extern GsSPRITE sprBlood2[4];
extern GsOT *OTablePt;
extern AreaNodeType *FieldArea;
extern long ComputeAreaLevel(AreaNodeType *node, long x, long z);
extern long GetAreaMapLevel(unsigned long *area, long x, long y, long z,
                            int mode);
extern void GetScreenPosition(long x, long y, long z, SVECTOR *screen);
extern void SoundEx(VECTOR *pos, int sound);
extern void DrawBleed(TEffectSlot *ef);

/*
 * MATCH. This is the gore/blood renderer installed by FUN_80035f44.  It is
 * closely related to DrawBlood, but always emits a small DrawBleed particle
 * and uses a 60-unit position jitter.
 *
 * The explicit scratch union is the original sp+0x18..sp+0x3f workspace:
 * the projection SVECTOR, bleed VECTOR, and temporary VECTOR/SVECTOR all
 * overlap, keeping the target's 0x60-byte frame.  sprBlood2 is an alias for
 * the second four-sprite bank; naming it separately is load-bearing because
 * the target materializes both bank bases independently.  `node_y` and
 * `level` must remain separate around ComputeAreaLevel so the flat and sloped
 * paths cross-jump through the target multiply tail.  Likewise, the named
 * bleed_x/y/z values prevent reassociation of `(position - 60) + rand()%120`,
 * and the full-width `green` local preserves the target's li 0x7f10 before a
 * byte store.
 */
void FUN_8003562c(TEffectSlot *ef)
{
    BloodType *param;
    GsSPRITE *spr;
    GsSPRITE *spr2;
    FUN_8003562cScratch scratch;
    u32 index;
    int state;

    param = &ef->param.blood;
    index = param->unk22;
    spr = &sprBlood[index];
    spr2 = &sprBlood2[index];
    state = param->unk23;

    if (state == 2)
    {
        goto state_two;
    }
    if (state < 3)
    {
        if (state == 1)
        {
            goto state_one;
        }
    }
    else if (state == 3)
    {
        u16 fade;
        s32 fade_shift;
        s32 brightness;
        s32 half_brightness;
        s32 x;
        s32 y;
        s32 z;
        s32 size;
        s32 rotate;
        s32 otz;
        s16 scale;
        s16 screen_x;
        s16 screen_y;
        s32 value;
        s32 priority;

        fade = *(u16 *)&param->mode - 5;
        *(u16 *)&param->mode = fade;
        if ((s16)fade <= 0)
        {
            *(u16 *)&param->mode = 0;
            ef->proc = 0;
        }

        spr->attribute = 0x50000000;
        x = param->px;
        y = param->py + param->vy;
        z = param->pz;
        size = param->scale;
        param->py = y;
        rotate = param->rotate;
        fade = *(u16 *)&param->mode;
        fade_shift = (u32)fade << 16;
        brightness = (s16)fade;
        GetScreenPosition(x, y, z, &scratch.screen);
        otz = scratch.screen.vz;
        if (otz < 0x25)
        {
            return;
        }
        scale = (s16)((size * 300) / otz) + 1;
        spr->scaley = scale;
        spr->scalex = scale;
        spr2->scaley = scale;
        spr2->scalex = scale;
        spr->rotate = rotate;
        spr2->rotate = rotate;
        screen_x = scratch.screen.vx;
        spr->x = screen_x;
        spr2->x = screen_x;
        screen_y = scratch.screen.vy;
        spr->y = screen_y;
        spr2->y = screen_y;
        half_brightness = brightness / 2;
        spr->r = (u8)brightness;
        spr->g = (u8)brightness;
        spr->b = (u8)brightness;
        spr2->r = (u8)half_brightness;
        spr2->g = (u8)half_brightness;
        spr2->b = (u8)half_brightness;

        value = (s32)((u16)scratch.screen.vz << 16) >> 0x12;
        if (value < 0)
        {
            goto zero_first;
        }
        priority = 0x4e1;
        if (value < 0x4e2)
        {
            priority = value;
        }
        goto first_done;
    zero_first:
        priority = 0;
    first_done:
        GsSortSprite(spr, OTablePt, (u16)priority);

        value = (s32)((u16)scratch.screen.vz << 16) >> 0x12;
        if (value < 0)
        {
            goto zero_second;
        }
        priority = 0x4e1;
        if (value < 0x4e2)
        {
            priority = value;
        }
        goto second_done;
    zero_second:
        priority = 0;
    second_done:
        GsSortSprite(spr2, OTablePt, (u16)priority);
        return;
    }

    goto normal_state;

state_two:
    {
        u16 count;

        count = param->time;
        param->time = count - 1;
        if ((s16)count <= 0)
        {
            param->time = 0x80;
            param->unk23++;
        }
        goto move_and_draw;
    }

state_one:
    {
        u16 count;

        param->scale += rand() % 0x1000;
        count = param->time;
        param->time = count - 1;
        if ((s16)count <= 0)
        {
            param->unk23++;
            param->time = rand() % 90;
        }
        goto move_and_draw;
    }

normal_state:
    {
        s32 x;
        s32 y;
        s32 z;
        s32 x10;
        s32 y10;
        s32 z10;
        s32 level;
        s32 node_y;
        AreaNodeType *node;
        int r;
        int random_x;
        int random_y;
        int random_z;
        s32 bleed_x;
        s32 bleed_y;
        s32 bleed_z;
        u16 count;
        SVECTOR *temporary;
        SVECTOR *velocity;
        long color;
        long green;
        int cursor;
        int searched;
        TEffectSlot *slot;
        TEffectSlot *base;
        TEffectSlot *found;
        BleedType *bleed;

        x = param->px;
        y = param->py;
        z = param->pz;
        x10 = x / 10;
        y10 = y / 10;
        z10 = z / 10;
        param->vy += 10;
        node = param->hint;
        if (node != 0)
        {
            node_y = node->y;
            if (y10 < node_y - 200 || node_y < y10 || x10 < node->x1 ||
                z10 < node->z1 || node->x2 < x10 || node->z2 < z10)
            {
                goto map_level;
            }
            goto compute_level;
        }
    map_level:
        level = GetAreaMapLevel(GlobalAreaMap, x, y - 300, z, 0);
        if (y <= level && FieldArea->division == -1)
        {
            param->hint = FieldArea;
        }
        goto level_done;
    compute_level:
        if (node->dy == 0)
        {
            goto flat_level;
        }
        level = ComputeAreaLevel(node, x10, z10);
        if (level == (s32)0x80000000)
        {
            goto level_done;
        }
        level *= 10;
        goto level_done;
    flat_level:
        level = node_y * 10;
    level_done:
        if (param->py < level)
        {
            goto expire;
        }
        param->vz = 0;
        param->vy = 0;
        param->vx = 0;
        if (level == (s32)0x80000000)
        {
            goto no_floor;
        }
        param->py = level;
        goto bounce_done;
    no_floor:
        param->vy = rand() % 8 + 8;
        param->rotate = 0;
        r = rand();
        param->unk22 += 2;
        param->scale = r % 0x2ab + 0x555;
    bounce_done:
        param->unk23 = 1;
        param->time = rand() % 10;
        SoundEx((VECTOR *)&param->px, 0x37);
        goto spawn_bleed;

    expire:
        count = param->time;
        param->time = count - 1;
        if ((s16)count <= 0)
        {
            ef->proc = 0;
        }

    spawn_bleed:
        memset(&scratch.bleed.temporary.position, 0, sizeof(VECTOR));
        random_x = rand();
        bleed_x = param->px - 60;
        scratch.bleed.temporary.position.vx = bleed_x + random_x % 120;
        random_y = rand();
        bleed_y = param->py - 60;
        scratch.bleed.temporary.position.vy = bleed_y + random_y % 120;
        random_z = rand();
        bleed_z = param->pz - 60;
        scratch.bleed.temporary.position.vz = bleed_z + random_z % 120;
        scratch.bleed.position = scratch.bleed.temporary.position;
        temporary = &scratch.bleed.temporary.velocity;
        memset(&scratch.bleed.temporary.velocity, 0, sizeof(SVECTOR));
        velocity = &scratch.bleed.velocity;
        color = 0x7f1017;
        scratch.bleed.temporary.velocity.vx = param->vx / 2;
        scratch.bleed.temporary.velocity.vy = param->vy / 2;
        scratch.bleed.temporary.velocity.vz = param->vz / 2;
        *velocity = *temporary;

        base = EffectSlot;
        cursor = CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_;
        slot = base + cursor;
        searched = 0;
        do
        {
            cursor++;
            slot++;
            if (199 < cursor)
            {
                slot = base;
                cursor = 0;
            }
            searched++;
            if (slot->proc == 0)
            {
                CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = cursor + 1;
                bleed = &slot->param.bleed;
                if (199 < CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_)
                {
                    CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = 0;
                }
                found = slot;
                goto bleed_found;
            }
        } while (searched < 200);
        found = &dmy;
        bleed = &dmy.param.bleed;
    bleed_found:
        found->param.bleed.pos = scratch.bleed.position;
        found->param.bleed.vec = *velocity;
        bleed->time = 7;
        bleed->r = 0x7f;
        green = 0x7f10;
        bleed->g = green;
        bleed->b = color;
        bleed->mode = 0;
        found->proc = (void (*)())DrawBleed;
    }

move_and_draw:
    {
        s32 size;
        s32 otz;
        s16 scale;
        s32 value;
        s32 priority;

        param->px += param->vx;
        param->py += param->vy;
        param->pz += param->vz;
        spr->rotate = param->rotate;
        spr->attribute = 0;
        spr->r = param->mode;
        spr->g = param->mode;
        spr->b = param->mode;
        size = param->scale;
        GetScreenPosition(param->px, param->py, param->pz, &scratch.screen);
        otz = scratch.screen.vz;
        if (otz < 0x25)
        {
            return;
        }
        scale = (s16)((size * 300) / otz) + 1;
        spr->scaley = scale;
        spr->scalex = scale;
        spr->x = scratch.screen.vx;
        spr->y = scratch.screen.vy;
        value = (s32)((u16)scratch.screen.vz << 16) >> 0x12;
        if (value < 0)
        {
            goto zero_final;
        }
        priority = 0x4e1;
        if (value < 0x4e2)
        {
            priority = value;
        }
        goto final_done;
    zero_final:
        priority = 0;
    final_done:
        GsSortSprite(spr, OTablePt, (u16)priority);
    }
}

// triage: HARD — 582 insns, mul/div, 1 loop, 7 callees, ~0.07 to ProcItemKusuri
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_8003562c(undefined4 *param_1)
//
// {
//   byte bVar1;
//   undefined1 uVar2;
//   short sVar3;
//   uint uVar4;
//   int iVar5;
//   _180fake_1 *p_Var6;
//   AreaNodeType *node;
//   tag_EffectSlot *ptVar7;
//   GsSPRITE *_29;
//   ushort uVar8;
//   int iVar9;
//   int iVar10;
//   uchar uVar11;
//   int iVar12;
//   long lVar13;
//   GsSPRITE *_29_00;
//   undefined4 local_48;
//   int local_44;
//   AreaNodeType *local_40;
//   long local_3c;
//   long local_38;
//   long local_34;
//   AreaNodeType *local_30;
//   long local_2c;
//   int local_28;
//   long local_24;
//
//   uVar4 = (uint)*(byte *)((int)param_1 + 0x26);
//   iVar5 = uVar4 * 0x24;
//   _29_00 = &sprBlood + uVar4;
//   _29 = (GsSPRITE *)(&DAT_800be9e8 + uVar4 * 9);
//   bVar1 = *(byte *)((int)param_1 + 0x27);
//   if (bVar1 == 2) {
//     uVar8 = *(ushort *)(param_1 + 7);
//     *(ushort *)(param_1 + 7) = uVar8 - 1;
//     if ((int)((uint)uVar8 << 0x10) < 1) {
//       *(undefined2 *)(param_1 + 7) = 0x80;
//       *(char *)((int)param_1 + 0x27) = *(char *)((int)param_1 + 0x27) + '\x01';
//     }
//   }
//   else {
//     if (bVar1 < 3) {
//       if (bVar1 == 1) {
//         iVar9 = rand();
//         iVar5 = iVar9;
//         if (iVar9 < 0) {
//           iVar5 = iVar9 + 0xfff;
//         }
//         uVar8 = *(ushort *)(param_1 + 7);
//         param_1[5] = param_1[5] + iVar9 + (iVar5 >> 0xc) * -0x1000;
//         *(ushort *)(param_1 + 7) = uVar8 - 1;
//         if ((int)((uint)uVar8 << 0x10) < 1) {
//           *(char *)((int)param_1 + 0x27) = *(char *)((int)param_1 + 0x27) + '\x01';
//           iVar5 = rand();
//           *(short *)(param_1 + 7) = (short)iVar5 + (short)(iVar5 / 0x5a) * -0x5a;
//         }
//         goto LAB_80035df0;
//       }
//     }
//     else if (bVar1 == 3) {
//       sVar3 = *(short *)(param_1 + 9);
//       *(ushort *)(param_1 + 9) = sVar3 - 5U;
//       if ((int)((uint)(ushort)(sVar3 - 5U) << 0x10) < 1) {
//         *(undefined2 *)(param_1 + 9) = 0;
//         *param_1 = 0;
//       }
//       _29_00->attribute = 0x50000000;
//       iVar9 = param_1[3];
//       iVar12 = param_1[5];
//       uVar8 = *(ushort *)(param_1 + 9);
//       param_1[3] = iVar9 + *(short *)(param_1 + 8);
//       lVar13 = param_1[6];
//       iVar10 = (uint)uVar8 << 0x10;
//       GetScreenPosition(param_1[2],iVar9 + *(short *)(param_1 + 8),param_1[4],&local_48);
//       iVar9 = (int)(short)local_44;
//       if (iVar9 < 0x25) {
//         return;
//       }
//       if (iVar9 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar9 == -1) && (iVar12 * 300 == -0x80000000)) {
//         trap(0x1800);
//       }
//       sVar3 = (short)((iVar12 * 300) / iVar9) + 1;
//       (&sprBlood)[uVar4].scaley = sVar3;
//       (&sprBlood)[uVar4].scalex = sVar3;
//       *(short *)(&DAT_800bea06 + iVar5) = sVar3;
//       *(short *)(&DAT_800bea04 + iVar5) = sVar3;
//       (&sprBlood)[uVar4].rotate = lVar13;
//       *(long *)(&DAT_800bea08 + iVar5) = lVar13;
//       (&sprBlood)[uVar4].x = (short)local_48;
//       *(short *)(&DAT_800be9ec + iVar5) = (short)local_48;
//       (&sprBlood)[uVar4].y = local_48._2_2_;
//       *(short *)(&DAT_800be9ee + iVar5) = local_48._2_2_;
//       uVar11 = (uchar)uVar8;
//       (&sprBlood)[uVar4].r = uVar11;
//       (&sprBlood)[uVar4].g = uVar11;
//       (&sprBlood)[uVar4].b = uVar11;
//       uVar2 = (undefined1)((iVar10 >> 0x10) - (iVar10 >> 0x1f) >> 1);
//       (&DAT_800be9fc)[iVar5] = uVar2;
//       (&DAT_800be9fd)[iVar5] = uVar2;
//       (&DAT_800be9fe)[iVar5] = uVar2;
//       iVar5 = (local_44 << 0x10) >> 0x12;
//       if (iVar5 < 0) {
//         iVar9 = 0;
//       }
//       else {
//         iVar9 = 0x4e1;
//         if (iVar5 < 0x4e2) {
//           iVar9 = iVar5;
//         }
//       }
//       GsSortSprite(_29_00,OTablePt,(ushort)iVar9);
//       iVar5 = (local_44 << 0x10) >> 0x12;
//       if (iVar5 < 0) {
//         iVar9 = 0;
//       }
//       else {
//         iVar9 = 0x4e1;
//         if (iVar5 < 0x4e2) {
//           iVar9 = iVar5;
//         }
//       }
//       uVar8 = (ushort)iVar9;
//       goto LAB_80035f10;
//     }
//     iVar12 = param_1[3];
//     iVar5 = (int)param_1[2] / 10;
//     *(short *)(param_1 + 8) = *(short *)(param_1 + 8) + 10;
//     node = (AreaNodeType *)param_1[1];
//     iVar9 = (int)param_1[4] / 10;
//     if (node == (AreaNodeType *)0x0) {
// LAB_800359d4:
//       iVar10 = GetAreaMapLevel(GlobalAreaMap,param_1[2],iVar12 + -300);
//       if ((iVar12 <= iVar10) && (FieldArea->division == -1)) {
//         param_1[1] = FieldArea;
//       }
//     }
//     else {
//       iVar10 = (int)node->y;
//       if (((((iVar12 / 10 < iVar10 + -200) || (iVar10 < iVar12 / 10)) || (iVar5 < node->x1)) ||
//           ((iVar9 < node->z1 || (node->x2 < iVar5)))) || (node->z2 < iVar9)) goto LAB_800359d4;
//       if ((node->dy == 0) || (iVar10 = ComputeAreaLevel(node,iVar5,iVar9), iVar10 != -0x80000000)) {
//         iVar10 = iVar10 * 10;
//       }
//     }
//     if ((int)param_1[3] < iVar10) {
//       uVar8 = *(ushort *)(param_1 + 7);
//       *(ushort *)(param_1 + 7) = uVar8 - 1;
//       if ((int)((uint)uVar8 << 0x10) < 1) {
//         *param_1 = 0;
//       }
//     }
//     else {
//       *(undefined2 *)((int)param_1 + 0x22) = 0;
//       *(undefined2 *)(param_1 + 8) = 0;
//       *(undefined2 *)((int)param_1 + 0x1e) = 0;
//       if (iVar10 == -0x80000000) {
//         iVar9 = rand();
//         iVar5 = iVar9;
//         if (iVar9 < 0) {
//           iVar5 = iVar9 + 7;
//         }
//         *(short *)(param_1 + 8) = (short)iVar9 + (short)(iVar5 >> 3) * -8 + 8;
//         param_1[6] = 0;
//         iVar5 = rand();
//         *(char *)((int)param_1 + 0x26) = *(char *)((int)param_1 + 0x26) + '\x02';
//         param_1[5] = iVar5 % 0x2ab + 0x555;
//       }
//       else {
//         param_1[3] = iVar10;
//       }
//       *(undefined1 *)((int)param_1 + 0x27) = 1;
//       iVar5 = rand();
//       *(short *)(param_1 + 7) = (short)iVar5 + (short)(iVar5 / 10) * -10;
//       SoundEx((VECTOR *)(param_1 + 2),0x37);
//     }
//     memset((uchar *)&local_30,'\0',0x10);
//     iVar5 = rand();
//     local_30 = (AreaNodeType *)(param_1[2] + -0x3c + iVar5 % 0x78);
//     iVar5 = rand();
//     local_2c = param_1[3] + -0x3c + iVar5 % 0x78;
//     iVar5 = rand();
//     local_38 = param_1[4] + -0x3c + iVar5 % 0x78;
//     local_40 = local_30;
//     local_3c = local_2c;
//     local_34 = local_24;
//     local_28 = local_38;
//     memset((uchar *)&local_30,'\0',8);
//     iVar9 = 0;
//     iVar5 = (uint)*(ushort *)((int)param_1 + 0x1e) << 0x10;
//     local_48 = (AreaNodeType *)
//                CONCAT22((short)(((int)((uint)*(ushort *)(param_1 + 8) << 0x10) >> 0x10) -
//                                 ((int)((uint)*(ushort *)(param_1 + 8) << 0x10) >> 0x1f) >> 1),
//                         (short)((iVar5 >> 0x10) - (iVar5 >> 0x1f) >> 1));
//     ptVar7 = EffectSlot + DAT_80097a3c;
//     iVar5 = (uint)*(ushort *)((int)param_1 + 0x22) << 0x10;
//     local_2c = CONCAT22(local_2c._2_2_,(short)((iVar5 >> 0x10) - (iVar5 >> 0x1f) >> 1));
//     local_44 = local_2c;
//     iVar5 = DAT_80097a3c;
//     do {
//       iVar5 = iVar5 + 1;
//       ptVar7 = ptVar7 + 1;
//       if (199 < iVar5) {
//         iVar5 = 0;
//         ptVar7 = EffectSlot;
//       }
//       iVar9 = iVar9 + 1;
//       if (ptVar7->proc == (undefined **)0x0) {
//         DAT_80097a3c = iVar5 + 1;
//         p_Var6 = &ptVar7->param;
//         if (199 < DAT_80097a3c) {
//           DAT_80097a3c = 0;
//         }
//         goto LAB_80035d88;
//       }
//     } while (iVar9 < 200);
//     ptVar7 = &dmy;
//     p_Var6 = &dmy.param;
// LAB_80035d88:
//     (ptVar7->param).blood.hint = local_40;
//     (ptVar7->param).blood.px = local_3c;
//     (ptVar7->param).blood.py = local_38;
//     (ptVar7->param).blood.pz = local_34;
//     (ptVar7->param).blood.scale = (long)local_48;
//     (ptVar7->param).blood.rotate = local_2c;
//     (p_Var6->bleed).time = '\a';
//     *(undefined1 *)((int)p_Var6 + 0x18) = 0x7f;
//     (p_Var6->bleed).g = '\x10';
//     (p_Var6->bleed).b = '\x17';
//     *(undefined1 *)((int)p_Var6 + 0x1c) = 0;
//     ptVar7->proc = (undefined **)DrawBleed;
//     local_30 = local_48;
//   }
// LAB_80035df0:
//   param_1[3] = param_1[3] + (int)*(short *)(param_1 + 8);
//   param_1[2] = param_1[2] + (int)*(short *)((int)param_1 + 0x1e);
//   param_1[4] = param_1[4] + (int)*(short *)((int)param_1 + 0x22);
//   (&sprBlood)[uVar4].rotate = param_1[6];
//   _29_00->attribute = 0;
//   (&sprBlood)[uVar4].r = *(uchar *)(param_1 + 9);
//   (&sprBlood)[uVar4].g = *(uchar *)(param_1 + 9);
//   (&sprBlood)[uVar4].b = *(uchar *)(param_1 + 9);
//   iVar9 = param_1[5];
//   GetScreenPosition(param_1[2],param_1[3],param_1[4],&local_48);
//   iVar5 = (int)(short)local_44;
//   if (iVar5 < 0x25) {
//     return;
//   }
//   if (iVar5 == 0) {
//     trap(0x1c00);
//   }
//   if ((iVar5 == -1) && (iVar9 * 300 == -0x80000000)) {
//     trap(0x1800);
//   }
//   sVar3 = (short)((iVar9 * 300) / iVar5) + 1;
//   (&sprBlood)[uVar4].scaley = sVar3;
//   (&sprBlood)[uVar4].scalex = sVar3;
//   (&sprBlood)[uVar4].x = (short)local_48;
//   (&sprBlood)[uVar4].y = local_48._2_2_;
//   iVar5 = (local_44 << 0x10) >> 0x12;
//   if (iVar5 < 0) {
//     iVar9 = 0;
//   }
//   else {
//     iVar9 = 0x4e1;
//     if (iVar5 < 0x4e2) {
//       iVar9 = iVar5;
//     }
//   }
//   uVar8 = (ushort)iVar9;
//   _29 = _29_00;
// LAB_80035f10:
//   GsSortSprite(_29,OTablePt,uVar8);
//   return;
// }
