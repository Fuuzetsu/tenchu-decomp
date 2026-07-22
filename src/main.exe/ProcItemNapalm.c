#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * ProcItemNapalm (0x800469c0) — expands the thrown fireball for twenty
 * frames, updates its two layered sprites, registers the damage conflict at
 * frame ten, emits a random attached frame effect on a collided character,
 * and disposes when it leaves the area map.  Modes 0/1/2 are initialise,
 * animate, and dispose; unknown modes still take the shared draw tail.
 *
 * Matching notes:
 *  - `ff` is live to mode 2 in $s6, while `switch (item->mode)` deliberately
 *    reloads the mode into $s4.  Literal `1` stores in case 1 reuse that switch
 *    index through cse's taken-edge equivalence; an explicit `mode` local adds
 *    $s7 and changes the frame.
 *  - The colour `t` is `u8` and is assigned in three statements.  Its narrow
 *    mode keeps rand()%25's quotient separate from the final remainder, giving
 *    the target's $v0/$v1 chain and the copy used by the third byte store.
 *    A single int expression is one instruction short and globally re-colours
 *    the chain.
 *  - The InsertConflict result is block-local `n`, distinct from the later
 *    query `cid`; sharing them rotates every register in the 0x78-byte index
 *    calculation.  Likewise, cache `proc` for the null check but invoke through
 *    `item->proc`: cse retains one load and allocates the target in $v0.
 *  - `random_pos` is zeroed and copied wholesale to `pos`; the two VECTOR
 *    declarations reproduce the sp+0x18/sp+0x28 stack objects and four-word
 *    block copy before SetFrame.
 *  - Both cleanup sequences are written out so jump2 cross-jumps from the map
 *    failure into mode 2's copy.  The two coordinate struct assignments at the
 *    end intentionally lower to the target's 0x50-byte copy loops.
 *  - This TU needs maspsx `--expand-div` for the model-count remainder guards
 *    and `--gp-extern sprNapalm2` for the six retail gp-relative loads; Build.hs
 *    and permute.py carry the mirrored per-function settings.
 */

extern Sprite3D *sprNapalm2;

extern s16 InsertConflict(ModelType *model);
extern s16 GetConflictResult(ModelType *model, s32 n);
extern s32 is_character_state_present_on_stage_(Humanoid *human);
extern long GetAreaMapLevel(u_long *area, long x, long y, long z, int mode);
extern void SetFrame(VECTOR *pos, short size, short time,
                     GsCOORDINATE2 *super);

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemNapalm(struct tag_TItem *item);
 *     ITEM.C:3014, 71 src lines, frame 80 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
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
 *     reg   $s5       struct Sprite3D * model
 *     reg   $s3       struct param_napalm * param
 *     reg   $s0       int ex
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s0       struct Humanoid * human
 *     reg   $s0       struct Humanoid * human
 *     reg   $s1       struct ModelType * model
 *     stack sp+16     struct VECTOR pos
 *     reg   $s2       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct Sprite3D *sprNapalm2;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

