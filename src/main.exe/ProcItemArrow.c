#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemArrow(struct tag_TItem *item);
 *     ITEM.C:3367, 118 src lines, frame 168 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     reg   $s4       struct ModelType * model
 *     reg   $s3       struct param_arrow * param
 *     stack sp+24     struct VECTOR v1
 *     stack sp+40     struct VECTOR v2
 *     stack sp+136    int rx
 *     stack sp+140    int ry
 *     reg   $a0       int cid
 *     reg   $s1       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       struct ModelType * model
 *     reg   $s2       struct Humanoid * m
 *     reg   $s2       struct Humanoid * human
 *     stack sp+56     struct PARAM_ITEM_LAUNCH param
 *     reg   $s1       struct tag_TItem * item
 *     reg   $s0       struct ModelType * model
 *     reg   $s1       struct tag_TItem * item
 *     reg   $s1       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern long GameClock;
 * END PSX.SYM */

#include "item.h"

/*
 * MATCH.
 *
 * ProcItemArrow (0x80047b94) flies, arms, attaches, and renders the homing
 * arrow.  Before attachment it maintains a 300-unit conflict box and aims
 * from the previous to current position; a character hit either disposes
 * the arrow or attaches it to a random model object.  Its final two modes
 * blink the attached arrow before disposal.
 *
 * Matching notes:
 *  - `v1`, `v2`, `rx`, and `ry` use the stack slots recovered by PSX.SYM.
 *    GetVectorRotation writes the two full-word outputs at sp+0x88/sp+0x8c;
 *    their later stores to SVECTOR members naturally use only the low halves.
 *  - The dead `mode_index = 0` assignment is a zero-code CSE eviction.  It
 *    forces expand_case to emit a fresh mode `lbu`; otherwise the entry
 *    guard's load is reused and the function is one instruction short.
 *  - `ff` is an s32 caller-saved value in a1.  It survives only the no-call
 *    mode-2 path; call-crossing mode-0 disposal prefixes rematerialize
 *    ITEM_MODE_DISPOSE (0xff)
 *    before all copies merge at the common indirect-call tail.
 *  - The payload guard is explicit control flow so both zero tests target
 *    the later aiming block.  `water` is deliberately s16: autorules
 *    found that narrowing this comparison-only constant colors the payload
 *    byte into v1 and `1` into v0, while still allowing the `li` to fill the
 *    payload-zero branch's delay slot.  s32 swaps those two registers.
 *  - `clock` prevents the halfword-load optimization on GameClock.  The
 *    original reads the declared long with `lw`, shifts it, then narrows at
 *    the model rotation store.
 *  - The target model-object cursor is a `ModelType **`: loop-free pointer
 *    adjustment after the guarded random remainder reproduces the single
 *    object-array load and the checked variable-division sequence.
 *  - `objp->locate = item->locate->locate` intentionally remains a whole
 *    GsCOORDINATE2 assignment.  GCC emits the target five-iteration,
 *    16-byte block-copy loop before DrawModel.
 */
extern void MoveFly(TItem *item, param_fly *param);
extern short DrawModel(ModelType *objp);
extern s32 is_character_state_present_on_stage_(Humanoid *human);
extern void ArrangeLocalMatrix(ModelType *model, MATRIX *t);

