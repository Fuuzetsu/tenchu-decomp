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
 * END PSX.SYM */

/*
 * The decisive reconstruction was aggregate syntax: assigning the 16-byte
 * VECTOR into `tmp.locate` makes cc1 emit the target's batched t1-t4 load/store
 * copy, while the following whole PARAM_ITEM_STAY assignment emits its second
 * batched copy.  The late `search_success` trampoline reproduces the target's
 * success-only vx/vz stores and jump back to the shared k==4 check without
 * cloning ReqItemStay.  The three one-shot loops around the final level/x/z
 * stores are zero-code global-allocation weights: they order level, z, x,
 * offs, and k into the target s0-s4 homes while leaving scheduling intact.
 */


/* The [4] bound is LOAD-BEARING CODEGEN, not a claim about the table's length:
 * the search loop below walks 4 entries x 2 shorts = 8 shorts past this symbol.
 * The target materializes this address as `lui s3,%hi; addiu s3,s3,%lo` -- ONE
 * register.  That is an UNSPLIT `la` macro, not a high/lo_sum pair: cc1 splits
 * an address pre-reload into two *distinct* pseudos (`lui $tmp,%hi; addiu
 * $dst,$tmp,%lo`), and local-alloc's combine_regs refuses to tie them whenever
 * the destination is a multi-block pseudo -- which a loop cursor always is.  The
 * split is declined only when mips_check_split() sees SYMBOL_REF_FLAG, which
 * ENCODE_SECTION_INFO (mips.h) sets iff the symbol's declared type is COMPLETE
 * and 0 < sizeof <= the -G threshold (8).  `extern short D_8008E404[];` is an
 * incomplete type (size -1), so it always split.  Widening this bound past [4]
 * re-splits the address and costs 3 bytes.  See docs/matching-cookbook.md.
 */
extern short D_8008E404[4];

extern s32 abs(s32 x);
extern void *memset(void *s, int c, u32 n);

void RestoreItemLayout(void *buf)
{
    TItem *it;
    TItemLayout *slot;
    s32 i;
    s32 j;
    PARAM_ITEM_STAY param;
    s32 level;
    s32 one;
    s32 sentinel;
    s32 x;
    s32 z;

    i = 0;
    it = items;
loop1:
    if (!(i < 0x1e))
        goto loop1_end;
    if (it->proc != 0) {
        it->mode = ITEM_MODE_DISPOSE;
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
    slot = buf;
loop2:
    if (!(j < 0x1e))
        return;
    if (slot->type != -1) {
        PARAM_ITEM_STAY tmp;

        memset(&tmp, 0, sizeof(PARAM_ITEM_STAY));
        tmp.type = slot->type;
        tmp.locate = slot->locate;
        param = tmp;

        level = GetAreaMapLevel(GlobalAreaMap, param.locate.vx, param.locate.vy, param.locate.vz, one);
        if (level == sentinel || abs(level - param.locate.vy) >= 0x3e8) {
            s32 k = 0;
            short *offs = D_8008E404;

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
                goto search_success;
            }
        search_check:
            if (k == 4) {
                goto skip_stay;
            }
        }
        do {
            param.locate.vy = level;
        } while (0);
        ReqItemStay(&param);
    skip_stay:;
    }
    slot++;
    j++;
    goto loop2;

search_success:
    do {
        param.locate.vx = x;
    } while (0);
    do {
        param.locate.vz = z;
    } while (0);
    goto search_check;
}

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
