#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcItemHenshin(struct tag_TItem *item);
 *     ITEM.C:2060, 140 src lines, frame 208 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
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
 *     param $s3       struct tag_TItem * item
 *     reg   $s4       struct param_henshin * param
 *     reg   $a1       int i
 *     reg   $s1       struct ModelArchiveType * mad
 *     reg   $a2       struct ModelArchiveType * hen
 *     stack sp+16     struct ModelArchiveType *[30] target
 *     reg   $s2       int targets
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s2       struct Humanoid * human
 *     reg   $s0       int itemID
 *     stack sp+136    struct PARAM_ITEM_LAUNCH p
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s3       struct tag_TItem * item
 *     stack sp+136    struct SVECTOR sv
 *     stack sp+144    struct SVECTOR sv
 *     reg   $s3       struct tag_TItem * item
 * END PSX.SYM */

#include "item.h"

/*
 * MATCH.
 *
 * ProcItemHenshin (0x80042c1c) owns the disguise transformation.  It starts
 * and monitors motion 0xf04, drops itself when that motion is interrupted,
 * swaps the owner's model-object data to/from two saved snapshots, and tears
 * down the previous disguise before installing a new one.
 *
 * Matching notes:
 *  - The retail snapshot begins with a four-byte archive-rotation header,
 *    followed by ordinary 12-byte model-part records. Indexing the nested
 *    `part` array by the loop's own counter gives loop.c one unbiased
 *    induction pointer and the target's natural +4/+8/+10/+12 offsets.
 *  - D_80097AEC and D_80097AF0 use volatile views only to preserve the
 *    original observable load/store sequence.  In particular, the restore
 *    path stores the current-disguise pointer before reloading item->owner,
 *    and mode 2 finishes its mode/count stores before loading owner/type.
 *    The old disguise pointer is copied once before its null/proc checks so
 *    volatility does not introduce redundant global reloads.
 *  - `buf` is the exact sp+0x10..0x37 scratch overlay: a PARAM_ITEM_LAUNCH on
 *    the interrupted-motion path and an unaligned SVECTOR aggregate copy on
 *    both smoke paths.
 *  - Case 0 deliberately does not assign D_80097AEC.  It jumps directly to
 *    the shared mode increment; only the completed mode-1 path installs the
 *    current item after disposing any prior disguise.
 */
extern tag_TItem *volatile D_80097AEC;
extern volatile u16 D_80097AF0;
extern SVECTOR D_80097AF4[];
extern HenshinModelSnapshot D_800C0630;
extern HenshinModelSnapshot D_800C06F0;

extern s16 SetNowMotion(Humanoid *human, s16 mid, s16 move);
extern void Sound(Humanoid *human, s32 sound);
extern s16 NowReturnNormal(Humanoid *human);