void ProcItemArrow(TItem *item)
{
    ModelType *objp;
    param_arrow *param;
    void (*ppu)(TItem *);
    s32 ff;
    u8 mode_index;
    VECTOR v1;
    VECTOR v2;
    int rx;
    int ry;

    objp = item->model;
    param = &item->param.arrow;
    ff = ITEM_MODE_DISPOSE;
    mode_index = item->mode;
    if (mode_index == ff)
    {
        item->mode = 0;
        return;
    }

    mode_index = 0;
    switch (item->mode)
    {
    case 0:
    {
        u8 count;
        s32 cid;
        s32 one;

        v1.vx = item->locate->locate.coord.t[0];
        v1.vy = item->locate->locate.coord.t[1];
        v1.vz = item->locate->locate.coord.t[2];
        MoveFly(item, &param->fly);
        count = param->count - 1;
        param->count = count;
        one = 1;
        if (count == 0)
        {
            s32 n;
            s32 size;

            DeleteConflict(item->locate);
            n = InsertConflict(item->locate);
            size = 300;
            ConflictObject[n].offset.vx = 0;
            ConflictObject[n].offset.vz = 0;
            ConflictObject[n].offset.vy = 0;
            ConflictObject[n].size.vz = size;
            ConflictObject[n].size.vy = size;
            ConflictObject[n].size.vx = size;
            ConflictObject[n].common = (void *)one;
            ConflictObject[n].size.pad = one;
            item->collision.size = size;
            item->collision.ofsY = 0;
            item->collision.mode = one;
            item->collision.pause = 0;
        }

        if ((item->locate->attribute & 0x8000) == 0)
        {
            cid = -1;
        }
        else
        {
            cid = GetConflictResult(item->locate, -1);
        }
        if (cid != -1)
        {
            Humanoid *human;

            human = (Humanoid *)ConflictObject[cid].common;
            if (is_character_state_present_on_stage_(human) != 0)
            {
                if ((ConflictObject[cid].size.pad & 1) != 0)
                {
                    ppu = item->proc;
                    if (ppu == 0)
                    {
                        return;
                    }
                    item->mode = ITEM_MODE_DISPOSE;
                    item->proc(item);
                    DeleteConflict(item->locate);
                    if (item->mode != 0)
                    {
                        AdtMessageBox(D_800121CC, item->type,
                                      (u32)item->mode);
                    }
                    item->owner = 0;
                    item->proc = 0;
                    return;
                }
                else
                {
                    ModelType **models;
                    ModelType *model;

                    models = human->model->object;
                    if (human->model->n > 0)
                    {
                        models += rand() % human->model->n;
                    }
                    model = *models;
                    SetImpact(GetAbsolutePosition(model, 0, 0, 0),
                              0x6000, 2);
                    SoundEx(GetAbsolutePosition(model, 0, 0, 0), 0x30);
                    ArrangeLocalMatrix(model,
                                       &item->locate->locate.coord);
                    item->locate->locate.flg = 0;
                    item->locate->locate.super =
                        (GsCOORDINATE2 *)model;
                    item->locate->locate.coord.t[0] = 0;
                    item->locate->locate.coord.t[1] = 0;
                    item->locate->locate.coord.t[2] = 0;
                    param->count = 0x78;
                    item->mode = item->mode + 1;
                    DeleteConflict(item->locate);
                    break;
                }
            }
            else
            {
                goto aim;
            }
        }

        {
            s16 water;
            u8 kind;

            if (param->fly.mode == 0)
            {
                goto aim;
            }
            water = KORO_WATER;
            kind = param->fly.p.koro.status;
            if (kind == KORO_NORMAL)
            {
                goto aim;
            }
            if (kind == water)
            {
                ppu = item->proc;
                if (ppu == 0)
                {
                    return;
                }
                item->mode = ITEM_MODE_DISPOSE;
                item->proc(item);
                DeleteConflict(item->locate);
                if (item->mode != 0)
                {
                    AdtMessageBox(D_800121CC, item->type,
                                  (u32)item->mode);
                }
                item->owner = 0;
                item->proc = 0;
                return;
            }
            SoundEx((VECTOR *)item->locate->locate.coord.t, 0x31);
            SetBleeds((VECTOR *)item->locate->locate.coord.t,
                      0, 0x19, 0x1e, 0x1e, 0xffff00);
            param->count = 0x1e;
            item->mode = item->mode + 1;
            DeleteConflict(item->locate);
            return;
        }

aim:
        v2.vx = item->locate->locate.coord.t[0];
        v2.vy = item->locate->locate.coord.t[1];
        v2.vz = item->locate->locate.coord.t[2];
        GetVectorRotation(&v1, &v2, &rx, &ry);
        item->locate->rotate.vx = rx;
        item->locate->rotate.vy = ry;
        {
            s32 clock;

            clock = GameClock;
            item->locate->rotate.vz = clock << 8;
        }
        UpdateCoordinate(item->locate);
        break;
    }

    case 1:
    {
        u8 count;

        count = param->count - 1;
        param->count = count;
        if (count != 0)
        {
            break;
        }
        param->count = 0xf;
        item->mode = item->mode + 1;
        break;
    }

    case 2:
    {
        u8 count;

        count = param->count - 1;
        param->count = count;
        if (count == 0)
        {
            ppu = item->proc;
            if (ppu == 0)
            {
                return;
            }
            item->mode = ff;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != 0)
            {
                AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
            return;
        }
        if ((count & 1) != 0)
        {
            return;
        }
        break;
    }
    }

    objp->locate = item->locate->locate;
    DrawModel(objp);
}

