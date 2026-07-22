#include "common.h"
#include "main.exe.h"

/*
 * ProcItemHappou (0x8004488c) — the happou (fire cracker / bouncing bomb)
 * item processor. Every frame: fly it (MoveFly), tick its countdown; at 0 register
 * a 300-unit conflict box. Always redraw (into the shared scratch model
 * HappouModel rather than item->model — the item's own visual is elsewhere)
 * and re-draw its afterimage trail. If the flight `mode` and rolling-overlay
 * `status` are armed, detonate (SetBleeds/SoundEx, dispose). Separately, if a
 * live character sits in the conflict box, SetImpact+SoundEx+DeleteConflict
 * (no dispose — this is the "someone touched it" hit, independent of the
 * detonation-armed path above).
 *
 * Matching notes (see also ProcItemMakibishi.c for the collision-box
 * conventions, ProcItemManebue.c for the countdown idiom):
 *  - `model = HappouModel; param = &item->param.launch;` are both
 *    computed before the entry mode==0xff test (model sequential, param's
 *    addiu fills the entry branch's delay slot).
 *  - The countdown is `t = param->count - 1;` (real `addiu -1`, sign-extended)
 *    tested on the NEW/post-decrement value — Manebue's idiom, NOT
 *    LightningBolt's `+0xff`-on-the-OLD-value idiom (verified against the
 *    raw immediate bytes in both places; per-function, not a fixed rule).
 *  - No entry `ff` local this time: the 0xff materialized for the entry
 *    test is a transient register, reused for unrelated things immediately
 *    after — nothing carries it to the later `item->mode = 0xff` dispose
 *    (a fresh literal there), so a plain literal at entry matches too.
 *  - The 300/1 collision-box constants are plain literals (no shared
 *    variable): unlike Makibishi's `item->mode += one`, nothing here adds
 *    them to anything (only stores), so there's no register-form-add tell
 *    forcing a variable.
 *  - `n = InsertConflict(...)` is `s32` (the same scheduling-tie fix as
 *    Makibishi/LightningBolt: extend right at the assignment).
 *  - The redundant-looking `mode != 0 && mode == 1 &&
 *    param->fly.p.koro.status != 0` is written
 *    exactly that way (three separate tests, matching Ghidra) — the asm
 *    shows two distinct branches on `mode` even though `== 1` implies `!= 0`.
 *  - The two dispose-shaped tails (detonate-and-dispose; the separate
 *    hit-conflict SetImpact path) are NOT the same code — no cross-jump
 *    merge here, they're independent, sequential ifs.
 */
#include "item.h"
#include "afterimage.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemHappou(struct tag_TItem *item);
 *     ITEM.C:2486, 52 src lines, frame 40 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     reg   $s2       struct ModelType * model
 *     reg   $s1       struct param_launch * param
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $s0       struct tag_TItem * item
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s1       struct Humanoid * m
 *     reg   $s1       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType *HappouModel;
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */

extern void MoveFly(tag_TItem *item, param_fly *param);
extern void DisposeAfterimage(AfterimageType *effect);
extern s16 DrawAfterimage(AfterimageType *effect, s16 flag);
extern void DrawModel(ModelType *m);
extern s16 InsertConflict(ModelType *m);
extern s16 GetConflictResult(ModelType *m, s32 n);
extern s32 is_character_state_present_on_stage_(Humanoid *h);
extern ConflictObjectType ConflictObject[];
/* Ghidra/m2c call this D_80097F54; bound here under a fresh name via
 * config/symbols.main.exe.txt because the data.s-internal `glabel
 * D_80097F54` (and its neighbor SyurikenModel, also referenced by
 * ReqItemLaunch) resolves 8 bytes low — a pre-existing accumulated-offset
 * drift in that data section, upstream of this function. Symbols bound
 * via config/symbols.main.exe.txt (LOCAL_COORDINATES_/NINKEN_CHARACTER_PTR,
 * right next to the drifted ones) resolve correctly regardless, which is
 * how this was diagnosed and is the same mechanism used to route around it. */
extern ModelType *HappouModel;

