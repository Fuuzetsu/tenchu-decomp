#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DefaultActionHumanoid(struct Humanoid *human);
 *     HUMAN.C:155, 175 src lines, frame 120 bytes, saved-reg mask 0x80ff0000 (DEMO build -- see below)
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
 *     reg   $s3       struct MapVector * map
 *     reg   $s5       struct SVECTOR * vector
 *     reg   $s2       struct VECTOR * locate
 *     reg   $s4       struct VECTOR * slocate
 *     reg   $s7       struct ModelType * object
 *     reg   $a2       long i
 *     reg   $s4       long xx
 *     reg   $v1       long yy
 *     reg   $s0       long zz
 *     reg   $s0       long ry
 *     stack sp+32     struct VECTOR position
 *     stack sp+48     struct MapVector mv
 *     stack sp+64     struct VECTOR position
 *
 * Globals it touches, as the original declared them:
 *     extern struct AreaNodeType *FieldArea;
 *     extern struct NodeIndexType *FieldIndex;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct Humanoid *StagePlayer;
 *     extern short RefrectMove[16][2];
 *     extern struct SVECTOR ConflictDistance;
 * END PSX.SYM */

/*
 * The nested one-shot loops around the conflict pointer and object id place
 * sched1 loop-note fences between the target's pointer, id, and size loads.
 * The identical yy arms add a zero-code CFG fence without loop-weighting the
 * collision pointer and rotating its a1/a2 allocation.
 */

extern s16 RefrectVector[16];

extern s32 GetAreaMapVector(u32 *area, MapVector *map, VECTOR *position,
                           s32 width, s16 mode);
extern s16 GetConflictResult(ModelType *model, s16 index);
extern s16 GetDirection(s32 x, s32 z, s16 rotate);

