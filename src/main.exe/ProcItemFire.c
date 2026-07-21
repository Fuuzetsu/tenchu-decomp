#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * ProcItemFire (0x80044de0) — rolls and draws the fire item, emits a small
 * random bleed every frame, occasionally converts an exhausted item back into
 * a pickup, expands its collision volume when lit, and attaches a frame effect
 * to a collided character until the countdown expires.
 *
 * Matching notes:
 *  - The mutually-exclusive particle/drop/explosion/frame aggregates share the
 *    explicit ProcItemFireScratch union.  Its views reproduce the retail
 *    sp+0x18..sp+0x77 overlay and the exact 0x98-byte frame; ordinary block
 *    locals made cc1 reserve an extra 24 bytes.
 *  - The three rand()%50 expressions use separate single-definition return
 *    temps plus `base_* = coordinate - 25`.  Reusing one rand temp leaves
 *    copies from $v0; inlining the calls lets combine reassociate `-25` with
 *    the remainder and removes each target coordinate-load hazard nop.
 *  - `count` is full-width, with explicit `(u8)` tests.  That keeps the
 *    decrement in one SI pseudo ($v1) and emits the target narrowing at each
 *    switch path; an `u8` local was one instruction short and needed a copy.
 *  - The pickup conversion intentionally mixes pointer and direct spellings:
 *    `launch->user` and ReqItemDrop retain the launch pointer in $s0, while
 *    direct aggregate fields preserve the target stack-relative loads/stores.
 *    The saved-position pointer supplies the sequential source loads.
 *  - Named full-width `size`/`collision_mode` values share 500 and 8 across
 *    halfword and word stores.  Separate literals materialize 8 twice.
 *  - This TU needs maspsx `--expand-div` for the dynamic model-count remainder
 *    guard; Build.hs and permute.py carry the mirrored per-function setting.
 */

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemFire(struct tag_TItem *item);
 *     ITEM.C:2586, 116 src lines, frame 176 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
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
 *     param $s2       struct tag_TItem * item
 *     reg   $s4       struct Sprite3D * model
 *     reg   $s5       struct param_smoke * param
 *     reg   $s2       struct tag_TItem * item
 *     stack sp+24     struct VECTOR pos
 *     stack sp+40     struct SVECTOR vec
 *     stack sp+56     struct PARAM_ITEM_STAY rparam
 *     reg   $s2       struct tag_TItem * item
 *     stack sp+104    struct PARAM_ITEM_LAUNCH param
 *     reg   $a0       int cid
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       struct ModelType * model
 *     reg   $v0       struct Humanoid * human
 *     stack sp+48     struct SVECTOR vec
 *     stack sp+80     struct VECTOR pos
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $s2       struct tag_TItem * item
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s0       struct Humanoid * human
 *     reg   $s0       struct Humanoid * human
 *     reg   $s1       struct ModelType * model
 *     stack sp+96     struct VECTOR pos
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */

/* The retail stack frame overlays the mutually-exclusive mode temporaries.
 * Keep that layout explicit: all four views begin at sp+0x18 and the largest
 * one ends immediately before the saved-register area at sp+0x78. */
typedef union
{
    struct
    {
        VECTOR pos;
        VECTOR random_pos;
    } particle;
    struct
    {
        PARAM_ITEM_STAY saved;
        u8 pad0[12];
        PARAM_ITEM_STAY rparam;
        u8 pad1[4];
        PARAM_ITEM_USE launch;
    } drop;
    struct
    {
        SVECTOR vec;
        VECTOR pos;
        VECTOR pos_buf;
    } explosion;
    struct
    {
        VECTOR pos;
        VECTOR random_pos;
    } frame;
} ProcItemFireScratch;

extern ConflictObjectType ConflictObject[];
extern u_long *GlobalAreaMap;
extern SVECTOR D_80097AE4[];
extern SVECTOR D_80097AFC[];

