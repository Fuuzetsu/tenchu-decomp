#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcKaginawa(struct tag_TItem *item);
 *     ITEM.C:1026, 67 src lines, frame 40 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     param $s0       struct tag_TItem * item
 *     stack sp+16     int rx
 *     stack sp+20     int ry
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v0       int dist
 *     reg   $s0       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct GsSPRITE TargetSprite[1];
 *     extern struct GsOT *OTablePt;
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

#include "item.h"

/*
 * ProcKaginawa (0x8003eea4) — the grappling-hook (kaginawa) item processor.
 * Three-way dispatch on the owner's hook state, all converging on the shared
 * ProcItem dispose tail:
 *  - owner->hookflag (Humanoid+0xCD) == 0: not yet thrown — enter DIRECTION
 *    camera mode and dispose.
 *  - owner is mid-throw but not in the hook-fly motion (motion->mid != 0x400):
 *    dispose.
 *  - owner IS in the hook-fly motion (mid == 0x400): aim the camera along the
 *    throw (GetVectorRotation off ViewInfo), and if the sight bit is held just
 *    re-sort the reticle sprite and bail; otherwise walk the camera target
 *    toward the hook tip (RotateVector a fixed offset, FUN_80039ddc a step,
 *    accumulate into CamState.TargetVector), snap onto the owner's model once
 *    close enough or the pitch turned upward, drop into LOCK camera mode,
 *    clear the hook flag, and dispose.
 *
 * Matching notes (see ProcItemTeleport.c / ProcItemKusuri.c for the item-TU
 * conventions this shares):
 *  - `ff = 0xff` is a callee-saved var ($s1) tested at entry and reused as
 *    `item->mode = ff` in the first two dispose blocks; in the big third
 *    block $s1 has been repurposed for the ViewInfo/CamState addresses, so
 *    the same `ff` variable is rematerialised as a fresh `li 0xff` there.
 *  - `own = item->owner` is ONE load feeding both the hookflag test and the
 *    motion->mid test (caller-saved $v1, dies at the first call); the big
 *    block reloads item->owner for its pad-bit test and hookflag clear.
 *  - The dispose tail is written out in all three branches; jump2's
 *    cross-jump merges the identical `jalr`-onward suffix into one shared
 *    tail after the third branch (the mode-store instruction differs — $s1
 *    vs rematerialised $v1 — so the merge starts at the call, not earlier).
 *  - The hook flag lives at Humanoid+0xCD, i.e. `own->item[0x19]` one past
 *    the DoInfoViewProc-indexed slots (item.h sizes item[] to 0x1A to cover
 *    it); read `lbu`, written `sb 0`.
 *  - `w.vx = v.vx; …; w.vx += ViewInfo.vpx; …` is the two-phase raw-copy-
 *    then-add the target stores twice per field (a single `w.vx = v.vx +
 *    ViewInfo.vpx` would store once); v/w are separate VECTOR locals.
 *  - `v = D_80012238;` is a plain extern VECTOR struct assignment — under
 *    -msplit-addresses the 16-byte (non-small) source address builds as a
 *    two-register lui/addiu pair and the four words copy through it.
 */
#include <psxsdk/libgs.h>

typedef struct
{
    s32 vpx, vpy, vpz;           /* 0x00 */
    s32 vrx, vry, vrz;           /* 0x0C */
} TViewInfo;

typedef struct
{
    VECTOR TargetVector;         /* 0x00 */
    Humanoid *Owner;             /* 0x10 */
    s32 Mode;                    /* 0x14 */
    s16 DirectionRX;             /* 0x18 */
    s16 DirectionRY;             /* 0x1A */
    s32 OldMode;                 /* 0x1C */
    u8 Valiation;                /* 0x20 */
} TCameraStatus;