short DefaultActionHumanoid(Humanoid *human)
{
    MapVector *map;
    SVECTOR *vector;
    VECTOR *locate;
    VECTOR *slocate;
    ModelType *object;
    long i;
    long xx;
    long yy;
    long zz;
    long ry;
    long direction;

    i = 1;
    map = &human->map;
    locate = human->locate;
    vector = &human->vector;
    object = *human->model->object;
    human->rotate->vy &= 0xfff;
    human->attribute = (u8)human->attribute;
    slocate = &human->slocate;

    if (map->vector == 0)
    {
        i = 0x11;
    }
    if (human->type != 0x85)
    {
        FieldArea = map->area;
        FieldIndex = map->index;
    }

    {
        VECTOR position;
        VECTOR *wide;
        MapVector *call_map;

        if (human->status == 7)
        {
            goto use_conflict_position;
        }
        call_map = map;
        if (human->status == 0xb)
        {
            goto use_locate_position;
        }
        wide = locate;
        if (map->height != 0)
        {
            goto probe_map;
        }
        {
            s32 abs_x;
            s32 abs_z;

            abs_x = vector->vx;
            if (abs_x < 0)
            {
                abs_x = -abs_x;
            }
            if (abs_x >= 0x51)
            {
                goto use_conflict_position;
            }
            abs_z = vector->vz;
            if (abs_z < 0)
            {
                abs_z = -abs_z;
            }
            if (abs_z < 0x51)
            {
                goto probe_map;
            }
        }

use_conflict_position:
        position = ConflictObject[(*human->model->object)->id].position;
        position.vy = locate->vy;
        GetAreaMapVector(GlobalAreaMap, map, &position, human->width, i);
        goto map_probe_done;
use_locate_position:
        wide = locate;
probe_map:
        GetAreaMapVector(GlobalAreaMap, call_map, wide, human->width, i);
map_probe_done:
        ;
    }

    if (map->attrib & 2)
    {
        human->attribute |= 0x200;
        if (vector->vy < 0)
        {
            vector->vy = 0;
        }
        map->height = 1;
    }

    if (map->height > 0 && (human->attribute & 0x20) == 0)
    {
        human->attribute |= 0x100;
        if (vector->vy < 400)
        {
            vector->vy += 20;
        }
        if (map->attrib & 0x200)
        {
            if (map->height < 25000 && human->life != 0)
            {
                if (human == StagePlayer)
                {
                    Sound(human, 0x49);
                    SetCameraMode(CMODE_FALL);
                }
                else
                {
                    Sound(human, 8);
                }
                human->life = 0;
                if ((human->type & 0xf0) != 0x80 && human != StagePlayer)
                {
                    ReqLifeBar(human);
                }
            }
            goto ground_motion;
        }
    }
    else
    {
        if (vector->vy > 0 || map->level == (s32)0x80000000)
        {
            if ((map->attrib & 0x200) == 0)
            {
                human->attribute |= 0x800;
            }
            if (map->level != (s32)0x80000000)
            {
                locate->vy = map->level;
            }
            vector->vy = 0;
        }
ground_motion:
        if ((map->attrib & 0x200) && map->height == 0 && human->status != 0x11)
        {
            SetNowMotion(human, 0x1100, 1);
        }
    }

    if (*(s32 *)&map->vector & (s32)0xffff0000)
    {
        u16 attribute;

        attribute = human->attribute;
        human->attribute = attribute | 0x2000;
        if (map->height < -450)
        {
            human->attribute = attribute | 0x3000;
            locate->vy = map->level;
        }
        else if (map->height < 0)
        {
            locate->vy = map->level;
        }
    }

    if (map->vector != 0)
    {
        human->attribute |= 0x400;
        zz = map->level;
        if (zz == (s32)0x80000000)
        {
            {
                MapVector mv;
                VECTOR position;
                s32 dx;
                s32 dz;
                s32 coefficient_x;
                s32 coefficient_z;

                position = ConflictObject[(*human->model->object)->id].position;
                locate->vx = slocate->vx;
                locate->vz = slocate->vz;
                locate->vy = slocate->vy;
                position.vy = locate->vy - 500;
                GetAreaMapVector(GlobalAreaMap, &mv, &position, 300, 4);

                coefficient_x = RefrectMove[mv.vector][0];
                dx = position.vx - locate->vx;
                locate->vx += coefficient_x * ((dx >= 0) ? dx : -dx);

                coefficient_z = RefrectMove[mv.vector][1];
                dz = position.vz - locate->vz;
                locate->vz += coefficient_z * ((dz >= 0) ? dz : -dz);

                if (mv.level != zz && locate->vy < mv.level)
                {
                    locate->vy = (vector->vy > 0)
                        ? locate->vy + vector->vy
                        : locate->vy + 20;
                }
                else
                {
                    vector->vz = 0;
                    vector->vx = 0;
                }
            }
        }
        else
        {
            s32 angle_abs;

            direction = map->vector;
            ry = RefrectVector[direction];
            if (ry == -1)
            {
                if (vector->vx != 0)
                {
                    goto reflect_motion;
                }
                if (vector->vz == 0)
                {
                    goto reflect_width;
                }
reflect_motion:
                xx = -vector->vx;
                zz = -vector->vz;
                goto apply_reflection;
reflect_width:
                i = ((s32)((u16)human->width << 16)) >> 18;
                xx = RefrectMove[direction][0] * i;
                zz = RefrectMove[direction][1] * i;
apply_reflection:
                locate->vx += xx;
                locate->vz += zz;
            }
            else
            {
                xx = (rsin(ry) * human->width) >> 14;
                yy = rcos(ry);
                i = human->rotate->vy - ry;
                zz = (yy * human->width) >> 14;
                angle_abs = (i >= 0) ? i : -i;
                if (angle_abs >= 2000)
                {
                    i = (i > 0) ? i - 0x1000 : i + 0x1000;
                }
                angle_abs = (i >= 0) ? i : -i;
                if (angle_abs < 0x708 || human != StagePlayer || map->height != 0)
                {
                    if (map->angleH == 0 &&
                        (human->status == 2 || human->status == 6))
                    {
                        {
                            SVECTOR *rotate;
                            s32 rotate_y;

                            rotate = human->rotate;
                            rotate_y = rotate->vy;
                            if (i > 0)
                            {
                                rotate_y -= 0x20;
                            }
                            else
                            {
                                rotate_y += 0x20;
                            }
                            rotate->vy = rotate_y;
                        }
                        MoveHumanoid(human, human->motion->motion->orderspd,
                                     human->motion->motion->sidespd);
                    }
                    xx >>= 1;
                    zz >>= 1;
                }
                locate->vx -= xx;
                locate->vz -= zz;
            }
        }
    }

    if (object->attribute & 0x8000)
    {
        while (1)
        {
            i = GetConflictResult(object, -1);
            if (i < 0)
            {
                break;
            }
            if (ConflictObject[i].common == human)
            {
                continue;
            }
            if (ConflictObject[i].size.pad & 1)
            {
                if (human->status != 0x11)
                {
                    u16 attribute;

                    attribute = human->attribute;
                    human->vector.pad = i;
                    human->attribute = attribute | 0x4000;
                }
                continue;
            }
            if ((ConflictObject[i].size.pad & 8) == 0 &&
                (human->attribute & 0x20) == 0)
            {
                s32 top;
                s32 object_y;
                s32 size_y;
                s32 object_id;
                ConflictObjectType *conflict;

                human->attribute |= 0x8000;
                xx = locate->vx;
                zz = locate->vz;

                locate->vx = xx - ((ConflictDistance.vx >= 0)
                    ? human->width : -human->width) / 8;

                locate->vz -= ((ConflictDistance.vz >= 0)
                    ? human->width : -human->width) / 8;

                do
                {
                    do
                    {
                        conflict = &ConflictObject[i];
                    }
                    while (0);
                    object_id = object->id;
                }
                while (0);
                size_y = conflict->size.vy;
                if (object_id != 0)
                {
                    yy = conflict->position.vy;
                }
                else
                {
                    yy = conflict->position.vy;
                }
                object_y = ConflictObject[object_id].position.vy;
                top = yy - size_y;
                if (object_y < top)
                {
                    locate->vx = xx;
                    locate->vz = zz;
                    if (conflict->size.pad & 4)
                    {
                        vector->vy = 0;
                        map->level = top;
                        locate->vy = top;
                        map->height = 0;
                        map->attrib &= 0xff80;
                        human->attribute &= 0x7eff;
                    }
                    else
                    {
                        vector->vy = 10;
                    }
                    continue;
                }
                if (yy + size_y < object_y &&
                    (human->status == 0 || human->status == 2 ||
                     human->status == 5 || human->status == 6))
                {
                    s32 direction_abs;

                    zz = GetDirection(ConflictObject[i].position.vx - locate->vx,
                                      ConflictObject[i].position.vz - locate->vz,
                                      human->locate->vy);
                    do
                    {
                        do
                        {
                            do
                            {
                                do
                                {
                                    do
                                    {
                                        do
                                        {
                                            do
                                            {
                                                do
                                                {
                                                    do
                                                    {
                                                        do
                                                        {
                                                            do
                                                            {
                                                                do
                                                                {
                                                                    do
                                                                    {
                                                                        do
                                                                        {
                                                                            direction_abs = zz >= 0 ? zz : -zz;
                                                                        }
                                                                        while (0);
                                                                    }
                                                                    while (0);
                                                                }
                                                                while (0);
                                                            }
                                                            while (0);
                                                        }
                                                        while (0);
                                                    }
                                                    while (0);
                                                }
                                                while (0);
                                            }
                                            while (0);
                                        }
                                        while (0);
                                    }
                                    while (0);
                                    direction = 0x1003;
                                }
                                while (0);
                                if (direction_abs < 0x44c)
                                {
                                    direction = 0x1000;
                                }
                            }
                            while (0);
                            SetNowMotion(human, direction, 1);
                        }
                        while (0);
                        Sound(human, 6);
                    }
                    while (0);
                }

                {
                    SVECTOR *rotate;
                    s32 rotate_y;

                    rotate = human->rotate;
                    yy = rotate->vy;
                    if (i & 1)
                    {
                        rotate_y = yy + human->turn;
                    }
                    else
                    {
                        rotate_y = yy - human->turn;
                    }
                    rotate->vy = rotate_y;
                }
                if (human->vector.vx != 0 || human->vector.vz != 0)
                {
                    MoveHumanoid(human, human->motion->motion->orderspd,
                                 human->motion->motion->sidespd);
                    if (human->trace != 0 && (human->attribute & 8))
                    {
                        human->trace->count = -20;
                    }
                }
            }
        }
    }
    return human->attribute;
}

