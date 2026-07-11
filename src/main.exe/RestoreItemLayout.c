#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void RestoreItemLayout(void *buf);
 *     ITEM.C:507, 22 src lines, frame 288 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $a0       void * buf
 *     reg   $s3       struct TItemLayout * slot
 *     stack sp+16     unsigned char [200] fn
 *     reg   $s1       int i
 *     reg   $s1       int i
 *     stack sp+216    struct PARAM_ITEM_STAY param
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct WorldType WorldMap[8][8][8];
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — right length is 624 bytes (156 insns); this draft
 * assembles to 628 (157 insns), 4 bytes / 1 instruction over, with 48
 * differing lines concentrated in two spots:
 *
 *  1. The initial "temp = *buf" 5-field fill (memset + 5 field reads into a
 *     scratch PARAM_ITEM_STAY, then copied into `param`) — the target
 *     batches the last 4 loads into t1-t4 THEN stores all 4 (register-
 *     scheduling headroom); this draft's cc1 run only ever finds one spare
 *     temp (v0) at that point and interleaves lw/sw/nop per field. Frame
 *     size and the full 10-register save mask (s0-s7+fp+ra) already match
 *     the target exactly (confirmed via cc1 -dg / rtldump), so this is a
 *     scheduler headroom difference, not a missing-register structural gap.
 *  2. The search sub-loop's success path: the target's `bnez` on success
 *     jumps CLEAR OVER the retry/loop-back code to a point AFTER the loop,
 *     doing the `param.locate.vx/vz` stores only on that one path before
 *     falling into the shared "if (k==4) skip" test; every C spelling tried
 *     that reproduces this exact jump-over-the-retry-code shape reintroduces
 *     a SECOND physical ReqItemStay call site (cc1 fails to cross-jump it
 *     with the direct/no-search-needed call, adding ~30 bytes) instead of
 *     the single shared call site this draft achieves by storing vx/vz
 *     unconditionally right after the "if (k<4)" block. Every variant of
 *     this trade-off (explicit `goto`+shared label, `k=4` recycled-test
 *     idiom, `else` vs plain fallthrough) was tried; this is the
 *     Pareto-best (single call site, closest byte count).
 *
 * Structural work that IS confirmed correct: both loops (the tag_TItem
 * dispose loop matches ClearItemLayout's hand-rolled goto-loop shape
 * exactly; the second loop's dispatch order, the shared "field9-style"
 * search-loop test recycling, and the `one`/`sentinel` constants promoted
 * to persistent locals so they demat once before the loop instead of at
 * every call site, matching the target's dedicated s7/s8-class registers).
 * One bounded permuter run (~470 iterations, `-j4`) found no score-0
 * candidate (best partial score 2810 vs base 3140, with candidates
 * increasingly erroring past iteration ~400) — not adopted.
 */

extern void *GlobalAreaMap;
extern short D_8008E404[];

extern long GetAreaMapLevel(void *map, long x, long y, long z, long e);
extern s32 abs(s32 x);
extern void *memset(void *s, int c, u32 n);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/RestoreItemLayout", RestoreItemLayout);
#else
void RestoreItemLayout(void *buf)
{
    tag_TItem *it;
    s32 i;
    s32 j;
    u8 *p;
    PARAM_ITEM_STAY param;
    s32 level;
    s32 one;
    s32 sentinel;

    i = 0;
    it = items;
loop1:
    if (!(i < 0x1e))
        goto loop1_end;
    if (it->proc != 0) {
        it->mode = 0xff;
        it->proc(it);
        DeleteConflict(it->locate);
        if (it->mode != 0) {
            AdtMessageBox(D_800121CC, it->type, (u32)it->mode);
        }
        it->owner = 0;
        it->proc = 0;
    }
    it++;
    i++;
    goto loop1;
loop1_end:

    j = 0;
    one = 1;
    sentinel = -0x80000000;
    p = (u8 *)buf;
loop2:
    if (!(j < 0x1e))
        return;
    if (*(s32 *)p != -1) {
        PARAM_ITEM_STAY tmp;

        memset(&tmp, 0, sizeof(PARAM_ITEM_STAY));
        tmp.type = *(s32 *)p;
        tmp.locate.vx = *(s32 *)(p + 4);
        tmp.locate.vy = *(s32 *)(p + 8);
        tmp.locate.vz = *(s32 *)(p + 0xc);
        tmp.locate.pad = *(s32 *)(p + 0x10);
        param = tmp;

        level = GetAreaMapLevel(GlobalAreaMap, param.locate.vx, param.locate.vy, param.locate.vz, one);
        if (level == sentinel || abs(level - param.locate.vy) >= 0x3e8) {
            s32 k;
            s32 x, z;
            short *offs;

            k = 0;
            offs = D_8008E404;
        searchloop:
            if (k < 4) {
                x = param.locate.vx + offs[0] * 1000;
                z = param.locate.vz + offs[1] * 1000;
                level = GetAreaMapLevel(GlobalAreaMap, x, param.locate.vy, z, one);
                if (level == sentinel || abs(level - param.locate.vy) >= 0x3e8) {
                    offs += 2;
                    k++;
                    goto searchloop;
                }
            }
            param.locate.vz = z;
            param.locate.vx = x;
            if (k == 4) {
                goto skip_stay;
            }
        }
        param.locate.vy = level;
        ReqItemStay(&param);
    skip_stay:;
    }
    p += 0x14;
    j++;
    goto loop2;
}
#endif

// triage: MEDIUM — 156 insns, indirect-call, 6 callees, ~0.14 to ClearItemLayout
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void RestoreItemLayout(undefined *buf)
//
// {
//   TItemType TVar1;
//   int iVar2;
//   tag_TItem *ptVar3;
//   int iVar4;
//   TItemType TVar5;
//   TItemType x;
//   short *psVar6;
//   int iVar7;
//   PARAM_ITEM_STAY local_58;
//   TItemType local_40;
//   TItemType local_3c;
//   TItemType local_38;
//   TItemType local_34;
//   TItemType local_30;
//
//   ptVar3 = items;
//   for (iVar4 = 0; iVar7 = 0, iVar4 < 0x1e; iVar4 = iVar4 + 1) {
//     if (ptVar3->proc != (undefined **)0x0) {
//       ptVar3->mode = 0xff;
//       (*(code *)ptVar3->proc)(ptVar3);
//       DeleteConflict(ptVar3->locate);
//       if (ptVar3->mode != 0) {
//         AdtMessageBox("item dispose fail   id %d  mode %d",ptVar3->type,(uint)ptVar3->mode);
//       }
//       ptVar3->owner = (Humanoid *)0x0;
//       ptVar3->proc = (undefined **)0x0;
//     }
//     ptVar3 = ptVar3 + 1;
//   }
//   do {
//     if (0x1d < iVar7) {
//       return;
//     }
//     if (*(TItemType *)buf != ~ITEM_KAGINAWA) {
//       memset((uchar *)&local_40,'\0',0x14);
//       local_58.type = *(TItemType *)buf;
//       local_58.locate.vx = *(TItemType *)((int)buf + 4);
//       local_58.locate.vy = *(TItemType *)((int)buf + 8);
//       local_58.locate.vz = *(TItemType *)((int)buf + 0xc);
//       local_58.locate.pad = *(TItemType *)((int)buf + 0x10);
//       local_40 = local_58.type;
//       local_3c = local_58.locate.vx;
//       local_38 = local_58.locate.vy;
//       local_34 = local_58.locate.vz;
//       local_30 = local_58.locate.pad;
//       TVar1 = GetAreaMapLevel(GlobalAreaMap,local_58.locate.vx,local_58.locate.vy);
//       if ((TVar1 == 0x80000000) || (iVar4 = abs(TVar1 - local_58.locate.vy), 999 < iVar4)) {
//         psVar6 = &DAT_8008e404;
//         for (iVar4 = 0; x = local_58.locate.vx, TVar5 = local_58.locate.vz, iVar4 < 4;
//             iVar4 = iVar4 + 1) {
//           x = local_58.locate.vx + *psVar6 * 1000;
//           TVar5 = local_58.locate.vz + psVar6[1] * 1000;
//           TVar1 = GetAreaMapLevel(GlobalAreaMap,x,local_58.locate.vy);
//           if ((TVar1 != 0x80000000) && (iVar2 = abs(TVar1 - local_58.locate.vy), iVar2 < 1000))
//           break;
//           psVar6 = psVar6 + 2;
//         }
//         local_58.locate.vz = TVar5;
//         local_58.locate.vx = x;
//         if (iVar4 == 4) goto LAB_8003d39c;
//       }
//       local_58.locate.vy = TVar1;
//       ReqItemStay(&local_58);
//     }
// LAB_8003d39c:
//     buf = (undefined *)((int)buf + 0x14);
//     iVar7 = iVar7 + 1;
//   } while( true );
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? AdtMessageBox(? *, s32, u8);                      /* extern */
// ? DeleteConflict(s32);                              /* extern */
// s32 GetAreaMapLevel(s32, s32, s32, s32, s32);       /* extern */
// ? ReqItemStay(s32 *);                               /* extern */
// s32 abs(s32);                                       /* extern */
// ? memset(s32 *, ?, ?);                              /* extern */
// extern ? D_800121CC;
// extern ? D_8008E404;
// extern s32 GlobalAreaMap;
// extern ? items;
//
// void RestoreItemLayout(void *arg0) {
//     s32 sp18;
//     s32 sp1C;
//     s32 sp20;
//     s32 sp24;
//     s32 sp28;
//     s32 sp30;
//     s32 sp34;
//     s32 sp38;
//     s32 sp3C;
//     s32 sp40;
//     ? (*temp_v0)(? *);
//     ? *var_s0;
//     ? *var_s3;
//     s32 temp_s1;
//     s32 temp_s2;
//     s32 var_s0_2;
//     s32 var_s1;
//     s32 var_s4;
//     s32 var_s6;
//     u8 temp_v0_2;
//     void *var_s5;
//
//     var_s1 = 0;
//     var_s0 = &items;
// loop_1:
//     var_s6 = 0;
//     if (var_s1 < 0x1E) {
//         temp_v0 = var_s0->unkC;
//         if (temp_v0 != NULL) {
//             var_s0->unk54 = 0xFFU;
//             temp_v0(var_s0);
//             DeleteConflict(var_s0->unk10);
//             temp_v0_2 = var_s0->unk54;
//             if (temp_v0_2 != 0) {
//                 AdtMessageBox(&D_800121CC, var_s0->unk8, temp_v0_2);
//             }
//             var_s0->unk0 = 0;
//             var_s0->unkC = NULL;
//         }
//         var_s0 += 0x58;
//         var_s1 += 1;
//         goto loop_1;
//     }
//     var_s5 = arg0;
// loop_8:
//     if (var_s6 < 0x1E) {
//         if (var_s5->unk0 != -1) {
//             memset(&sp30, 0, 0x14);
//             sp30 = var_s5->unk0;
//             sp34 = var_s5->unk4;
//             sp38 = var_s5->unk8;
//             sp3C = var_s5->unkC;
//             sp40 = var_s5->unk10;
//             sp18 = sp30;
//             sp1C = sp34;
//             sp20 = sp38;
//             sp24 = sp3C;
//             sp28 = sp40;
//             var_s0_2 = GetAreaMapLevel(GlobalAreaMap, sp1C, sp20, sp24, 1);
//             var_s4 = 0;
//             if ((var_s0_2 == 0x80000000) || (var_s4 = 0, ((abs(var_s0_2 - sp20) < 0x3E8) == 0))) {
//                 var_s3 = &D_8008E404;
// loop_13:
//                 if (var_s4 < 4) {
//                     temp_s2 = sp1C + (var_s3->unk0 * 0x3E8);
//                     temp_s1 = sp24 + (var_s3->unk2 * 0x3E8);
//                     var_s0_2 = GetAreaMapLevel(GlobalAreaMap, temp_s2, sp20, temp_s1, 1);
//                     if ((var_s0_2 == 0x80000000) || (abs(var_s0_2 - sp20) >= 0x3E8)) {
//                         var_s3 += 4;
//                         var_s4 += 1;
//                         goto loop_13;
//                     }
//                     sp1C = temp_s2;
//                     sp24 = temp_s1;
//                 }
//                 if (var_s4 != 4) {
//                     goto block_19;
//                 }
//             } else {
// block_19:
//                 sp20 = var_s0_2;
//                 ReqItemStay(&sp18);
//             }
//         }
//         var_s5 += 0x14;
//         var_s6 += 1;
//         goto loop_8;
//     }
// }
