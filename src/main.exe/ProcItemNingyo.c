#include "common.h"
#include "main.exe.h"
#include "item.h"

typedef struct
{
    ModelType *model;
    VECTOR position;
    SVECTOR offset;
    SVECTOR size;
    void *common;
    u8 result[64];
    u8 pad[0x10];
} ConflictObjectType;

typedef struct
{
    void *hint;
    s16 vx;
    s16 vy;
    s16 vz;
    u8 status;
    u8 pad;
    u8 count;
    u8 hp;
} param_ningyo;

typedef struct
{
    VECTOR TargetVector;
    Humanoid *Owner;
    s32 Mode;
    s16 DirectionRX;
    s16 DirectionRY;
    s32 OldMode;
    u8 Valiation;
} TCameraStatus;

typedef struct
{
    VECTOR v;
    VECTOR pos;
} ProcItemNingyoVectors;

typedef union
{
    struct
    {
        SVECTOR sv;
        PARAM_ITEM_USE launch;
    } drop;
    ProcItemNingyoVectors vectors;
} ProcItemNingyoScratch;

extern s16 Humans;
extern TCameraStatus CamState;
extern Humanoid *HumanGroup[];
extern ConflictObjectType ConflictObject[];
extern SVECTOR ConflictDistance;
extern SVECTOR D_80097AE4[];
extern u8 D_80097AE0;
extern ModelType *D_80097F50;

extern void MoveKorogari(tag_TItem *item, param_korogari *param);
extern s16 InsertConflict(ModelType *model);
extern s16 GetConflictResult(ModelType *model, s32 n);
extern s32 GetVectorDistance(VECTOR *v1, VECTOR *v2);
extern void SetBleeds(VECTOR *pos, short grange, short srange, short n,
                      int time, long col);
extern void DrawModel(ModelType *model);
extern MATRIX *RotMatrixYXZ(SVECTOR *r, MATRIX *m);
extern MATRIX *ScaleMatrix(MATRIX *m, VECTOR *v);

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcItemNingyo(struct tag_TItem *item);
 *     ITEM.C:1882, 132 src lines, frame 112 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s5       struct tag_TItem * item
 *     reg   $s4       struct param_ningyo * param
 *     reg   $a0       int cid
 *     reg   $s5       struct tag_TItem * item
 *     stack sp+24     struct SVECTOR sv
 *     stack sp+32     struct VECTOR v
 *     reg   $s5       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       struct ModelType * model
 *     reg   $s2       int i
 *     reg   $s0       struct Humanoid * human
 *     reg   $s1       long len
 *     reg   $v0       struct Humanoid * human
 *     reg   $v0       struct Humanoid * human
 *     reg   $a1       int i
 *     reg   $s5       struct tag_TItem * item
 *     stack sp+48     struct VECTOR pos
 *     reg   $s4       struct param_korogari * param
 *     reg   $s4       struct param_korogari * param
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct TCameraStatus CamState;
 *     extern struct Humanoid *HumanGroup[32];
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct SVECTOR ConflictDistance;
 * END PSX.SYM */

/* Matching checkpoint (retail): the pure-C draft has the exact 0x68 frame,
 * 564 instructions, exact 36/12/33/1 branch/jump/call/return inventory, and
 * target item/param/sentinel homes s3/s4/s5.  The remaining 41 differing
 * bytes are confined to the entry Humans-loop schedule, the decoy launch's
 * s0/s1/s2 rotation, and the mode-1 constant schedule.
 *
 * The otherwise-odd one-shot loops are measured allocator/scheduler fences:
 * the launch fence prevents a stack-address pseudo from occupying s3, while
 * the dispose fences keep item in the narrow priority window between param
 * and the later bounce temporary and allow the indirect call's target delay
 * slots.  They emit no branch or loop instructions. */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ProcItemNingyo", ProcItemNingyo);