void ProcItemHenshin(tag_TItem *item)
{
    Humanoid *human;
    ModelArchiveType *mad;
    u8 ff;
    u8 buf[sizeof(PARAM_ITEM_LAUNCH)];

    human = item->owner;
    mad = human->model;
    ff = 0xff;

    if (item->mode == ff)
    {
        if (item == D_80097AEC)
        {
            s32 i;
            HenshinModelSnapshot *saved;

            i = 0;
            saved = &D_800C0630;
            mad->rotate.pad = (s16)saved->rotate_pad;
            if (mad->n > 0)
            {
                do
                {
                    mad->object[i]->object.tmd = saved->part[i].tmd;
                    mad->object[i]->locate.coord.t[0] =
                        saved->part[i].position.vx;
                    mad->object[i]->locate.coord.t[1] =
                        saved->part[i].position.vy;
                    mad->object[i]->locate.coord.t[2] =
                        saved->part[i].position.vz;
                    i++;
                } while (i < mad->n);
            }
            if (item->owner->status == 0xb)
            {
                NowReturnNormal(item->owner);
            }
            D_80097AEC = 0;
            ((volatile tag_TItem *)item)->owner->active_item = 0;
        }
        item->mode = 0;
        return;
    }

    switch (item->mode)
    {
    case 0:
        SetNowMotion(human, 0xf04, 1);
        Sound(item->owner, 0x4c);
        item->mode = item->mode + 1;
        return;

    case 1:
    {
        MotionManager *motion;

        motion = human->motion;
        if (motion->mid != 0xf04)
        {
            VECTOR *pos;
            Humanoid *drop_owner;
            s32 itemID;

            pos = GetAbsolutePosition(item->locate, 0, 0, 0);
            drop_owner = item->owner;
            itemID = item->type;
            memset(buf, 0, sizeof(PARAM_ITEM_LAUNCH));
            ((PARAM_ITEM_LAUNCH *)buf)->type = itemID;
            ((PARAM_ITEM_LAUNCH *)buf)->user = drop_owner;
            ((PARAM_ITEM_LAUNCH *)buf)->start.vx = pos->vx;
            ((PARAM_ITEM_LAUNCH *)buf)->start.vy = pos->vy;
            ((PARAM_ITEM_LAUNCH *)buf)->start.vz = pos->vz;
            ((PARAM_ITEM_LAUNCH *)buf)->end.vx = rand() % 200 - 100;
            ((PARAM_ITEM_LAUNCH *)buf)->end.vy = rand() % 100 - 200;
            ((PARAM_ITEM_LAUNCH *)buf)->end.vz = rand() % 200 - 100;
            ReqItemDrop((PARAM_ITEM_LAUNCH *)buf);
            if (item->proc == 0)
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
        if (motion->count != 0)
        {
            return;
        }
        if (motion->loop == 0)
        {
            return;
        }

        NowReturnNormal(human);
        *(SVECTOR *)buf = D_80097AF4[0];
        SetSmoke((VECTOR *)mad->locate.coord.t, (SVECTOR *)buf, 10, 6);
        {
            tag_TItem *old;

            old = D_80097AEC;
            if (old != 0 && old->proc != 0)
            {
                old->mode = ff;
                old->proc(old);
                DeleteConflict(old->locate);
                if (old->mode != 0)
                {
                    AdtMessageBox(D_800121CC, old->type, (u32)old->mode);
                }
                old->owner = 0;
                old->proc = 0;
            }
        }
        D_80097AEC = item;
        item->mode = item->mode + 1;
        return;
    }

    case 2:
    {
        s32 i;
        HenshinModelSnapshot *saved;
        volatile tag_TItem *vitem;
        Humanoid *mode_owner;
        u16 itemID;

        i = 0;
        saved = &D_800C06F0;
        mad->rotate.pad = (s16)saved->rotate_pad;
        if (mad->n > 0)
        {
            do
            {
                mad->object[i]->object.tmd = saved->part[i].tmd;
                mad->object[i]->locate.coord.t[0] =
                    saved->part[i].position.vx;
                mad->object[i]->locate.coord.t[1] =
                    saved->part[i].position.vy;
                mad->object[i]->locate.coord.t[2] =
                    saved->part[i].position.vz;
                i++;
            } while (i < mad->n);
        }
        vitem = item;
        vitem->mode = vitem->mode + 1;
        D_80097AF0 = 600;
        mode_owner = vitem->owner;
        itemID = *(volatile u16 *)&vitem->type;
        FRAMES_UNTIL_END_OF_ALERT = -600;
        mode_owner->active_item = itemID;
        return;
    }

    case 3:
    {
        u16 count;

        count = D_80097AF0 - 1;
        D_80097AF0 = count;
        if ((s32)(count << 16) > 0 &&
            item->owner->active_item == item->type &&
            item->owner->status != 0x10 &&
            item->owner->status != 0x11)
        {
            if (item->owner->status != 7)
            {
                return;
            }
            if (item->owner->motion->loop >= 0 &&
                item->owner->motion->mid < 0x714)
            {
                return;
            }
        }
        *(SVECTOR *)buf = D_80097AF4[0];
        SetSmoke((VECTOR *)mad->locate.coord.t, (SVECTOR *)buf, 10, 6);
        if (item->proc == 0)
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
    }
}

// Historical Ghidra decompilation reference:
//
//
// void ProcItemHenshin(tag_TItem *item)
//
// {
//   undefined ***pppuVar1;
//   byte bVar2;
//   short sVar3;
//   tag_TItem *ptVar4;
//   tag_TItem *ptVar5;
//   undefined2 *puVar6;
//   VECTOR *pVVar7;
//   undefined **ppuVar8;
//   Humanoid *pHVar9;
//   ModelType **ppMVar10;
//   MotionManager *pMVar11;
//   int iVar12;
//   ModelArchiveType *pMVar13;
//   TItemType TVar14;
//   PARAM_ITEM_LAUNCH local_40;
//
//   pHVar9 = item->owner;
//   pMVar13 = pHVar9->model;
//   if (item->mode == 0xff) {
//     if (item == DAT_80097aec) {
//       iVar12 = 0;
//       (pMVar13->rotate).pad = DAT_800c0630;
//       puVar6 = &DAT_800c0630;
//       if (0 < pMVar13->n) {
//         do {
//           (pMVar13->object[iVar12]->object).tmd = *(ulong **)(puVar6 + 2);
//           (pMVar13->object[iVar12]->locate).coord.t[0] = (int)(short)puVar6[4];
//           (pMVar13->object[iVar12]->locate).coord.t[1] = (int)(short)puVar6[5];
//           ppMVar10 = pMVar13->object + iVar12;
//           iVar12 = iVar12 + 1;
//           ((*ppMVar10)->locate).coord.t[2] = (int)(short)puVar6[6];
//           puVar6 = puVar6 + 6;
//         } while (iVar12 < pMVar13->n);
//       }
//       if (item->owner->status == 0xb) {
//         NowReturnNormal(item->owner);
//       }
//       DAT_80097aec = (tag_TItem *)0x0;
//       item->owner->itmctl = 0;
//     }
//     item->mode = '\0';
//     return;
//   }
//   bVar2 = item->mode;
//   if (bVar2 == 1) {
//     pMVar11 = pHVar9->motion;
//     if (pMVar11->mid != 0xf04) {
//       pVVar7 = GetAbsolutePosition(item->locate,0,0,0);
//       pHVar9 = item->owner;
//       TVar14 = item->type;
//       memset((uchar *)&local_40,'\0',0x28);
//       local_40.start.vx = pVVar7->vx;
//       local_40.start.vy = pVVar7->vy;
//       local_40.start.vz = pVVar7->vz;
//       local_40.type = TVar14;
//       local_40.user = pHVar9;
//       iVar12 = rand();
//       local_40.end.vx = iVar12 % 200 + -100;
//       iVar12 = rand();
//       local_40.end.vy = iVar12 % 100 + -200;
//       iVar12 = rand();
//       local_40.end.vz = iVar12 % 200 + -100;
//       ReqItemDrop(&local_40);
//       ppuVar8 = item->proc;
//       if (ppuVar8 == (undefined **)0x0) {
//         return;
//       }
//       item->mode = 0xff;
//       goto LAB_8004316c;
//     }
//     if (pMVar11->count != 0) {
//       return;
//     }
//     if (pMVar11->loop == 0) {
//       return;
//     }
//     NowReturnNormal(pHVar9);
//     local_40.type = DAT_80097af4;
//     local_40.user = DAT_80097af8;
//     SetSmoke((VECTOR *)(pMVar13->locate).coord.t,(SVECTOR *)&local_40,10,6);
//     ptVar4 = DAT_80097aec;
//     ptVar5 = item;
//     if ((DAT_80097aec != (tag_TItem *)0x0) &&
//        (pppuVar1 = &DAT_80097aec->proc, ptVar5 = item, *pppuVar1 != (undefined **)0x0)) {
//       DAT_80097aec->mode = 0xff;
//       (*(code *)*pppuVar1)(ptVar4);
//       DeleteConflict(ptVar4->locate);
//       if (ptVar4->mode != 0) {
//         AdtMessageBox("item dispose fail   id %d  mode %d",ptVar4->type,(uint)ptVar4->mode);
//       }
//       ptVar4->owner = (Humanoid *)0x0;
//       ptVar4->proc = (undefined **)0x0;
//       ptVar5 = item;
//     }
//   }
//   else {
//     if (1 < bVar2) {
//       if (bVar2 == 2) {
//         iVar12 = 0;
//         (pMVar13->rotate).pad = DAT_800c06f0;
//         puVar6 = &DAT_800c06f0;
//         if (0 < pMVar13->n) {
//           do {
//             (pMVar13->object[iVar12]->object).tmd = *(ulong **)(puVar6 + 2);
//             (pMVar13->object[iVar12]->locate).coord.t[0] = (int)(short)puVar6[4];
//             (pMVar13->object[iVar12]->locate).coord.t[1] = (int)(short)puVar6[5];
//             ppMVar10 = pMVar13->object + iVar12;
//             iVar12 = iVar12 + 1;
//             ((*ppMVar10)->locate).coord.t[2] = (int)(short)puVar6[6];
//             puVar6 = puVar6 + 6;
//           } while (iVar12 < pMVar13->n);
//         }
//         item->mode = item->mode + '\x01';
//         DAT_80097af0 = 600;
//         DAT_800979c0 = 0xfffffda8;
//         item->owner->itmctl = (short)item->type;
//         return;
//       }
//       if (bVar2 != 3) {
//         return;
//       }
//       DAT_80097af0 = DAT_80097af0 - 1;
//       if ((((0 < (int)((uint)DAT_80097af0 << 0x10)) &&
//            (pHVar9 = item->owner, (int)pHVar9->itmctl == item->type)) &&
//           (sVar3 = pHVar9->status, sVar3 != 0x10)) && (sVar3 != 0x11)) {
//         if (sVar3 != 7) {
//           return;
//         }
//         if ((-1 < pHVar9->motion->loop) && (pHVar9->motion->mid < 0x714)) {
//           return;
//         }
//       }
//       local_40.type = DAT_80097af4;
//       local_40.user = DAT_80097af8;
//       SetSmoke((VECTOR *)(pMVar13->locate).coord.t,(SVECTOR *)&local_40,10,6);
//       ppuVar8 = item->proc;
//       if (ppuVar8 == (undefined **)0x0) {
//         return;
//       }
//       item->mode = 0xff;
// LAB_8004316c:
//       (*(code *)ppuVar8)(item);
//       DeleteConflict(item->locate);
//       if (item->mode != 0) {
//         AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//       }
//       item->owner = (Humanoid *)0x0;
//       item->proc = (undefined **)0x0;
//       return;
//     }
//     if (bVar2 != 0) {
//       return;
//     }
//     SetNowMotion(pHVar9,0xf04,1);
//     Sound(item->owner,0x4c);
//     ptVar5 = DAT_80097aec;
//   }
//   DAT_80097aec = ptVar5;
//   item->mode = item->mode + '\x01';
//   return;
// }