// triage: VERY-HARD — 656 insns, mul/div, 7 loop, 10 callees, ~0.05 to ProcItemDrop
// likely-relevant cookbook sections:
//   - Loops: 7 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short DefaultActionHumanoid(Humanoid *human)
//
// {
//   ushort uVar1;
//   ulong *mvp;
//   short sVar2;
//   int iVar3;
//   int iVar4;
//   int iVar5;
//   int iVar6;
//   MotionDataType *pMVar7;
//   int iVar8;
//   int iVar9;
//   VECTOR *wide;
//   int iVar10;
//   long lVar11;
//   uint uVar12;
//   VECTOR *pVVar13;
//   MapVector *pos;
//   ModelType *model;
//   VECTOR local_60;
//   VECTOR local_50;
//   int local_38;
//   int local_34;
//   int local_30;
//   long local_2c;
//
//   pos = &human->map;
//   pVVar13 = human->locate;
//   model = *human->model->object;
//   human->rotate->vy = human->rotate->vy & 0xfff;
//   human->attribute = (ushort)(byte)human->attribute;
//   if (human->type != 0x85) {
//     FieldArea = *(AreaNodeType **)&human->field10_0x30;
//     FieldIndex = *(NodeIndexType **)&human->field14_0x34;
//   }
//   if (human->status == 7) {
// LAB_80028214:
//     sVar2 = (*human->model->object)->id;
//     local_60.vx = ConflictObject[sVar2].position.vx;
//     local_60.vz = ConflictObject[sVar2].position.vz;
//     local_60.pad = ConflictObject[sVar2].position.pad;
//     local_60.vy = pVVar13->vy;
//     wide = &local_60;
//   }
//   else {
//     wide = pVVar13;
//     if ((human->status != 0xb) && ((human->map).height == 0)) {
//       iVar3 = (int)(human->vector).vx;
//       if (iVar3 < 0) {
//         iVar3 = -iVar3;
//       }
//       if (iVar3 < 0x51) {
//         iVar3 = (int)(human->vector).vz;
//         if (iVar3 < 0) {
//           iVar3 = -iVar3;
//         }
//         if (iVar3 < 0x51) goto LAB_80028290;
//       }
//       goto LAB_80028214;
//     }
//   }
// LAB_80028290:
//   GetAreaMapVector((MapVector *)GlobalAreaMap,(VECTOR *)pos,(long)wide);
//   if (((human->map).attrib & 2U) != 0) {
//     human->attribute = human->attribute | 0x200;
//     if ((human->vector).vy < 0) {
//       (human->vector).vy = 0;
//     }
//     (human->map).height = 1;
//   }
//   if (((human->map).height < 1) || ((human->attribute & 0x20U) != 0)) {
//     if ((0 < (human->vector).vy) || (pos->level == -0x80000000)) {
//       if (((human->map).attrib & 0x200U) == 0) {
//         human->attribute = human->attribute | 0x800;
//       }
//       if (pos->level != -0x80000000) {
//         pVVar13->vy = pos->level;
//       }
//       (human->vector).vy = 0;
//     }
// LAB_8002841c:
//     if (((((human->map).attrib & 0x200U) != 0) && ((human->map).height == 0)) &&
//        (human->status != 0x11)) {
//       SetNowMotion(human,0x1100,1);
//     }
//   }
//   else {
//     human->attribute = human->attribute | 0x100;
//     if ((human->vector).vy < 400) {
//       (human->vector).vy = (human->vector).vy + 0x14;
//     }
//     if (((human->map).attrib & 0x200U) != 0) {
//       if (((human->map).height < 25000) && (human->life != 0)) {
//         if (human == StagePlayer) {
//           Sound(human,0x49);
//           SetCameraMode(CMODE_FALL);
//         }
//         else {
//           Sound(human,8);
//         }
//         human->life = 0;
//         if (((human->type & 0xf0U) != 0x80) && (human != StagePlayer)) {
//           ReqLifeBar(human);
//         }
//       }
//       goto LAB_8002841c;
//     }
//   }
//   uVar12._0_1_ = (human->map).vector;
//   uVar12._1_1_ = (human->map).direct;
//   uVar12._2_1_ = (human->map).angleL;
//   uVar12._3_1_ = (human->map).angleH;
//   if ((uVar12 & 0xffff0000) != 0) {
//     uVar1 = human->attribute;
//     human->attribute = uVar1 | 0x2000;
//     iVar3 = (human->map).height;
//     if (iVar3 < -0x1c2) {
//       human->attribute = uVar1 | 0x3000;
//     }
//     else if (-1 < iVar3) goto LAB_800284b0;
//     pVVar13->vy = pos->level;
//   }
// LAB_800284b0:
//   if ((human->map).vector != '\0') {
//     human->attribute = human->attribute | 0x400;
//     mvp = GlobalAreaMap;
//     if (pos->level == -0x80000000) {
//       sVar2 = (*human->model->object)->id;
//       local_38 = ConflictObject[sVar2].position.vx;
//       local_30 = ConflictObject[sVar2].position.vz;
//       local_2c = ConflictObject[sVar2].position.pad;
//       pVVar13->vx = (human->slocate).vx;
//       pVVar13->vz = (human->slocate).vz;
//       local_34 = (human->slocate).vy;
//       pVVar13->vy = local_34;
//       local_34 = local_34 + -500;
//       GetAreaMapVector((MapVector *)mvp,&local_50,(long)&local_38);
//       iVar3 = local_38 - pVVar13->vx;
//       if (iVar3 < 0) {
//         iVar3 = -iVar3;
//       }
//       pVVar13->vx = pVVar13->vx + RefrectMove[0][(uint)(byte)local_50.pad * 2] * iVar3;
//       iVar3 = local_30 - pVVar13->vz;
//       if (iVar3 < 0) {
//         iVar3 = -iVar3;
//       }
//       pVVar13->vz = pVVar13->vz + RefrectMove[0][(uint)(byte)local_50.pad * 2 + 1] * iVar3;
//       if ((local_50.vx == -0x80000000) || (iVar3 = pVVar13->vy, local_50.vx <= iVar3)) {
//         (human->vector).vz = 0;
//         (human->vector).vx = 0;
//       }
//       else {
//         iVar4 = (int)(human->vector).vy;
//         iVar5 = iVar3 + iVar4;
//         if (iVar4 < 1) {
//           iVar5 = iVar3 + 0x14;
//         }
//         pVVar13->vy = iVar5;
//       }
//     }
//     else {
//       uVar12 = (uint)(human->map).vector;
//       iVar3 = (int)RefrectVector[uVar12];
//       if (iVar3 == -1) {
//         iVar4 = (int)(human->vector).vx;
//         if ((iVar4 == 0) && ((human->vector).vz == 0)) {
//           iVar8 = (int)((uint)(ushort)human->width << 0x10) >> 0x12;
//           iVar4 = RefrectMove[0][uVar12 * 2] * iVar8;
//           iVar8 = RefrectMove[0][uVar12 * 2 + 1] * iVar8;
//         }
//         else {
//           iVar4 = -iVar4;
//           iVar8 = -(int)(human->vector).vz;
//         }
//         iVar4 = pVVar13->vx + iVar4;
//         iVar8 = pVVar13->vz + iVar8;
//       }
//       else {
//         iVar5 = rsin(iVar3);
//         iVar5 = iVar5 * human->width;
//         iVar4 = iVar5 >> 0xe;
//         iVar9 = rcos(iVar3);
//         iVar9 = iVar9 * human->width;
//         iVar3 = human->rotate->vy - iVar3;
//         iVar10 = iVar3;
//         if (iVar3 < 0) {
//           iVar10 = -iVar3;
//         }
//         iVar8 = iVar9 >> 0xe;
//         iVar6 = iVar3;
//         if ((1999 < iVar10) && (iVar6 = iVar3 + -0x1000, iVar3 < 1)) {
//           iVar6 = iVar3 + 0x1000;
//         }
//         iVar3 = iVar6;
//         if (iVar6 < 0) {
//           iVar3 = -iVar6;
//         }
//         if (((iVar3 < 0x708) || (human != StagePlayer)) || ((human->map).height != 0)) {
//           if (((human->map).angleH == '\0') && ((human->status == 2 || (human->status == 6)))) {
//             if (iVar6 < 1) {
//               sVar2 = 0x20;
//             }
//             else {
//               sVar2 = -0x20;
//             }
//             human->rotate->vy = human->rotate->vy + sVar2;
//             pMVar7 = human->motion->motion;
//             MoveHumanoid(human,(ushort)pMVar7->orderspd,(ushort)pMVar7->sidespd);
//           }
//           iVar4 = iVar5 >> 0xf;
//           iVar8 = iVar9 >> 0xf;
//         }
//         iVar4 = pVVar13->vx - iVar4;
//         iVar8 = pVVar13->vz - iVar8;
//       }
//       pVVar13->vx = iVar4;
//       pVVar13->vz = iVar8;
//     }
//   }
//   if (((int)model->attribute & 0x8000U) != 0) {
//     while( true ) {
//       sVar2 = GetConflictResult(model,-1);
//       uVar12 = (uint)sVar2;
//       if ((int)uVar12 < 0) break;
//       if ((Humanoid *)ConflictObject[uVar12].common != human) {
//         uVar1 = ConflictObject[uVar12].size.pad;
//         if ((uVar1 & 1) == 0) {
//           if (((uVar1 & 8) == 0) && ((human->attribute & 0x20U) == 0)) {
//             human->attribute = human->attribute | 0x8000;
//             iVar3 = pVVar13->vx;
//             lVar11 = pVVar13->vz;
//             if (ConflictDistance.vx < 0) {
//               iVar4 = -(int)human->width;
//             }
//             else {
//               iVar4 = (int)human->width;
//             }
//             if (iVar4 < 0) {
//               iVar4 = iVar4 + 7;
//             }
//             pVVar13->vx = iVar3 - (iVar4 >> 3);
//             if (ConflictDistance.vz < 0) {
//               iVar4 = -(int)human->width;
//             }
//             else {
//               iVar4 = (int)human->width;
//             }
//             if (iVar4 < 0) {
//               iVar4 = iVar4 + 7;
//             }
//             pVVar13->vz = pVVar13->vz - (iVar4 >> 3);
//             iVar10 = (int)ConflictObject[uVar12].size.vy;
//             iVar5 = ConflictObject[uVar12].position.vy;
//             iVar9 = ConflictObject[model->id].position.vy;
//             iVar4 = iVar5 - iVar10;
//             if (iVar9 < iVar4) {
//               pVVar13->vx = iVar3;
//               pVVar13->vz = lVar11;
//               if ((ConflictObject[uVar12].size.pad & 4U) == 0) {
//                 (human->vector).vy = 10;
//               }
//               else {
//                 (human->vector).vy = 0;
//                 pos->level = iVar4;
//                 pVVar13->vy = iVar4;
//                 (human->map).height = 0;
//                 (human->map).attrib = (human->map).attrib & 0xff80;
//                 human->attribute = human->attribute & 0x7eff;
//               }
//             }
//             else {
//               if ((iVar5 + iVar10 < iVar9) &&
//                  ((((sVar2 = human->status, sVar2 == 0 || (sVar2 == 2)) || (sVar2 == 5)) ||
//                   (sVar2 == 6)))) {
//                 sVar2 = GetDirection(ConflictObject[uVar12].position.vx - pVVar13->vx,
//                                      ConflictObject[uVar12].position.vz - pVVar13->vz,
//                                      (short)human->locate->vy);
//                 iVar3 = (int)sVar2;
//                 if (iVar3 < 0) {
//                   iVar3 = -iVar3;
//                 }
//                 sVar2 = 0x1003;
//                 if (iVar3 < 0x44c) {
//                   sVar2 = 0x1000;
//                 }
//                 SetNowMotion(human,sVar2,1);
//                 Sound(human,6);
//               }
//               if ((uVar12 & 1) == 0) {
//                 sVar2 = -human->turn;
//               }
//               else {
//                 sVar2 = human->turn;
//               }
//               human->rotate->vy = human->rotate->vy + sVar2;
//               if (((human->vector).vx != 0) || ((human->vector).vz != 0)) {
//                 pMVar7 = human->motion->motion;
//                 MoveHumanoid(human,(ushort)pMVar7->orderspd,(ushort)pMVar7->sidespd);
//                 if ((human->trace != (TraceLine *)0x0) && ((human->attribute & 8U) != 0)) {
//                   human->trace->count = -0x14;
//                 }
//               }
//             }
//           }
//         }
//         else if (human->status != 0x11) {
//           uVar1 = human->attribute;
//           (human->vector).pad = sVar2;
//           human->attribute = uVar1 | 0x4000;
//         }
//       }
//     }
//   }
//   return human->attribute;
// }