void ProcItemHappou(tag_TItem *item)
{
    ModelType *model;
    param_launch *param;
    u8 t;
    u8 mode;
    s32 i;
    s32 n;

    model = HappouModel;
    param = &item->param.launch;
    if (item->mode == 0xff)
    {
        DisposeAfterimage(param->effect);
        item->mode = 0;
        return;
    }
    MoveFly(item, &param->fly);
    t = param->count - 1;
    param->count = t;
    if (t == 0)
    {
        DeleteConflict(item->locate);
        n = InsertConflict(item->locate);
        ConflictObject[n].offset.vx = 0;
        ConflictObject[n].offset.vz = 0;
        ConflictObject[n].offset.vy = 0;
        ConflictObject[n].size.vz = 300;
        ConflictObject[n].size.vy = 300;
        ConflictObject[n].size.vx = 300;
        ConflictObject[n].common = (void *)1;
        ConflictObject[n].size.pad = 1;
        item->coll_size = 300;
        item->coll_ofsY = 0;
        item->coll_mode = 1;
        item->coll_pause = 0;
    }
    UpdateCoordinate(item->locate);
    model->locate = item->locate->locate;
    DrawModel(model);
    DrawAfterimage(param->effect, 1);
    mode = param->fly.mode;
    if (mode != 0)
    {
        if (mode == 1 && param->fly.p.koro.status != 0)
        {
            SetBleeds((VECTOR *)item->locate->locate.coord.t, 0, 0x19, 0xa, 0xa, 0xffff00);
            SoundEx((VECTOR *)item->locate->locate.coord.t, 0x31);
            if (item->proc != 0)
            {
                item->mode = 0xff;
                item->proc(item);
                DeleteConflict(item->locate);
                if (item->mode != 0)
                {
                    AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
                }
                item->owner = 0;
                item->proc = 0;
            }
        }
    }
    if ((item->locate->attribute & 0x8000) == 0)
        i = -1;
    else
        i = GetConflictResult(item->locate, -1);
    if (i != -1 && is_character_state_present_on_stage_(ConflictObject[i].common) != 0)
    {
        SetImpact((VECTOR *)item->locate->locate.coord.t, 0x4000, 2);
        SoundEx((VECTOR *)item->locate->locate.coord.t, 0x30);
        DeleteConflict(item->locate);
    }
}

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ProcItemHappou(tag_TItem *item)
//
// {
//   char cVar1;
//   ModelType *objp;
//   uchar uVar2;
//   short sVar3;
//   ModelType *pMVar4;
//   ModelType *pMVar5;
//   SVECTOR *pSVar6;
//   int iVar7;
//   undefined4 uVar8;
//   undefined4 uVar9;
//   undefined4 uVar10;
//
//   objp = DAT_80097f54;
//   if (item->mode == 0xff) {
//     DisposeAfterimage((item->param).launch.effect);
//     item->mode = '\0';
//   }
//   else {
//     MoveFly((int *)item,(int *)&(item->param).henshin);
//     uVar2 = (item->param).launch.count + 0xff;
//     (item->param).launch.count = uVar2;
//     if (uVar2 == '\0') {
//       DeleteConflict(item->locate);
//       sVar3 = InsertConflict(item->locate);
//       ConflictObject[sVar3].offset.vx = 0;
//       ConflictObject[sVar3].offset.vz = 0;
//       ConflictObject[sVar3].offset.vy = 0;
//       ConflictObject[sVar3].size.vz = 300;
//       ConflictObject[sVar3].size.vy = 300;
//       ConflictObject[sVar3].size.vx = 300;
//       ConflictObject[sVar3].common = (undefined *)0x1;
//       ConflictObject[sVar3].size.pad = 1;
//       (item->collision).size = 300;
//       (item->collision).ofsY = 0;
//       (item->collision).mode = 1;
//       (item->collision).pause = 0;
//     }
//     UpdateCoordinate(item->locate);
//     pMVar4 = item->locate;
//     pSVar6 = &pMVar4->rotate;
//     pMVar5 = objp;
//     do {
//       uVar8 = *(undefined4 *)(pMVar4->locate).coord.m[0];
//       uVar9 = *(undefined4 *)((pMVar4->locate).coord.m[0] + 2);
//       uVar10 = *(undefined4 *)((pMVar4->locate).coord.m[1] + 1);
//       (pMVar5->locate).flg = (pMVar4->locate).flg;
//       *(undefined4 *)(pMVar5->locate).coord.m[0] = uVar8;
//       *(undefined4 *)((pMVar5->locate).coord.m[0] + 2) = uVar9;
//       *(undefined4 *)((pMVar5->locate).coord.m[1] + 1) = uVar10;
//       pMVar4 = (ModelType *)((pMVar4->locate).coord.m + 2);
//       pMVar5 = (ModelType *)((pMVar5->locate).coord.m + 2);
//     } while (pMVar4 != (ModelType *)pSVar6);
//     DrawModel(objp);
//     DrawAfterimage((item->param).launch.effect,1);
//     cVar1 = *(char *)((int)&item->param + 0x28);
//     if (((cVar1 != '\0') && (cVar1 == '\x01')) && (*(char *)((int)&item->param + 10) != '\0')) {
//       SetBleeds((short)item->locate + 0x18,0,0x19);
//       SoundEx((VECTOR *)(item->locate->locate).coord.t,0x31);
//       if (item->proc != (undefined **)0x0) {
//         item->mode = 0xff;
//         (*(code *)item->proc)(item);
//         DeleteConflict(item->locate);
//         if (item->mode != 0) {
//           AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//         }
//         item->owner = (Humanoid *)0x0;
//         item->proc = (undefined **)0x0;
//       }
//     }
//     if (((int)item->locate->attribute & 0x8000U) == 0) {
//       iVar7 = -1;
//     }
//     else {
//       sVar3 = GetConflictResult(item->locate,-1);
//       iVar7 = (int)sVar3;
//     }
//     if ((iVar7 != -1) && (iVar7 = FUN_800294dc(ConflictObject[iVar7].common), iVar7 != 0)) {
//       SetImpact((VECTOR *)(item->locate->locate).coord.t,0x4000,2);
//       SoundEx((VECTOR *)(item->locate->locate).coord.t,0x30);
//       DeleteConflict(item->locate);
//     }
//   }
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? AdtMessageBox(? *, s32, u8);                      /* extern */
// ? DeleteConflict(void *);                           /* extern */
// ? DisposeAfterimage(s32);                           /* extern */
// ? DrawAfterimage(s32, ?);                           /* extern */
// ? DrawModel(void *);                                /* extern */
// s16 GetConflictResult(void *, ?);                   /* extern */
// s16 InsertConflict(void *);                         /* extern */
// ? MoveFly(void *, void *);                          /* extern */
// ? SetBleeds(void *, ?, ?, s32, s32, s32);           /* extern */
// ? SetImpact(void *, ?, ?);                          /* extern */
// ? SoundEx(void *, ?);                               /* extern */
// ? UpdateCoordinate(void *);                         /* extern */
// s32 is_character_state_present_on_stage_(s32);      /* extern */
// extern ? D_800121CC;
// extern void *HappouModel;
// extern ? ConflictObject;
//
// void ProcItemHappou(void *arg0) {
//     ? (*temp_v0_2)(void *);
//     s16 var_a0;
//     u8 temp_v0;
//     u8 temp_v0_3;
//     u8 temp_v1_2;
//     void *temp_a0;
//     void *temp_a0_2;
//     void *temp_s1;
//     void *temp_s2;
//     void *temp_v1;
//     void *var_v0;
//     void *var_v1;
//
//     temp_s2 = HappouModel;
//     temp_s1 = arg0 + 0x20;
//     if (arg0->unk54 == 0xFF) {
//         DisposeAfterimage(temp_s1->unk2C);
//         arg0->unk54 = 0U;
//         return;
//     }
//     MoveFly(arg0, temp_s1);
//     temp_v0 = temp_s1->unk30 - 1;
//     temp_s1->unk30 = temp_v0;
//     if (!(temp_v0 & 0xFF)) {
//         DeleteConflict(arg0->unk10);
//         temp_v1 = (InsertConflict(arg0->unk10) * 0x78) + &ConflictObject;
//         temp_v1->unk14 = 0;
//         temp_v1->unk18 = 0;
//         temp_v1->unk16 = 0;
//         temp_v1->unk20 = 0x12C;
//         temp_v1->unk1E = 0x12C;
//         temp_v1->unk1C = 0x12C;
//         temp_v1->unk24 = 1;
//         temp_v1->unk22 = (s16) 1;
//         arg0->unk1C = 0x12C;
//         arg0->unk1E = 0;
//         arg0->unk14 = 1;
//         arg0->unk18 = 0;
//     }
//     UpdateCoordinate(arg0->unk10);
//     var_v0 = arg0->unk10;
//     var_v1 = temp_s2;
//     temp_a0 = var_v0 + 0x50;
//     do {
//         var_v1->unk0 = (s32) var_v0->unk0;
//         var_v1->unk4 = (s32) var_v0->unk4;
//         var_v1->unk8 = (s32) var_v0->unk8;
//         var_v1->unkC = (s32) var_v0->unkC;
//         var_v0 += 0x10;
//         var_v1 += 0x10;
//     } while (var_v0 != temp_a0);
//     DrawModel(temp_s2);
//     DrawAfterimage(temp_s1->unk2C, 1);
//     temp_v1_2 = temp_s1->unk28;
//     if ((temp_v1_2 != 0) && (temp_v1_2 == 1) && (temp_s1->unkA != 0)) {
//         SetBleeds(arg0->unk10 + 0x18, 0, 0x19, 0xA, 0xA, 0xFFFF00);
//         SoundEx(arg0->unk10 + 0x18, 0x31);
//         temp_v0_2 = arg0->unkC;
//         if (temp_v0_2 != NULL) {
//             arg0->unk54 = 0xFFU;
//             temp_v0_2(arg0);
//             DeleteConflict(arg0->unk10);
//             temp_v0_3 = arg0->unk54;
//             if (temp_v0_3 != 0) {
//                 AdtMessageBox(&D_800121CC, arg0->unk8, temp_v0_3);
//             }
//             arg0->unk0 = 0;
//             arg0->unkC = NULL;
//         }
//     }
//     temp_a0_2 = arg0->unk10;
//     if (!(temp_a0_2->unk5A & 0x8000)) {
//         var_a0 = -1;
//     } else {
//         var_a0 = GetConflictResult(temp_a0_2, -1);
//     }
//     if ((var_a0 != -1) && (is_character_state_present_on_stage_(((var_a0 * 0x78) + &ConflictObject)->unk24) != 0)) {
//         SetImpact(arg0->unk10 + 0x18, 0x4000, 2);
//         SoundEx(arg0->unk10 + 0x18, 0x30);
//         DeleteConflict(arg0->unk10);
//     }
// }