extern void GetVectorRotation(VECTOR *start, VECTOR *end, s32 *rx, s32 *ry);
extern void RotateVector(VECTOR *vec, int rx, int ry, int rz);
extern s32 FUN_80039ddc(VECTOR *from, VECTOR *to, VECTOR *out, u32 flag);
extern s32 GetVectorDistance(VECTOR *a, VECTOR *b);
extern void SetCameraMode(int mode);
extern void GsSortSprite(GsSPRITE *sp, GsOT *ot, int pri);
extern GsOT *OTablePt;
extern GsSPRITE TargetSprite;
extern TCameraStatus CamState;
extern TViewInfo ViewInfo;
extern VECTOR D_80012238;

void ProcKaginawa(tag_TItem *item)
{
    void (*ppu)(tag_TItem *);
    Humanoid *own;
    VECTOR v;
    VECTOR w;
    s32 rx, ry;
    s32 dist;
    s32 tx, ty, tz;
    u8 ff;

    ff = 0xff;
    if (item->mode == ff)
    {
        item->mode = 0;
        return;
    }
    own = item->owner;
    if (own->item[0x19] == 0)
    {
        SetCameraMode(1);
        ppu = item->proc;
        if (ppu == 0)
            return;
        item->mode = ff;
        item->proc(item);
        DeleteConflict(item->locate);
        if (item->mode != 0)
            AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
        item->owner = 0;
        item->proc = 0;
    }
    else if (own->motion->mid != 0x400)
    {
        ppu = item->proc;
        if (ppu == 0)
            return;
        item->mode = ff;
        item->proc(item);
        DeleteConflict(item->locate);
        if (item->mode != 0)
            AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
        item->owner = 0;
        item->proc = 0;
    }
    else
    {
        GetVectorRotation((VECTOR *)&ViewInfo, (VECTOR *)&ViewInfo.vrx, &rx, &ry);
        if (item->owner->pad.data & 0x10)
        {
            if (rx < 0)
                GsSortSprite(&TargetSprite, OTablePt, 0);
            return;
        }
        v = D_80012238;
        RotateVector(&v, rx, ry, 0);
        w.vx = v.vx;
        w.vy = v.vy;
        w.vz = v.vz;
        w.vx += ViewInfo.vpx;
        w.vy += ViewInfo.vpy;
        w.vz += ViewInfo.vpz;
        FUN_80039ddc((VECTOR *)&ViewInfo, &w, (VECTOR *)&CamState, 0);
        tx = v.vx;
        if (tx < 0)
            tx += 0xF;
        v.vx = tx >> 4;
        ty = v.vy;
        if (ty < 0)
            ty += 0xF;
        v.vy = ty >> 4;
        tz = v.vz;
        if (tz < 0)
            tz += 0xF;
        v.vz = tz >> 4;
        CamState.TargetVector.vx += v.vx;
        CamState.TargetVector.vy += v.vy;
        CamState.TargetVector.vz += v.vz;
        dist = GetVectorDistance((VECTOR *)CamState.Owner->model->locate.coord.t, &CamState.TargetVector);
        if (rx > 0 || dist > 15000)
        {
            CamState.TargetVector = *(VECTOR *)CamState.Owner->model->locate.coord.t;
        }
        SetCameraMode(0xD);
        item->owner->item[0x19] = 0;
        ppu = item->proc;
        if (ppu == 0)
            return;
        item->mode = ff;
        item->proc(item);
        DeleteConflict(item->locate);
        if (item->mode != 0)
            AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
        item->owner = 0;
        item->proc = 0;
    }
}