extern void MoveKorogari(tag_TItem *item, param_korogari *param);
extern s16 InsertConflict(ModelType *model);
extern s16 GetConflictResult(ModelType *model, s32 n);
extern s32 is_character_state_present_on_stage_(Humanoid *human);
extern long GetAreaMapLevel(u_long *area, long x, long y, long z, int mode);
extern void SetExplosion(VECTOR *pos, SVECTOR *vec);
extern void SetHinoko(VECTOR *pos, SVECTOR *vec, s32 n);
extern void SetSmokeS(VECTOR *pos, short vx, short vy, short vz, int time);
extern void SetFrame(VECTOR *pos, short size, short time,
                     GsCOORDINATE2 *super);
extern void reset_alert_duration(void);

void ProcItemFire(tag_TItem *item)
{
    Sprite3D *sprt;
    param_smoke *param;
    u8 ff;
    s32 count;
    s32 mode;
    s32 one;
    s32 cid;
    ProcItemFireScratch scratch;

    sprt = item->model;
    param = (param_smoke *)item->param;
    ff = 0xff;
    if (item->mode == ff)
    {
        item->mode = 0;
        return;
    }

    MoveKorogari(item, &param->koro);
    if (param->koro.status == 1)
    {
        if (item->proc != 0)
        {
            item->mode = ff;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != 0)
            {
                AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
        }
        return;
    }

    UpdateCoordinate(item->locate);
    sprt->locate = item->locate->locate;
    DrawSprite(sprt);

    {
        VECTOR *position;
        SVECTOR *vec;
        s32 random_x;
        s32 random_y;
        s32 random_z;
        s32 base_x;
        s32 base_y;
        s32 base_z;

        vec = (SVECTOR *)&scratch.particle.random_pos;
        memset(&scratch.particle.random_pos, 0, sizeof(VECTOR));
        random_x = rand();
        base_x = item->locate->locate.coord.t[0] - 25;
        scratch.particle.random_pos.vx = base_x + random_x % 50;
        random_y = rand();
        base_y = item->locate->locate.coord.t[1] - 25;
        scratch.particle.random_pos.vy = base_y + random_y % 50;
        random_z = rand();
        base_z = item->locate->locate.coord.t[2] - 25;
        scratch.particle.random_pos.vz = base_z + random_z % 50;
        scratch.particle.pos = scratch.particle.random_pos;
        *vec = D_80097AFC[0];
        position = &scratch.particle.pos;
        SetBleed(position, vec, rand() % 20, 0xffff00);
    }

    count = param->count - 1;
    param->count = count;
    mode = item->mode;
    one = 1;
    switch (mode)
    {
    case 0:
        if ((u8)count == 0)
        {
            if (rand() % 10 < 2)
            {
                PARAM_ITEM_STAY *saved;
                PARAM_ITEM_USE *launch;

                memset(&scratch.drop.rparam, 0, sizeof(PARAM_ITEM_STAY));
                scratch.drop.rparam.type = item->type;
                scratch.drop.rparam.locate.vx = sprt->locate.coord.t[0];
                scratch.drop.rparam.locate.vy = sprt->locate.coord.t[1];
                scratch.drop.rparam.locate.vz = sprt->locate.coord.t[2];
                scratch.drop.saved = scratch.drop.rparam;

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

                saved = &scratch.drop.saved;
                launch = &scratch.drop.launch;
                scratch.drop.launch.type = saved->type;
                launch->user = (Humanoid *)1;
                scratch.drop.launch.start.vx = saved->locate.vx;
                scratch.drop.launch.start.vy = saved->locate.vy;
                scratch.drop.launch.start.vz = saved->locate.vz;
                scratch.drop.launch.end.vx = 0;
                scratch.drop.launch.end.vy = 0;
                scratch.drop.launch.end.vz = 0;
                scratch.drop.launch.start.vy = GetAreaMapLevel(
                    GlobalAreaMap, scratch.drop.launch.start.vx,
                    scratch.drop.launch.start.vy,
                    scratch.drop.launch.start.vz, 0);
                ReqItemDrop(launch);
                SetSmokeS(&saved->locate, 0, -100, 0, 10);
                return;
            }
        }
        else
        {
            if ((u8)count == 0x8c)
            {
                s32 n;
                s32 size;
                s32 collision_mode;

                DeleteConflict(item->locate);
                n = InsertConflict(item->locate);
                size = 500;
                collision_mode = 8;
                ConflictObject[n].offset.vx = 0;
                ConflictObject[n].offset.vz = 0;
                ConflictObject[n].offset.vy = 0;
                ConflictObject[n].size.vz = size;
                ConflictObject[n].size.vy = size;
                ConflictObject[n].size.vx = size;
                ConflictObject[n].common = (void *)one;
                ConflictObject[n].size.pad = collision_mode;
                item->coll_size = size;
                item->coll_ofsY = 0;
                item->coll_mode = collision_mode;
                item->coll_pause = 0;
            }

            if ((item->locate->attribute & 0x8000) == 0)
            {
                cid = -1;
            }
            else
            {
                cid = GetConflictResult(item->locate, -1);
            }
            if (cid == -1)
            {
                return;
            }
            if (is_character_state_present_on_stage_(
                    (Humanoid *)ConflictObject[cid].common) == 0 &&
                ConflictObject[cid].size.pad != 1)
            {
                return;
            }
        }
        item->mode = item->mode + 1;
        return;

    case 1:
    {
        s32 n;

        scratch.explosion.vec = D_80097AE4[0];
        memset(&scratch.explosion.pos_buf, 0, sizeof(VECTOR));
        scratch.explosion.pos_buf.vx = item->locate->locate.coord.t[0];
        scratch.explosion.pos_buf.vy = item->locate->locate.coord.t[1];
        scratch.explosion.pos_buf.vz = item->locate->locate.coord.t[2];
        scratch.explosion.pos = scratch.explosion.pos_buf;
        SetExplosion(&scratch.explosion.pos, &scratch.explosion.vec);

        scratch.explosion.vec.vx = 75;
        scratch.explosion.vec.vy = 120;
        scratch.explosion.vec.vz = 75;
        SetHinoko(&scratch.explosion.pos, &scratch.explosion.vec, 8);
        scratch.explosion.vec.vx = 0;
        scratch.explosion.vec.vy = -200;
        scratch.explosion.vec.vz = 0;
        SetSmoke(&scratch.explosion.pos, &scratch.explosion.vec, 20, 6);
        SoundEx(&scratch.explosion.pos, 0x25);

        DeleteConflict(item->locate);
        n = InsertConflict(item->locate);
        ConflictObject[n].offset.vx = 0;
        ConflictObject[n].offset.vz = 0;
        ConflictObject[n].offset.vy = 0;
        ConflictObject[n].size.vz = 1500;
        ConflictObject[n].size.vy = 1500;
        ConflictObject[n].size.vx = 1500;
        ConflictObject[n].common = (void *)(s32)mode;
        ConflictObject[n].size.pad = mode;
        item->coll_size = 1500;
        item->coll_ofsY = 0;
        item->coll_mode = mode;
        item->coll_pause = 0;
        item->mode = item->mode + 1;
        param->count = 3;
        reset_alert_duration();
        return;
    }

    case 2:
        if ((u8)count == 0 && item->proc != 0)
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
                ModelType **objects;
                ModelType *frame_model;
                objects = human->model->object;
                if (human->model->n > 0)
                {
                    objects += rand() % human->model->n;
                }
                frame_model = *objects;
                memset(&scratch.frame.random_pos, 0, sizeof(VECTOR));
                scratch.frame.random_pos.vx = rand() % 200 - 100;
                scratch.frame.random_pos.vy = rand() % 200 - 100;
                scratch.frame.random_pos.vz = rand() % 200 - 100;
                scratch.frame.pos = scratch.frame.random_pos;
                SetFrame(&scratch.frame.pos, 0x3000, 0x78,
                         (GsCOORDINATE2 *)frame_model);
            }
        }
        return;
    }
}