#else
void ProcItemNingyo(tag_TItem *item)
{
    param_ningyo *param;
    s32 cid;
    s32 ff;
    ProcItemNingyoScratch scratch;

    param = (param_ningyo *)item->param;
    ff = 0xff;
    if (item->mode == ff)
    {
        if (param->hp != 99)
        {
            s32 i;
            s32 n;
            Humanoid **humans;

            n = Humans;
            if (n > 0)
            {
                s32 limit;

                humans = HumanGroup;
                i = 0;
                limit = n;
                do
                {
                    Humanoid *human;

                    human = *humans;
                    if (human->target == item->locate)
                    {
                        human->target = (ModelType *)CamState.Owner->model;
                    }
                    i++;
                    humans++;
                } while (i < limit);
            }
            D_80097AE0 = D_80097AE0 - 1;
        }
        item->mode = 0;
        return;
    }

    MoveKorogari(item, (param_korogari *)param);
    if (param->status == 1)
    {
        goto dispose;
    }

    switch (item->mode)
    {
    case 0:
    {
        s32 count;

        count = param->count - 1;
        param->count = count;
        if ((u8)count == 0)
        {
            param->count = 0;
            item->mode = item->mode + 1;
            scratch.drop.sv = D_80097AE4[0];
            SetSmoke((VECTOR *)item->locate->locate.coord.t,
                     &scratch.drop.sv, 10, 6);
            SoundEx((VECTOR *)item->locate->locate.coord.t, 0x23);
            if (D_80097AE0 < 3)
            {
                param->hp = 3;
                D_80097AE0 = D_80097AE0 + 1;
                goto draw_mode0;
            }
            else
            {
                Humanoid *owner;
                s32 type;
                ModelType *model;
                VECTOR *position;

                owner = item->owner;
                type = item->type;
                model = item->locate;
                memset((PARAM_ITEM_USE *)&scratch.vectors.v.vz, 0,
                       sizeof(PARAM_ITEM_USE));
                if (owner != 0)
                {
                    ((PARAM_ITEM_USE *)&scratch.vectors.v.vz)->type = type;
                }
                else
                {
                    ((PARAM_ITEM_USE *)&scratch.vectors.v.vz)->type = type;
                }
                ((PARAM_ITEM_USE *)&scratch.vectors.v.vz)->user = owner;
                position = (VECTOR *)model->locate.coord.t;
                ((PARAM_ITEM_USE *)&scratch.vectors.v.vz)->start.vx = position->vx;
                ((PARAM_ITEM_USE *)&scratch.vectors.v.vz)->start.vy = position->vy;
                ((PARAM_ITEM_USE *)&scratch.vectors.v.vz)->start.vz = position->vz;
                ((PARAM_ITEM_USE *)&scratch.vectors.v.vz)->end.vx = rand() % 200 - 100;
                ((PARAM_ITEM_USE *)&scratch.vectors.v.vz)->end.vy = rand() % 100 - 200;
                ((PARAM_ITEM_USE *)&scratch.vectors.v.vz)->end.vz = rand() % 200 - 100;
                ReqItemDrop((PARAM_ITEM_USE *)&scratch.vectors.v.vz);
            }
        }
        else
        {
            goto draw_mode0;
        }

dispose:
        do {
            do { do { do {
                if (item->proc == 0)
                {
                    return;
                }
            } while (0); } while (0); } while (0);
            item->mode = ff;
            item->proc(item);
        } while (0);
        DeleteConflict(item->locate);
        if (item->mode != 0)
        {
            AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
        }
        item->owner = 0;
        item->proc = 0;
        return;

draw_mode0:
        UpdateCoordinate(item->locate);
        item->model->locate = item->locate->locate;
        DrawSprite(item->model);
        return;
    }

    case 1:
    {
        s32 n;
        s32 size;
        s32 offset_y;
        s32 collision_mode;

        param->count = param->count + 1;
        memset(&scratch.vectors.pos, 0, sizeof(VECTOR));
        scratch.vectors.pos.vx = param->count << 8;
        scratch.vectors.pos.vy = param->count << 8;
        scratch.vectors.pos.vz = param->count << 8;
        scratch.vectors.v = scratch.vectors.pos;
        RotMatrixYXZ(&item->locate->rotate, &item->locate->locate.coord);
        ScaleMatrix(&item->locate->locate.coord, &scratch.vectors.v);
        item->locate->locate.flg = 0;
        D_80097F50->locate = item->locate->locate;
        DrawModel(D_80097F50);
        if (param->count < 16)
        {
            return;
        }

        DeleteConflict(item->locate);
        n = InsertConflict(item->locate);
        size = 500;
        offset_y = -250;
        collision_mode = 12;
        ConflictObject[n].common = (void *)1;
        ConflictObject[n].offset.vx = 0;
        ConflictObject[n].offset.vz = 0;
        ConflictObject[n].offset.vy = offset_y;
        ConflictObject[n].size.vz = size;
        ConflictObject[n].size.vy = size;
        ConflictObject[n].size.vx = size;
        ConflictObject[n].size.pad = collision_mode;
        item->coll_mode = collision_mode;
        item->coll_size = size;
        item->coll_ofsY = offset_y;
        item->coll_pause = 0;
        param->count = 3;
        item->mode = item->mode + 1;
        return;
    }

    case 2:
    {
        s32 count;

        if ((item->locate->attribute & 0x8000) == 0)
        {
            cid = -1;
        }
        else
        {
            cid = GetConflictResult(item->locate, -1);
        }

        count = param->count - 1;
        param->count = count;
        if ((u8)count == 0)
        {
            s32 i;
            Humanoid **humans;

            i = 0;
            humans = HumanGroup;
            while (1)
            {
                Humanoid *human;
                s32 len;

                if (!(i < Humans))
                {
                    break;
                }
                human = *humans;
                len = GetVectorDistance(
                    (VECTOR *)item->locate->locate.coord.t,
                    human->locate);
                if (len < 10000 && human->target != 0 &&
                    len < GetVectorDistance(
                              (VECTOR *)human->target->locate.coord.t,
                              human->locate) &&
                    ((u16)human->type & 0xf0) != 0x80)
                {
                    human->target = item->locate;
                }
                humans++;
                i++;
            }
            param->count = 30;
        }
        else if (cid != -1)
        {
            ConflictObjectType *conflict;
            ConflictObjectType *conflicts;
            s32 collision_mode;

            conflicts = ConflictObject;
            conflict = &conflicts[cid];
            collision_mode = conflict->size.pad;
            if (collision_mode == 1)
            {
                if (param->hp == 0)
                {
                    SetBleeds((VECTOR *)item->locate->locate.coord.t,
                              0, 30, 30, 30, 0xffff00);
                    SoundEx((VECTOR *)item->locate->locate.coord.t, 0x23);
                    if (item->proc != 0)
                    {
                        item->mode = 0xff;
                        item->proc(item);
                        DeleteConflict(item->locate);
                        if (item->mode != 0)
                        {
                            AdtMessageBox(D_800121CC, item->type,
                                          (u32)item->mode);
                        }
                        item->owner = 0;
                        item->proc = 0;
                    }
                    item->mode = item->mode + 1;
                }
                else
                {
                    s32 vz;
                    s32 vx;
                    s32 shifted_vx;
                    u8 hp;

                    memset(&scratch.vectors.pos, 0, sizeof(VECTOR));
                    scratch.vectors.pos.vx = conflict->position.vx;
                    scratch.vectors.pos.vy = conflict->position.vy;
                    scratch.vectors.pos.vz = conflict->position.vz;
                    scratch.vectors.v = scratch.vectors.pos;
                    vx = -ConflictDistance.vx;
                    do
                    {
                        if (vx < 0)
                        {
                            vx += 15;
                        }
                    } while (0);
                    shifted_vx = vx >> 4;
                    vz = -ConflictDistance.vz;
                    if (vz < 0)
                    {
                        vz += 15;
                    }
                    hp = param->hp;
                    param->vx = shifted_vx;
                    param->vy = -100;
                    param->vz = vz >> 4;
                    param->hint = 0;
                    param->status = 0;
                    param->hp = hp - 1;
                    SoundEx((VECTOR *)item->locate->locate.coord.t, 0x30);
                }
            }
            else if (collision_mode != 8)
            {
                s32 random_x;
                s32 random_z;
                s32 vx;
                s32 vy;
                s32 vz;
                s16 xbase;
                s16 xrem;

                random_x = rand();
                vx = -ConflictDistance.vx;
                if (vx < 0)
                {
                    vx += 7;
                }
                xbase = vx >> 3;
                xrem = random_x % 20;
                vy = 0;
                if (-501 < ConflictDistance.vy)
                {
                    vy = -100;
                }
                random_z = rand();
                vz = -ConflictDistance.vz;
                if (vz < 0)
                {
                    vz += 7;
                }
                param->vx = xbase + xrem - 10;
                param->vy = vy;
                param->hint = 0;
                param->status = 0;
                param->vz = (vz >> 3) + random_z % 20 - 10;
            }
        }

        UpdateCoordinate(item->locate);
        D_80097F50->locate = item->locate->locate;
        DrawModel(D_80097F50);
        return;
    }
    }
    return;
}
#endif