// triage: MEDIUM — 186 insns, indirect-call, 8 callees, ~0.20 to ProcItemTeleport
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ProcKaginawa(tag_TItem *item)
//
// {
//   undefined **ppuVar1;
//   int iVar2;
//   ModelArchiveType *pMVar3;
//   VECTOR local_40;
//   int local_30;
//   int local_2c;
//   int local_28;
//   int local_20;
//   int local_1c;
//
//   if (item->mode == 0xff) {
//     item->mode = '\0';
//     return;
//   }
//   if (item->owner->field_0xcd == '\0') {
//     SetCameraMode(CMODE_DIRECTION);
//     ppuVar1 = item->proc;
//     if (ppuVar1 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
//   }
//   else if (item->owner->motion->mid == 0x400) {
//     GetVectorRotation((VECTOR *)&ViewInfo,(VECTOR *)&ViewInfo.vrx,&local_20,&local_1c);
//     if (((item->owner->pad).data & 0x10) != 0) {
//       if (-1 < local_20) {
//         return;
//       }
//       GsSortSprite(TargetSprite,OTablePt,0);
//       return;
//     }
//     local_40.vx = DAT_80012238;
//     local_40.vy = DAT_8001223c;
//     local_40.vz = DAT_80012240;
//     local_40.pad = DAT_80012244;
//     RotateVector(&local_40,local_20,local_1c,0);
//     local_30 = local_40.vx + ViewInfo.vpx;
//     local_2c = local_40.vy + ViewInfo.vpy;
//     local_28 = local_40.vz + ViewInfo.vpz;
//     FUN_80039ddc(&ViewInfo,&local_30,&CamState,0);
//     if (local_40.vx < 0) {
//       local_40.vx = local_40.vx + 0xf;
//     }
//     local_40.vx = local_40.vx >> 4;
//     if (local_40.vy < 0) {
//       local_40.vy = local_40.vy + 0xf;
//     }
//     local_40.vy = local_40.vy >> 4;
//     if (local_40.vz < 0) {
//       local_40.vz = local_40.vz + 0xf;
//     }
//     local_40.vz = local_40.vz >> 4;
//     CamState.TargetVector.vx = CamState.TargetVector.vx + local_40.vx;
//     CamState.TargetVector.vy = CamState.TargetVector.vy + local_40.vy;
//     CamState.TargetVector.vz = CamState.TargetVector.vz + local_40.vz;
//     iVar2 = GetVectorDistance((VECTOR *)((CamState.Owner)->model->locate).coord.t,
//                               &CamState.TargetVector);
//     if ((0 < local_20) || (15000 < iVar2)) {
//       pMVar3 = (CamState.Owner)->model;
//       CamState.TargetVector.vx = (pMVar3->locate).coord.t[0];
//       CamState.TargetVector.vy = (pMVar3->locate).coord.t[1];
//       CamState.TargetVector.vz = (pMVar3->locate).coord.t[2];
//       CamState.TargetVector.pad = *(long *)(pMVar3->locate).workm.m[0];
//     }
//     SetCameraMode(CMODE_LOCK);
//     item->owner->field_0xcd = 0;
//     ppuVar1 = item->proc;
//     if (ppuVar1 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
//   }
//   else {
//     ppuVar1 = item->proc;
//     if (ppuVar1 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
//   }
//   (*(code *)ppuVar1)(item);
//   DeleteConflict(item->locate);
//   if (item->mode != 0) {
//     AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//   }
//   item->owner = (Humanoid *)0x0;
//   item->proc = (undefined **)0x0;
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? AdtMessageBox(? *, s32, u8);                      /* extern */
// ? DeleteConflict(s32);                              /* extern */
// ? FUN_80039ddc(? *, s32 *, ? *, ?);                 /* extern */
// s32 GetVectorDistance(s32, ? *, s32);               /* extern */
// ? GetVectorRotation(? *, void *, s32 *, s32 *);     /* extern */
// ? GsSortSprite(? *, s32, ?);                        /* extern */
// ? RotateVector(s32 *, s32, s32, ?);                 /* extern */
// ? SetCameraMode(?);                                 /* extern */
// extern ? CamState;
// extern ? D_800121CC;
// extern ? D_80012238;
// extern s32 OTablePt;
// extern ? TargetSprite;
// extern ? ViewInfo;
//
// void ProcKaginawa(void *arg0) {
//     s32 sp10;
//     s32 sp14;
//     s32 sp18;
//     s32 sp1C;
//     s32 sp20;
//     s32 sp24;
//     s32 sp28;
//     s32 sp30;
//     s32 sp34;
//     s32 temp_a0;
//     s32 temp_a1;
//     s32 temp_a2;
//     s32 temp_v0;
//     s32 var_a0;
//     s32 var_v0;
//     s32 var_v1;
//     u8 temp_v0_3;
//     void *temp_v0_2;
//     void *temp_v1;
//
//     if (arg0->unk54 == 0xFF) {
//         arg0->unk54 = 0U;
//         return;
//     }
//     temp_v1 = arg0->unk0;
//     if (temp_v1->unkCD == 0) {
//         SetCameraMode(1);
//         if (arg0->unkC != NULL) {
//             arg0->unk54 = 0xFFU;
//             goto block_22;
//         }
//     } else if (*temp_v1->unk5C != 0x400) {
//         if (arg0->unkC != NULL) {
//             arg0->unk54 = 0xFFU;
//             goto block_22;
//         }
//     } else {
//         GetVectorRotation(&ViewInfo, &ViewInfo + 0xC, &sp30, &sp34);
//         if (arg0->unk0->unk10 & 0x10) {
//             if (sp30 < 0) {
//                 GsSortSprite(&TargetSprite, OTablePt, 0);
//             }
//         } else {
//             sp10 = D_80012238.unk0;
//             sp14 = D_80012238.unk4;
//             sp18 = D_80012238.unk8;
//             sp1C = D_80012238.unkC;
//             RotateVector(&sp10, sp30, sp34, 0);
//             sp20 = sp10;
//             sp24 = sp14;
//             sp28 = sp18;
//             sp20 = sp10 + ViewInfo.unk0;
//             sp24 = sp14 + ViewInfo.unk4;
//             sp28 = sp18 + ViewInfo.unk8;
//             FUN_80039ddc(&ViewInfo, &sp20, &CamState, 0);
//             var_v0 = sp10;
//             if (var_v0 < 0) {
//                 var_v0 += 0xF;
//             }
//             var_v1 = sp14;
//             temp_a2 = var_v0 >> 4;
//             sp10 = temp_a2;
//             if (var_v1 < 0) {
//                 var_v1 += 0xF;
//             }
//             var_a0 = sp18;
//             temp_a1 = var_v1 >> 4;
//             sp14 = temp_a1;
//             if (var_a0 < 0) {
//                 var_a0 += 0xF;
//             }
//             temp_a0 = var_a0 >> 4;
//             sp18 = temp_a0;
//             CamState.unk0 = (s32) (CamState.unk0 + temp_a2);
//             CamState.unk4 = (s32) (CamState.unk4 + temp_a1);
//             CamState.unk8 = (s32) (CamState.unk8 + temp_a0);
//             temp_v0 = GetVectorDistance(CamState.unk10->unk58 + 0x18, &CamState, temp_a2) < 0x3A99;
//             if ((sp30 > 0) || (temp_v0 == 0)) {
//                 temp_v0_2 = CamState.unk10->unk58;
//                 CamState.unk0 = (s32) temp_v0_2->unk18;
//                 CamState.unk4 = (s32) temp_v0_2->unk1C;
//                 CamState.unk8 = (s32) temp_v0_2->unk20;
//                 CamState.unkC = (s32) temp_v0_2->unk24;
//             }
//             SetCameraMode(0xD);
//             arg0->unk0->unkCD = 0U;
//             if (arg0->unkC != NULL) {
//                 arg0->unk54 = 0xFFU;
// block_22:
//                 arg0->unkC(arg0);
//                 DeleteConflict(arg0->unk10);
//                 temp_v0_3 = arg0->unk54;
//                 if (temp_v0_3 != 0) {
//                     AdtMessageBox(&D_800121CC, arg0->unk8, temp_v0_3);
//                 }
//                 arg0->unk0 = NULL;
//                 arg0->unkC = NULL;
//             }
//         }
//     }
// }