// Historical Ghidra decompilation reference:
//
//
// void ProcItemArrow(tag_TItem *item)
//
// {
//   char cVar1;
//   uchar uVar2;
//   short sVar3;
//   int iVar4;
//   undefined **ppuVar5;
//   VECTOR *pVVar6;
//   byte bVar7;
//   SVECTOR *pSVar8;
//   int iVar9;
//   ModelType *pMVar10;
//   undefined4 uVar11;
//   undefined4 uVar12;
//   undefined4 uVar13;
//   undefined4 *puVar14;
//   ModelType *pMVar15;
//   undefined *puVar16;
//   ModelType *objp;
//   VECTOR local_40;
//   VECTOR local_30;
//   short local_20 [2];
//   short local_1c [2];
//
//   objp = item->model;
//   if (item->mode == 0xff) {
//     item->mode = '\0';
//     return;
//   }
//   bVar7 = item->mode;
//   if (bVar7 == 1) {
//     uVar2 = (item->param).arrow.count + 0xff;
//     (item->param).arrow.count = uVar2;
//     if (uVar2 == '\0') {
//       (item->param).arrow.count = '\x0f';
//       item->mode = item->mode + '\x01';
//     }
//   }
//   else if (bVar7 < 2) {
//     if (bVar7 == 0) {
//       local_40.vx = (item->locate->locate).coord.t[0];
//       local_40.vy = (item->locate->locate).coord.t[1];
//       local_40.vz = (item->locate->locate).coord.t[2];
//       MoveFly((int *)item,(int *)&(item->param).henshin);
//       uVar2 = (item->param).arrow.count + 0xff;
//       (item->param).arrow.count = uVar2;
//       if (uVar2 == '\0') {
//         DeleteConflict(item->locate);
//         sVar3 = InsertConflict(item->locate);
//         ConflictObject[sVar3].offset.vx = 0;
//         ConflictObject[sVar3].offset.vz = 0;
//         ConflictObject[sVar3].offset.vy = 0;
//         ConflictObject[sVar3].size.vz = 300;
//         ConflictObject[sVar3].size.vy = 300;
//         ConflictObject[sVar3].size.vx = 300;
//         ConflictObject[sVar3].common = (undefined *)0x1;
//         ConflictObject[sVar3].size.pad = 1;
//         (item->collision).size = 300;
//         (item->collision).ofsY = 0;
//         (item->collision).mode = 1;
//         (item->collision).pause = 0;
//       }
//       if (((int)item->locate->attribute & 0x8000U) == 0) {
//         iVar9 = -1;
//       }
//       else {
//         sVar3 = GetConflictResult(item->locate,-1);
//         iVar9 = (int)sVar3;
//       }
//       if (iVar9 == -1) {
//         if ((*(char *)((int)&item->param + 0x28) != '\0') &&
//            (cVar1 = *(char *)((int)&item->param + 10), cVar1 != '\0')) {
//           if (cVar1 != '\x01') {
//             SoundEx((VECTOR *)(item->locate->locate).coord.t,0x31);
//             SetBleeds((short)item->locate + 0x18,0,0x19);
//             (item->param).arrow.count = '\x1e';
//             item->mode = item->mode + '\x01';
//             DeleteConflict(item->locate);
//             return;
//           }
//           ppuVar5 = item->proc;
//           if (ppuVar5 == (undefined **)0x0) {
//             return;
//           }
//           item->mode = 0xff;
// LAB_80048018:
//           (*(code *)ppuVar5)(item);
//           DeleteConflict(item->locate);
//           if (item->mode != 0) {
//             AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//           }
//           item->owner = (Humanoid *)0x0;
//           item->proc = (undefined **)0x0;
//           return;
//         }
//       }
//       else {
//         puVar16 = ConflictObject[iVar9].common;
//         iVar4 = FUN_800294dc(puVar16);
//         if (iVar4 != 0) {
//           if ((ConflictObject[iVar9].size.pad & 1U) != 0) {
//             ppuVar5 = item->proc;
//             if (ppuVar5 == (undefined **)0x0) {
//               return;
//             }
//             item->mode = 0xff;
//             goto LAB_80048018;
//           }
//           puVar14 = *(undefined4 **)(*(int *)(puVar16 + 0x58) + 0x68);
//           if (0 < *(short *)(*(int *)(puVar16 + 0x58) + 100)) {
//             iVar9 = rand();
//             iVar4 = (int)*(short *)(*(int *)(puVar16 + 0x58) + 100);
//             if (iVar4 == 0) {
//               trap(0x1c00);
//             }
//             if ((iVar4 == -1) && (iVar9 == -0x80000000)) {
//               trap(0x1800);
//             }
//             puVar14 = puVar14 + iVar9 % iVar4;
//           }
//           pMVar15 = (ModelType *)*puVar14;
//           pVVar6 = GetAbsolutePosition(pMVar15,0,0,0);
//           SetImpact(pVVar6,0x6000,2);
//           pVVar6 = GetAbsolutePosition(pMVar15,0,0,0);
//           SoundEx(pVVar6,0x30);
//           ArrangeLocalMatrix(pMVar15,&(item->locate->locate).coord);
//           (item->locate->locate).flg = 0;
//           (item->locate->locate).super = (_GsCOORDINATE2 *)pMVar15;
//           (item->locate->locate).coord.t[0] = 0;
//           (item->locate->locate).coord.t[1] = 0;
//           (item->locate->locate).coord.t[2] = 0;
//           (item->param).arrow.count = 'x';
//           item->mode = item->mode + '\x01';
//           DeleteConflict(item->locate);
//           goto LAB_80048064;
//         }
//       }
//       local_30.vx = (item->locate->locate).coord.t[0];
//       local_30.vy = (item->locate->locate).coord.t[1];
//       local_30.vz = (item->locate->locate).coord.t[2];
//       GetVectorRotation(&local_40,&local_30,(int *)local_20,(int *)local_1c);
//       (item->locate->rotate).vx = local_20[0];
//       (item->locate->rotate).vy = local_1c[0];
//       (item->locate->rotate).vz = (short)(GameClock << 8);
//       UpdateCoordinate(item->locate);
//     }
//   }
//   else if (bVar7 == 2) {
//     uVar2 = (item->param).arrow.count;
//     bVar7 = uVar2 - 1;
//     (item->param).arrow.count = bVar7;
//     if (uVar2 == '\x01') {
//       ppuVar5 = item->proc;
//       if (ppuVar5 == (undefined **)0x0) {
//         return;
//       }
//       item->mode = 0xff;
//       goto LAB_80048018;
//     }
//     if ((bVar7 & 1) != 0) {
//       return;
//     }
//   }
// LAB_80048064:
//   pMVar10 = item->locate;
//   pSVar8 = &pMVar10->rotate;
//   pMVar15 = objp;
//   do {
//     uVar11 = *(undefined4 *)(pMVar10->locate).coord.m[0];
//     uVar12 = *(undefined4 *)((pMVar10->locate).coord.m[0] + 2);
//     uVar13 = *(undefined4 *)((pMVar10->locate).coord.m[1] + 1);
//     (pMVar15->locate).flg = (pMVar10->locate).flg;
//     *(undefined4 *)(pMVar15->locate).coord.m[0] = uVar11;
//     *(undefined4 *)((pMVar15->locate).coord.m[0] + 2) = uVar12;
//     *(undefined4 *)((pMVar15->locate).coord.m[1] + 1) = uVar13;
//     pMVar10 = (ModelType *)((pMVar10->locate).coord.m + 2);
//     pMVar15 = (ModelType *)((pMVar15->locate).coord.m + 2);
//   } while (pMVar10 != (ModelType *)pSVar8);
//   DrawModel(objp);
//   return;
// }