// triage: HARD — 564 insns, mul/div, 4 loop, indirect-call, 17 callees, ~0.16 to ProcItemHappou
// likely-relevant cookbook sections:
//   - Loops: 4 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Type propagation algorithm not settling */
//
// void ProcItemNingyo(tag_TItem *item)
//
// {
//   byte bVar1;
//   uchar uVar2;
//   short sVar3;
//   ModelType *pMVar4;
//   int iVar5;
//   int iVar6;
//   int iVar7;
//   ModelType *pMVar8;
//   SVECTOR *pSVar9;
//   int iVar10;
//   Humanoid **ppHVar11;
//   undefined4 uVar12;
//   undefined4 uVar13;
//   undefined4 uVar14;
//   TItemType TVar15;
//   Humanoid *pHVar16;
//   _202fake_6 *param;
//   undefined1 local_50 [12];
//   Humanoid *local_44;
//   long local_40;
//   long local_3c;
//   TItemType local_38;
//   Humanoid *local_34;
//   int local_30;
//   int local_2c;
//   int local_28;
//
//   param = &item->param;
//   if (item->mode == 0xff) {
//     if ((item->param).ningyo.hp != 'c') {
//       iVar7 = (int)Humans;
//       iVar10 = 0;
//       if (0 < iVar7) {
//         ppHVar11 = HumanGroup;
//         do {
//           if ((*ppHVar11)->target == item->locate) {
//             (*ppHVar11)->target = (ModelType *)(CamState.Owner)->model;
//           }
//           iVar10 = iVar10 + 1;
//           ppHVar11 = ppHVar11 + 1;
//         } while (iVar10 < iVar7);
//       }
//       DAT_80097ae0 = DAT_80097ae0 - 1;
//     }
//     item->mode = '\0';
//     return;
//   }
//   MoveKorogari(item,(param_korogari *)&param->launch);
//   if (*(char *)((int)&item->param + 10) == '\x01') {
// LAB_800423d8:
//     if (item->proc != (undefined **)0x0) {
//       item->mode = 0xff;
//       (*(code *)item->proc)(item);
//       DeleteConflict(item->locate);
//       if (item->mode != 0) {
//         AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//       }
//       item->owner = (Humanoid *)0x0;
//       item->proc = (undefined **)0x0;
//     }
//   }
//   else {
//     bVar1 = item->mode;
//     if (bVar1 == 1) {
//       (item->param).smoke.count = (item->param).smoke.count + '\x01';
//       memset((uchar *)&local_40,'\0',0x10);
//       local_50._0_4_ = (uint)(item->param).smoke.count << 8;
//       local_50._4_4_ = (uint)(item->param).smoke.count << 8;
//       local_50._8_4_ = (uint)(item->param).smoke.count << 8;
//       local_44 = local_34;
//       local_40 = local_50._0_4_;
//       local_3c = local_50._4_4_;
//       local_38 = local_50._8_4_;
//       RotMatrixYXZ(&item->locate->rotate,&(item->locate->locate).coord);
//       ScaleMatrix(&(item->locate->locate).coord,(VECTOR *)local_50);
//       (item->locate->locate).flg = 0;
//       pMVar8 = item->locate;
//       pSVar9 = &pMVar8->rotate;
//       pMVar4 = DAT_80097f50;
//       do {
//         uVar13 = *(undefined4 *)(pMVar8->locate).coord.m[0];
//         uVar14 = *(undefined4 *)((pMVar8->locate).coord.m[0] + 2);
//         uVar12 = *(undefined4 *)((pMVar8->locate).coord.m[1] + 1);
//         (pMVar4->locate).flg = (pMVar8->locate).flg;
//         *(undefined4 *)(pMVar4->locate).coord.m[0] = uVar13;
//         *(undefined4 *)((pMVar4->locate).coord.m[0] + 2) = uVar14;
//         *(undefined4 *)((pMVar4->locate).coord.m[1] + 1) = uVar12;
//         pMVar8 = (ModelType *)((pMVar8->locate).coord.m + 2);
//         pMVar4 = (ModelType *)((pMVar4->locate).coord.m + 2);
//       } while (pMVar8 != (ModelType *)pSVar9);
//       DrawModel(DAT_80097f50);
//       if ((item->param).smoke.count < 0x10) {
//         return;
//       }
//       DeleteConflict(item->locate);
//       sVar3 = InsertConflict(item->locate);
//       ConflictObject[sVar3].common = (undefined *)0x1;
//       ConflictObject[sVar3].offset.vx = 0;
//       ConflictObject[sVar3].offset.vz = 0;
//       ConflictObject[sVar3].offset.vy = -0xfa;
//       ConflictObject[sVar3].size.vz = 500;
//       ConflictObject[sVar3].size.vy = 500;
//       ConflictObject[sVar3].size.vx = 500;
//       ConflictObject[sVar3].size.pad = 0xc;
//       (item->collision).mode = 0xc;
//       (item->collision).size = 500;
//       (item->collision).ofsY = -0xfa;
//       (item->collision).pause = 0;
//       (item->param).smoke.count = '\x03';
//       item->mode = item->mode + '\x01';
//       return;
//     }
//     if (1 < bVar1) {
//       if (bVar1 != 2) {
//         return;
//       }
//       if (((int)item->locate->attribute & 0x8000U) == 0) {
//         iVar7 = -1;
//       }
//       else {
//         sVar3 = GetConflictResult(item->locate,-1);
//         iVar7 = (int)sVar3;
//       }
//       uVar2 = (item->param).smoke.count + 0xff;
//       (item->param).smoke.count = uVar2;
//       if (uVar2 == '\0') {
//         ppHVar11 = HumanGroup;
//         for (iVar7 = 0; iVar7 < Humans; iVar7 = iVar7 + 1) {
//           pHVar16 = *ppHVar11;
//           iVar10 = GetVectorDistance((VECTOR *)(item->locate->locate).coord.t,pHVar16->locate);
//           if ((((iVar10 < 10000) && (pHVar16->target != (ModelType *)0x0)) &&
//               (iVar5 = GetVectorDistance((VECTOR *)(pHVar16->target->locate).coord.t,pHVar16->locate
//                                         ), iVar10 < iVar5)) && ((pHVar16->type & 0xf0U) != 0x80)) {
//             pHVar16->target = item->locate;
//           }
//           ppHVar11 = ppHVar11 + 1;
//         }
//         (item->param).smoke.count = '\x1e';
//       }
//       else if (iVar7 != -1) {
//         sVar3 = ConflictObject[iVar7].size.pad;
//         if (sVar3 == 1) {
//           if ((item->param).ningyo.hp == '\0') {
//             SetBleeds((short)item->locate + 0x18,0,0x1e);
//             SoundEx((VECTOR *)(item->locate->locate).coord.t,0x23);
//             if (item->proc != (undefined **)0x0) {
//               item->mode = 0xff;
//               (*(code *)item->proc)(item);
//               DeleteConflict(item->locate);
//               if (item->mode != 0) {
//                 AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//               }
//               item->owner = (Humanoid *)0x0;
//               item->proc = (undefined **)0x0;
//             }
//             item->mode = item->mode + '\x01';
//           }
//           else {
//             memset((uchar *)&local_40,'\0',0x10);
//             local_50._0_4_ = ConflictObject[iVar7].position.vx;
//             local_50._4_4_ = ConflictObject[iVar7].position.vy;
//             local_50._8_4_ = ConflictObject[iVar7].position.vz;
//             iVar7 = -(int)ConflictDistance.vx;
//             local_44 = local_34;
//             if (iVar7 < 0) {
//               iVar7 = iVar7 + 0xf;
//             }
//             iVar10 = -(int)ConflictDistance.vz;
//             if (iVar10 < 0) {
//               iVar10 = iVar10 + 0xf;
//             }
//             uVar2 = (item->param).ningyo.hp;
//             (item->param).napalm.vec.vz = (short)(iVar7 >> 4);
//             (item->param).napalm.vec.pad = -100;
//             *(short *)((int)&item->param + 8) = (short)(iVar10 >> 4);
//             (param->smoke).koro.hint = (AreaNodeType *)0x0;
//             *(undefined1 *)((int)&item->param + 10) = 0;
//             (item->param).ningyo.hp = uVar2 + 0xff;
//             local_40 = local_50._0_4_;
//             local_3c = local_50._4_4_;
//             local_38 = local_50._8_4_;
//             SoundEx((VECTOR *)(item->locate->locate).coord.t,0x30);
//           }
//         }
//         else if (sVar3 != 8) {
//           iVar10 = rand();
//           iVar7 = -(int)ConflictDistance.vx;
//           if (iVar7 < 0) {
//             iVar7 = iVar7 + 7;
//           }
//           sVar3 = 0;
//           if (-0x1f5 < ConflictDistance.vy) {
//             sVar3 = -100;
//           }
//           iVar6 = rand();
//           iVar5 = -(int)ConflictDistance.vz;
//           if (iVar5 < 0) {
//             iVar5 = iVar5 + 7;
//           }
//           (item->param).napalm.vec.vz =
//                (short)(iVar7 >> 3) + (short)iVar10 + (short)(iVar10 / 0x14) * -0x14 + -10;
//           (item->param).napalm.vec.pad = sVar3;
//           (param->smoke).koro.hint = (AreaNodeType *)0x0;
//           *(undefined1 *)((int)&item->param + 10) = 0;
//           *(short *)((int)&item->param + 8) =
//                (short)(iVar5 >> 3) + (short)iVar6 + (short)(iVar6 / 0x14) * -0x14 + -10;
//         }
//       }
//       UpdateCoordinate(item->locate);
//       pMVar8 = item->locate;
//       pSVar9 = &pMVar8->rotate;
//       pMVar4 = DAT_80097f50;
//       do {
//         uVar13 = *(undefined4 *)(pMVar8->locate).coord.m[0];
//         uVar14 = *(undefined4 *)((pMVar8->locate).coord.m[0] + 2);
//         uVar12 = *(undefined4 *)((pMVar8->locate).coord.m[1] + 1);
//         (pMVar4->locate).flg = (pMVar8->locate).flg;
//         *(undefined4 *)(pMVar4->locate).coord.m[0] = uVar13;
//         *(undefined4 *)((pMVar4->locate).coord.m[0] + 2) = uVar14;
//         *(undefined4 *)((pMVar4->locate).coord.m[1] + 1) = uVar12;
//         pMVar8 = (ModelType *)((pMVar8->locate).coord.m + 2);
//         pMVar4 = (ModelType *)((pMVar4->locate).coord.m + 2);
//       } while (pMVar8 != (ModelType *)pSVar9);
//       DrawModel(DAT_80097f50);
//       return;
//     }
//     if (bVar1 != 0) {
//       return;
//     }
//     uVar2 = (item->param).smoke.count + 0xff;
//     (item->param).smoke.count = uVar2;
//     if (uVar2 == '\0') {
//       (item->param).smoke.count = '\0';
//       item->mode = item->mode + '\x01';
//       local_50._0_4_ = DAT_80097ae4;
//       local_50._4_4_ = DAT_80097ae8;
//       SetSmoke((VECTOR *)(item->locate->locate).coord.t,(SVECTOR *)local_50,10,6);
//       SoundEx((VECTOR *)(item->locate->locate).coord.t,0x23);
//       if (2 < DAT_80097ae0) {
//         pHVar16 = item->owner;
//         TVar15 = item->type;
//         pMVar4 = item->locate;
//         memset(local_50 + 8,'\0',0x28);
//         local_40 = (pMVar4->locate).coord.t[0];
//         local_3c = (pMVar4->locate).coord.t[1];
//         local_38 = (pMVar4->locate).coord.t[2];
//         local_50._8_4_ = TVar15;
//         local_44 = pHVar16;
//         iVar7 = rand();
//         local_30 = iVar7 % 200 + -100;
//         iVar7 = rand();
//         local_2c = iVar7 % 100 + -200;
//         iVar7 = rand();
//         local_28 = iVar7 % 200 + -100;
//         ReqItemDrop((PARAM_ITEM_USE *)(local_50 + 8));
//         goto LAB_800423d8;
//       }
//       (item->param).ningyo.hp = '\x03';
//       DAT_80097ae0 = DAT_80097ae0 + 1;
//     }
//     UpdateCoordinate(item->locate);
//     pMVar4 = item->locate;
//     pMVar8 = item->model;
//     pSVar9 = &pMVar4->rotate;
//     do {
//       uVar13 = *(undefined4 *)(pMVar4->locate).coord.m[0];
//       uVar14 = *(undefined4 *)((pMVar4->locate).coord.m[0] + 2);
//       uVar12 = *(undefined4 *)((pMVar4->locate).coord.m[1] + 1);
//       (pMVar8->locate).flg = (pMVar4->locate).flg;
//       *(undefined4 *)(pMVar8->locate).coord.m[0] = uVar13;
//       *(undefined4 *)((pMVar8->locate).coord.m[0] + 2) = uVar14;
//       *(undefined4 *)((pMVar8->locate).coord.m[1] + 1) = uVar12;
//       pMVar4 = (ModelType *)((pMVar4->locate).coord.m + 2);
//       pMVar8 = (ModelType *)((pMVar8->locate).coord.m + 2);
//     } while (pMVar4 != (ModelType *)pSVar9);
//     DrawSprite((Sprite3D *)item->model);
//   }
//   return;
// }