void ProcItemNapalm(tag_TItem *item)
{
    Sprite3D *sprt;
    param_napalm *param;
    void (*proc)(tag_TItem *);
    u8 ff;
    u8 count;
    s32 ex;
    s32 cid;

    sprt = (Sprite3D *)item->model;
    param = &item->param.napalm;
    ff = 0xff;
    if (item->mode == ff)
    {
        item->mode = 0;
        return;
    }

    switch (item->mode)
    {
    case 0:
        param->count = 0;
        item->mode = item->mode + 1;
        return;

    case 1:
    {
        u8 t;

        ex = param->count;
        ex = ex * ex;
        item->locate->locate.coord.t[0] += param->vec.vx * ex / 100;
        item->locate->locate.coord.t[1] += param->vec.vy * ex / 100;
        item->locate->locate.coord.t[2] += param->vec.vz * ex / 100;

        t = rand() % 25;
        t = t - 26;
        t = t - param->count * 230 / 20;
        sprt->sprite.r = t;
        sprt->sprite.g = sprt->sprite.r;
        sprt->sprite.b = sprt->sprite.r;
        sprt->sprite.rotate = (rand() % 360) << 12;
        sprt->scale = (ex << 12) / 50 + 0x1000;

        sprNapalm2->sprite.r = (ff - sprt->sprite.r) / 3;
        sprNapalm2->sprite.g = sprNapalm2->sprite.r;
        sprNapalm2->sprite.b = sprNapalm2->sprite.r;
        sprNapalm2->sprite.rotate = sprt->sprite.rotate;
        sprNapalm2->scale = sprt->scale;

        if (param->count == 10)
        {
            s32 n;

            DeleteConflict(item->locate);
            n = InsertConflict(item->locate);
            ConflictObject[n].offset.vx = 0;
            ConflictObject[n].offset.vz = 0;
            ConflictObject[n].offset.vy = 0;
            ConflictObject[n].size.vz = 500;
            ConflictObject[n].size.vy = 500;
            ConflictObject[n].size.vx = 500;
            ConflictObject[n].common = (void *)1;
            ConflictObject[n].size.pad = 1;
            item->collision.size = 500;
            item->collision.ofsY = 0;
            item->collision.mode = 1;
            item->collision.pause = 0;
        }

        count = param->count + 1;
        param->count = count;
        if (count > 20)
        {
            item->mode = item->mode + 1;
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
                VECTOR pos;
                VECTOR random_pos;

                objects = human->model->object;
                if (human->model->n > 0)
                {
                    objects += rand() % human->model->n;
                }
                frame_model = *objects;
                memset(&random_pos, 0, sizeof(VECTOR));
                random_pos.vx = rand() % 200 - 100;
                random_pos.vy = rand() % 200 - 100;
                random_pos.vz = rand() % 200 - 100;
                pos = random_pos;
                SetFrame(&pos, 0x3000, 0x3c,
                         (GsCOORDINATE2 *)frame_model);
            }
        }

        if (GetAreaMapLevel(GlobalAreaMap,
                            item->locate->locate.coord.t[0],
                            item->locate->locate.coord.t[1],
                            item->locate->locate.coord.t[2], 0) ==
            (long)0x80000000)
        {
            proc = item->proc;
            if (proc == 0)
            {
                return;
            }
            item->mode = 0xff;
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
        break;
    }

    case 2:
        proc = item->proc;
        if (proc == 0)
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

    UpdateCoordinate(item->locate);
    sprNapalm2->locate = item->locate->locate;
    DrawSprite(sprNapalm2);
    sprt->locate = item->locate->locate;
    DrawSprite(sprt);
}

// triage: HARD — 418 insns, mul/div, 2 loop, indirect-call, 11 callees, ~0.25 to ProcItemMakibishi
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Type propagation algorithm not settling */
//
// void ProcItemNapalm(tag_TItem *item)
//
// {
//   uchar uVar1;
//   byte bVar2;
//   short sVar3;
//   int iVar4;
//   long lVar5;
//   undefined **ppuVar6;
//   ModelType *pMVar7;
//   Sprite3D *pSVar8;
//   int iVar9;
//   SVECTOR *pSVar10;
//   int iVar11;
//   undefined4 uVar12;
//   undefined4 uVar13;
//   undefined4 uVar14;
//   uint uVar15;
//   undefined *puVar16;
//   undefined4 *puVar17;
//   _GsCOORDINATE2 *super;
//   Sprite3D *sprt;
//   VECTOR local_40;
//   int local_30;
//   int local_2c;
//   int local_28;
//   long local_24;
//
//   sprt = (Sprite3D *)item->model;
//   if (item->mode == 0xff) {
//     item->mode = '\0';
//     return;
//   }
//   bVar2 = item->mode;
//   if (bVar2 == 1) {
//     uVar15 = (uint)(item->param).napalm.count;
//     iVar9 = uVar15 * uVar15;
//     (item->locate->locate).coord.t[0] =
//          (item->locate->locate).coord.t[0] + ((item->param).napalm.vec.vx * iVar9) / 100;
//     (item->locate->locate).coord.t[1] =
//          (item->locate->locate).coord.t[1] + ((item->param).napalm.vec.vy * iVar9) / 100;
//     (item->locate->locate).coord.t[2] =
//          (item->locate->locate).coord.t[2] + ((item->param).napalm.vec.vz * iVar9) / 100;
//     iVar4 = rand();
//     uVar1 = ((char)iVar4 + (char)(iVar4 / 0x19) * -0x19 + -0x1a) -
//             (char)(((uint)(item->param).napalm.count * 0xe6) / 0x14);
//     (sprt->sprite).r = uVar1;
//     (sprt->sprite).g = uVar1;
//     (sprt->sprite).b = uVar1;
//     iVar4 = rand();
//     iVar11 = 0xff - (uint)(sprt->sprite).r;
//     (sprt->sprite).rotate = (iVar4 % 0x168) * 0x1000;
//     sprt->scale = (uint)(iVar9 * 0x1000) / 0x32 + 0x1000;
//     (DAT_80097f60->sprite).r =
//          (char)((ulonglong)((longlong)iVar11 * 0x55555556) >> 0x20) - (char)(iVar11 >> 0x1f);
//     (DAT_80097f60->sprite).g = (DAT_80097f60->sprite).r;
//     (DAT_80097f60->sprite).b = (DAT_80097f60->sprite).r;
//     pSVar8 = DAT_80097f60;
//     (DAT_80097f60->sprite).rotate = (sprt->sprite).rotate;
//     pSVar8->scale = sprt->scale;
//     if ((item->param).napalm.count == '\n') {
//       DeleteConflict(item->locate);
//       sVar3 = InsertConflict(item->locate);
//       ConflictObject[sVar3].offset.vx = 0;
//       ConflictObject[sVar3].offset.vz = 0;
//       ConflictObject[sVar3].offset.vy = 0;
//       ConflictObject[sVar3].size.vz = 500;
//       ConflictObject[sVar3].size.vy = 500;
//       ConflictObject[sVar3].size.vx = 500;
//       ConflictObject[sVar3].common = (undefined *)0x1;
//       ConflictObject[sVar3].size.pad = 1;
//       (item->collision).size = 500;
//       (item->collision).ofsY = 0;
//       (item->collision).mode = 1;
//       (item->collision).pause = 0;
//     }
//     bVar2 = (item->param).napalm.count + 1;
//     (item->param).napalm.count = bVar2;
//     if (0x14 < bVar2) {
//       item->mode = item->mode + '\x01';
//     }
//     if (((int)item->locate->attribute & 0x8000U) == 0) {
//       iVar9 = -1;
//     }
//     else {
//       sVar3 = GetConflictResult(item->locate,-1);
//       iVar9 = (int)sVar3;
//     }
//     if (iVar9 != -1) {
//       puVar16 = ConflictObject[iVar9].common;
//       iVar9 = FUN_800294dc(puVar16);
//       if (iVar9 != 0) {
//         puVar17 = *(undefined4 **)(*(int *)(puVar16 + 0x58) + 0x68);
//         if (0 < *(short *)(*(int *)(puVar16 + 0x58) + 100)) {
//           iVar9 = rand();
//           iVar4 = (int)*(short *)(*(int *)(puVar16 + 0x58) + 100);
//           if (iVar4 == 0) {
//             trap(0x1c00);
//           }
//           if ((iVar4 == -1) && (iVar9 == -0x80000000)) {
//             trap(0x1800);
//           }
//           puVar17 = puVar17 + iVar9 % iVar4;
//         }
//         super = (_GsCOORDINATE2 *)*puVar17;
//         memset((uchar *)&local_30,'\0',0x10);
//         iVar9 = rand();
//         local_30 = iVar9 % 200 + -100;
//         iVar9 = rand();
//         local_2c = iVar9 % 200 + -100;
//         iVar9 = rand();
//         local_40.vz = iVar9 % 200 + -100;
//         local_40.vx = local_30;
//         local_40.vy = local_2c;
//         local_40.pad = local_24;
//         local_28 = local_40.vz;
//         SetFrame(&local_40,0x3000,0x3c,super);
//       }
//     }
//     lVar5 = GetAreaMapLevel(GlobalAreaMap,(item->locate->locate).coord.t[0],
//                             (item->locate->locate).coord.t[1]);
//     if (lVar5 == -0x80000000) {
//       ppuVar6 = item->proc;
//       if (ppuVar6 == (undefined **)0x0) {
//         return;
//       }
//       item->mode = 0xff;
// LAB_80046f50:
//       (*(code *)ppuVar6)(item);
//       DeleteConflict(item->locate);
//       if (item->mode != 0) {
//         AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//       }
//       item->owner = (Humanoid *)0x0;
//       item->proc = (undefined **)0x0;
//       return;
//     }
//   }
//   else if (bVar2 < 2) {
//     if (bVar2 == 0) {
//       (item->param).napalm.count = '\0';
//       item->mode = item->mode + '\x01';
//       return;
//     }
//   }
//   else if (bVar2 == 2) {
//     ppuVar6 = item->proc;
//     if (ppuVar6 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
//     goto LAB_80046f50;
//   }
//   UpdateCoordinate(item->locate);
//   pMVar7 = item->locate;
//   pSVar10 = &pMVar7->rotate;
//   pSVar8 = DAT_80097f60;
//   do {
//     uVar13 = *(undefined4 *)(pMVar7->locate).coord.m[0];
//     uVar14 = *(undefined4 *)((pMVar7->locate).coord.m[0] + 2);
//     uVar12 = *(undefined4 *)((pMVar7->locate).coord.m[1] + 1);
//     (pSVar8->locate).flg = (pMVar7->locate).flg;
//     *(undefined4 *)(pSVar8->locate).coord.m[0] = uVar13;
//     *(undefined4 *)((pSVar8->locate).coord.m[0] + 2) = uVar14;
//     *(undefined4 *)((pSVar8->locate).coord.m[1] + 1) = uVar12;
//     pMVar7 = (ModelType *)((pMVar7->locate).coord.m + 2);
//     pSVar8 = (Sprite3D *)((pSVar8->locate).coord.m + 2);
//   } while (pMVar7 != (ModelType *)pSVar10);
//   DrawSprite(DAT_80097f60);
//   pMVar7 = item->locate;
//   pSVar10 = &pMVar7->rotate;
//   pSVar8 = sprt;
//   do {
//     uVar13 = *(undefined4 *)(pMVar7->locate).coord.m[0];
//     uVar14 = *(undefined4 *)((pMVar7->locate).coord.m[0] + 2);
//     uVar12 = *(undefined4 *)((pMVar7->locate).coord.m[1] + 1);
//     (pSVar8->locate).flg = (pMVar7->locate).flg;
//     *(undefined4 *)(pSVar8->locate).coord.m[0] = uVar13;
//     *(undefined4 *)((pSVar8->locate).coord.m[0] + 2) = uVar14;
//     *(undefined4 *)((pSVar8->locate).coord.m[1] + 1) = uVar12;
//     pMVar7 = (ModelType *)((pMVar7->locate).coord.m + 2);
//     pSVar8 = (Sprite3D *)((pSVar8->locate).coord.m + 2);
//   } while (pMVar7 != (ModelType *)pSVar10);
//   DrawSprite(sprt);
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? AdtMessageBox(? *, s32, u8);                      /* extern */
// ? DeleteConflict(void *, u8, ?, s32);               /* extern */
// ? DrawSprite(void *);                               /* extern */
// s32 GetAreaMapLevel(s32, s32, s32, s32, s32);       /* extern */
// s16 GetConflictResult(void *, ?);                   /* extern */
// s16 InsertConflict(void *);                         /* extern */
// ? SetFrame(s32 *, ?, ?, s32);                       /* extern */
// ? UpdateCoordinate(void *);                         /* extern */
// s32 is_character_state_present_on_stage_(void *);   /* extern */
// ? memset(s32 *, ?, ?);                              /* extern */
// s32 rand(s32, void *, s32);                         /* extern */
// extern ? ConflictObject;
// extern ? D_800121CC;
// extern void *sprNapalm2;
// extern s32 GlobalAreaMap;
//
// void ProcItemNapalm(void *arg0) {
//     s32 sp18;
//     s32 sp1C;
//     s32 sp20;
//     s32 sp24;
//     s32 sp28;
//     s32 sp2C;
//     s32 sp30;
//     s16 var_a0;
//     s32 *var_s1;
//     s32 temp_a0;
//     s32 temp_lo;
//     s32 temp_ret;
//     s32 temp_ret_2;
//     s32 temp_ret_3;
//     s32 temp_s1;
//     u8 temp_a1_4;
//     u8 temp_s0;
//     u8 temp_s4;
//     u8 temp_v0;
//     u8 temp_v0_2;
//     u8 temp_v0_5;
//     u8 temp_v1;
//     void *temp_a0_2;
//     void *temp_a0_3;
//     void *temp_a1;
//     void *temp_a1_2;
//     void *temp_a1_3;
//     void *temp_s0_2;
//     void *temp_s3;
//     void *temp_s5;
//     void *temp_v0_3;
//     void *temp_v0_4;
//     void *temp_v1_2;
//     void *temp_v1_3;
//     void *var_a0_2;
//     void *var_v0;
//     void *var_v0_2;
//     void *var_v1;
//
//     temp_s5 = arg0->unk4;
//     temp_s3 = arg0 + 0x20;
//     if (arg0->unk54 == 0xFF) {
//         arg0->unk54 = 0U;
//         return;
//     }
//     temp_s4 = arg0->unk54;
//     switch (temp_s4) {                              /* irregular */
//     case 0:
//         temp_s3->unk8 = 0U;
//         arg0->unk54 = (u8) (arg0->unk54 + 1);
//         return;
//     case 1:
//         temp_s0 = temp_s3->unk8;
//         temp_lo = temp_s0 * temp_s0;
//         temp_a1 = arg0->unk10;
//         temp_a1->unk18 = (s32) (temp_a1->unk18 + ((arg0->unk20 * temp_lo) / 100));
//         temp_a1_2 = arg0->unk10;
//         temp_a1_2->unk1C = (s32) (temp_a1_2->unk1C + ((temp_s3->unk2 * temp_lo) / 100));
//         temp_a1_3 = arg0->unk10;
//         temp_a0 = (temp_s3->unk4 * temp_lo) / 100;
//         temp_a1_3->unk20 = (s32) (temp_a1_3->unk20 + temp_a0);
//         temp_ret = rand(temp_a0, temp_a1_3);
//         temp_v1 = temp_s3->unk8;
//         temp_v0 = ((temp_ret % 25) - 0x1A) - ((temp_v1 * 0xE6) / 20);
//         temp_s5->unk7C = temp_v0;
//         temp_s5->unk7D = temp_v0;
//         temp_s5->unk7E = temp_v0;
//         temp_s5->unk88 = (s32) ((rand((s32) (temp_v1 * 0xE6) >> 0x1F, (void *) (temp_ret / 25), MULT_HI(temp_ret, 0x51EB851F)) % 360) << 0xC);
//         temp_s5->unk64 = (s32) (((temp_lo << 0xC) / 50) + 0x1000);
//         temp_a1_4 = (0xFF - temp_s5->unk7C) / 3;
//         sprNapalm2->unk7C = temp_a1_4;
//         sprNapalm2->unk7D = (u8) sprNapalm2->unk7C;
//         sprNapalm2->unk7E = (u8) sprNapalm2->unk7C;
//         sprNapalm2->unk88 = (s32) temp_s5->unk88;
//         sprNapalm2->unk64 = (s32) temp_s5->unk64;
//         if (temp_s3->unk8 == 0xA) {
//             DeleteConflict(arg0->unk10, temp_a1_4, 0x55555556, MULT_HI((temp_lo << 0xC), 0x51EB851F));
//             temp_v1_2 = (InsertConflict(arg0->unk10) * 0x78) + &ConflictObject;
//             temp_v1_2->unk14 = 0;
//             temp_v1_2->unk18 = 0;
//             temp_v1_2->unk16 = 0;
//             temp_v1_2->unk20 = 0x1F4;
//             temp_v1_2->unk1E = 0x1F4;
//             temp_v1_2->unk1C = 0x1F4;
//             temp_v1_2->unk24 = (s32) temp_s4;
//             temp_v1_2->unk22 = (s16) temp_s4;
//             arg0->unk1C = 0x1F4;
//             arg0->unk1E = 0;
//             arg0->unk14 = (s32) temp_s4;
//             arg0->unk18 = 0;
//         }
//         temp_v0_2 = temp_s3->unk8 + 1;
//         temp_s3->unk8 = temp_v0_2;
//         if ((u32) (temp_v0_2 & 0xFF) >= 0x15U) {
//             arg0->unk54 = (u8) (arg0->unk54 + 1);
//         }
//         temp_a0_2 = arg0->unk10;
//         if (!(temp_a0_2->unk5A & 0x8000)) {
//             var_a0 = -1;
//         } else {
//             var_a0 = GetConflictResult(temp_a0_2, -1);
//         }
//         if (var_a0 != -1) {
//             temp_s0_2 = ((var_a0 * 0x78) + &ConflictObject)->unk24;
//             if (is_character_state_present_on_stage_(temp_s0_2) != 0) {
//                 temp_v0_3 = temp_s0_2->unk58;
//                 var_s1 = temp_v0_3->unk68;
//                 if (temp_v0_3->unk64 > 0) {
//                     var_s1 += (rand((s32) &sp28) % (s16) temp_s0_2->unk58->unk64) * 4;
//                 }
//                 temp_s1 = *var_s1;
//                 memset(&sp28, 0, 0x10);
//                 temp_ret_2 = rand();
//                 sp28 = (temp_ret_2 % 200) - 0x64;
//                 temp_ret_3 = rand(temp_ret_2 / 200);
//                 sp2C = (temp_ret_3 % 200) - 0x64;
//                 sp30 = (rand(temp_ret_3 / 200) % 200) - 0x64;
//                 sp18 = sp28;
//                 sp1C = sp2C;
//                 sp20 = sp30;
//                 sp24 = sp34;
//                 SetFrame(&sp18, 0x3000, 0x3C, temp_s1);
//             }
//         }
//         temp_v0_4 = arg0->unk10;
//         if (GetAreaMapLevel(GlobalAreaMap, temp_v0_4->unk18, temp_v0_4->unk1C, temp_v0_4->unk20, 0) == 0x80000000) {
//             if (arg0->unkC != NULL) {
//                 arg0->unk54 = 0xFFU;
// block_26:
//                 arg0->unkC(arg0);
//                 DeleteConflict(arg0->unk10);
//                 temp_v0_5 = arg0->unk54;
//                 if (temp_v0_5 != 0) {
//                     AdtMessageBox(&D_800121CC, arg0->unk8, temp_v0_5);
//                 }
//                 arg0->unk0 = 0;
//                 arg0->unkC = NULL;
//                 return;
//             }
//         } else {
//         default:
//             UpdateCoordinate(arg0->unk10);
//             var_v0 = arg0->unk10;
//             var_v1 = sprNapalm2;
//             temp_a0_3 = var_v0 + 0x50;
//             do {
//                 var_v1->unk0 = (s32) var_v0->unk0;
//                 var_v1->unk4 = (s32) var_v0->unk4;
//                 var_v1->unk8 = (s32) var_v0->unk8;
//                 var_v1->unkC = (s32) var_v0->unkC;
//                 var_v0 += 0x10;
//                 var_v1 += 0x10;
//             } while (var_v0 != temp_a0_3);
//             DrawSprite(sprNapalm2);
//             var_a0_2 = arg0->unk10;
//             var_v0_2 = temp_s5;
//             temp_v1_3 = var_a0_2 + 0x50;
//             do {
//                 var_v0_2->unk0 = (s32) var_a0_2->unk0;
//                 var_v0_2->unk4 = (s32) var_a0_2->unk4;
//                 var_v0_2->unk8 = (s32) var_a0_2->unk8;
//                 var_v0_2->unkC = (s32) var_a0_2->unkC;
//                 var_a0_2 += 0x10;
//                 var_v0_2 += 0x10;
//             } while (var_a0_2 != temp_v1_3);
//             DrawSprite(temp_s5);
//             return;
//         }
//         break;
//     case 2:
//         if (arg0->unkC != NULL) {
//             arg0->unk54 = 0xFFU;
//             goto block_26;
//         }
//         break;
//     }
// }