// triage: HARD — 583 insns, mul/div, 1 loop, indirect-call, 20 callees, ~0.22 to ProcItemHappou
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Type propagation algorithm not settling */
//
// void ProcItemFire(tag_TItem *item)
//
// {
//   byte bVar1;
//   short sVar2;
//   ModelType *pMVar3;
//   int iVar4;
//   int iVar5;
//   uchar uVar6;
//   Sprite3D *pSVar7;
//   SVECTOR *pSVar8;
//   undefined4 uVar9;
//   undefined4 uVar10;
//   undefined4 uVar11;
//   VECTOR *pos;
//   undefined *puVar12;
//   undefined4 *puVar13;
//   _GsCOORDINATE2 *super;
//   Sprite3D *sprt;
//   undefined1 local_80 [12];
//   long local_74;
//   SVECTOR local_70;
//   int local_68;
//   long local_64;
//   TItemType local_60;
//   long local_5c;
//   long local_58;
//   long local_54;
//   SVECTOR local_50;
//   PARAM_ITEM_USE local_48;
//
//   sprt = (Sprite3D *)item->model;
//   if (item->mode == 0xff) {
//     item->mode = '\0';
//   }
//   else {
//     MoveKorogari(item,(param_korogari *)&(item->param).launch);
//     if (*(char *)((int)&item->param + 10) == '\x01') {
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
//     else {
//       UpdateCoordinate(item->locate);
//       pMVar3 = item->locate;
//       pSVar8 = &pMVar3->rotate;
//       pSVar7 = sprt;
//       do {
//         uVar9 = *(undefined4 *)(pMVar3->locate).coord.m[0];
//         uVar10 = *(undefined4 *)((pMVar3->locate).coord.m[0] + 2);
//         uVar11 = *(undefined4 *)((pMVar3->locate).coord.m[1] + 1);
//         (pSVar7->locate).flg = (pMVar3->locate).flg;
//         *(undefined4 *)(pSVar7->locate).coord.m[0] = uVar9;
//         *(undefined4 *)((pSVar7->locate).coord.m[0] + 2) = uVar10;
//         *(undefined4 *)((pSVar7->locate).coord.m[1] + 1) = uVar11;
//         pMVar3 = (ModelType *)((pMVar3->locate).coord.m + 2);
//         pSVar7 = (Sprite3D *)((pSVar7->locate).coord.m + 2);
//       } while (pMVar3 != (ModelType *)pSVar8);
//       DrawSprite(sprt);
//       memset((uchar *)&local_70,'\0',0x10);
//       iVar4 = rand();
//       local_70._0_4_ = (item->locate->locate).coord.t[0] + -0x19 + iVar4 % 0x32;
//       iVar4 = rand();
//       local_70._4_4_ = (item->locate->locate).coord.t[1] + -0x19 + iVar4 % 0x32;
//       iVar4 = rand();
//       local_80._8_4_ = (item->locate->locate).coord.t[2] + -0x19 + iVar4 % 0x32;
//       local_80._0_2_ = local_70.vx;
//       local_80._2_2_ = local_70.vy;
//       local_80._4_2_ = local_70.vz;
//       local_80._6_2_ = local_70.pad;
//       local_74 = local_64;
//       local_70.vx = (short)DAT_80097afc;
//       local_70.vy = DAT_80097afc._2_2_;
//       local_70.vz = (short)DAT_80097b00;
//       local_70.pad = DAT_80097b00._2_2_;
//       local_68 = local_80._8_4_;
//       iVar4 = rand();
//       SetBleed((VECTOR *)local_80,&local_70,iVar4 % 0x14,0xffff00);
//       uVar6 = (item->param).smoke.count + 0xff;
//       (item->param).smoke.count = uVar6;
//       bVar1 = item->mode;
//       if (bVar1 == 1) {
//         local_80._0_4_ = DAT_80097ae4;
//         local_80._4_4_ = DAT_80097ae8;
//         memset((uchar *)&local_68,'\0',0x10);
//         local_80._8_4_ = (item->locate->locate).coord.t[0];
//         local_74 = (item->locate->locate).coord.t[1];
//         pos = (VECTOR *)(local_80 + 8);
//         local_70._0_4_ = (item->locate->locate).coord.t[2];
//         local_70.vz = (undefined2)local_5c;
//         local_70.pad = local_5c._2_2_;
//         local_68 = local_80._8_4_;
//         local_64 = local_74;
//         local_60 = local_70._0_4_;
//         SetExplosion(pos,(SVECTOR *)local_80);
//         local_80._0_4_ = 0x78004b;
//         local_80._4_2_ = 0x4b;
//         SetHinoko(pos,(SVECTOR *)local_80,8);
//         local_80._0_4_ = -0xc80000;
//         local_80._4_4_ = (uint)(ushort)local_80._6_2_ << 0x10;
//         SetSmoke(pos,(SVECTOR *)local_80,0x14,6);
//         SoundEx(pos,0x25);
//         DeleteConflict(item->locate);
//         sVar2 = InsertConflict(item->locate);
//         ConflictObject[sVar2].offset.vx = 0;
//         ConflictObject[sVar2].offset.vz = 0;
//         ConflictObject[sVar2].offset.vy = 0;
//         ConflictObject[sVar2].size.vz = 0x5dc;
//         ConflictObject[sVar2].size.vy = 0x5dc;
//         ConflictObject[sVar2].size.vx = 0x5dc;
//         ConflictObject[sVar2].common = (undefined *)0x1;
//         ConflictObject[sVar2].size.pad = 1;
//         uVar6 = item->mode;
//         (item->collision).size = 0x5dc;
//         (item->collision).ofsY = 0;
//         (item->collision).mode = 1;
//         (item->collision).pause = 0;
//         item->mode = uVar6 + '\x01';
//         (item->param).smoke.count = '\x03';
//         FUN_8002f7f4();
//       }
//       else if (bVar1 < 2) {
//         if (bVar1 == 0) {
//           if (uVar6 == '\0') {
//             iVar4 = rand();
//             if (iVar4 % 10 < 2) {
//               memset((uchar *)&local_60,'\0',0x14);
//               local_80._0_4_ = item->type;
//               local_80._4_4_ = (sprt->locate).coord.t[0];
//               local_80._8_4_ = (sprt->locate).coord.t[1];
//               local_74 = (sprt->locate).coord.t[2];
//               local_70.vx = local_50.vx;
//               local_70.vy = local_50.vy;
//               local_60 = local_80._0_4_;
//               local_5c = local_80._4_4_;
//               local_58 = local_80._8_4_;
//               local_54 = local_74;
//               if (item->proc != (undefined **)0x0) {
//                 item->mode = 0xff;
//                 (*(code *)item->proc)(item);
//                 DeleteConflict(item->locate);
//                 if (item->mode != 0) {
//                   AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//                 }
//                 item->owner = (Humanoid *)0x0;
//                 item->proc = (undefined **)0x0;
//               }
//               local_48.type = local_80._0_4_;
//               local_48.user = (Humanoid *)&DAT_00000001;
//               local_48.start.vx = local_80._4_4_;
//               local_48.start.vy = local_80._8_4_;
//               local_48.end.vx = 0;
//               local_48.end.vy = 0;
//               local_48.end.vz = 0;
//               local_48.start.vz = local_74;
//               local_48.start.vy = GetAreaMapLevel(GlobalAreaMap,local_80._4_4_,local_80._8_4_);
//               ReqItemDrop(&local_48);
//               SetSmokeS(local_80 + 4,0,0xffffff9c,0,10);
//               return;
//             }
//           }
//           else {
//             if (uVar6 == 0x8c) {
//               DeleteConflict(item->locate);
//               sVar2 = InsertConflict(item->locate);
//               ConflictObject[sVar2].offset.vx = 0;
//               ConflictObject[sVar2].offset.vz = 0;
//               ConflictObject[sVar2].offset.vy = 0;
//               ConflictObject[sVar2].size.vz = 500;
//               ConflictObject[sVar2].size.vy = 500;
//               ConflictObject[sVar2].size.vx = 500;
//               ConflictObject[sVar2].common = (undefined *)0x1;
//               ConflictObject[sVar2].size.pad = 8;
//               (item->collision).size = 500;
//               (item->collision).ofsY = 0;
//               (item->collision).mode = 8;
//               (item->collision).pause = 0;
//             }
//             if (((int)item->locate->attribute & 0x8000U) == 0) {
//               iVar4 = -1;
//             }
//             else {
//               sVar2 = GetConflictResult(item->locate,-1);
//               iVar4 = (int)sVar2;
//             }
//             if (iVar4 == -1) {
//               return;
//             }
//             iVar5 = FUN_800294dc(ConflictObject[iVar4].common);
//             if ((iVar5 == 0) && (ConflictObject[iVar4].size.pad != 1)) {
//               return;
//             }
//           }
//           item->mode = item->mode + '\x01';
//         }
//       }
//       else if (bVar1 == 2) {
//         if ((uVar6 == '\0') && (item->proc != (undefined **)0x0)) {
//           item->mode = 0xff;
//           (*(code *)item->proc)(item);
//           DeleteConflict(item->locate);
//           if (item->mode != 0) {
//             AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//           }
//           item->owner = (Humanoid *)0x0;
//           item->proc = (undefined **)0x0;
//         }
//         if (((int)item->locate->attribute & 0x8000U) == 0) {
//           iVar4 = -1;
//         }
//         else {
//           sVar2 = GetConflictResult(item->locate,-1);
//           iVar4 = (int)sVar2;
//         }
//         if (iVar4 != -1) {
//           puVar12 = ConflictObject[iVar4].common;
//           iVar4 = FUN_800294dc(puVar12);
//           if (iVar4 != 0) {
//             puVar13 = *(undefined4 **)(*(int *)(puVar12 + 0x58) + 0x68);
//             if (0 < *(short *)(*(int *)(puVar12 + 0x58) + 100)) {
//               iVar4 = rand();
//               iVar5 = (int)*(short *)(*(int *)(puVar12 + 0x58) + 100);
//               if (iVar5 == 0) {
//                 trap(0x1c00);
//               }
//               if ((iVar5 == -1) && (iVar4 == -0x80000000)) {
//                 trap(0x1800);
//               }
//               puVar13 = puVar13 + iVar4 % iVar5;
//             }
//             super = (_GsCOORDINATE2 *)*puVar13;
//             memset((uchar *)&local_70,'\0',0x10);
//             iVar4 = rand();
//             local_70._0_4_ = iVar4 % 200 + -100;
//             iVar4 = rand();
//             local_70._4_4_ = iVar4 % 200 + -100;
//             iVar4 = rand();
//             local_80._8_4_ = iVar4 % 200 + -100;
//             local_80._0_2_ = local_70.vx;
//             local_80._2_2_ = local_70.vy;
//             local_80._4_2_ = local_70.vz;
//             local_80._6_2_ = local_70.pad;
//             local_74 = local_64;
//             local_68 = local_80._8_4_;
//             SetFrame((VECTOR *)local_80,0x3000,0x78,super);
//           }
//         }
//       }
//     }
//   }
//   return;
// }
