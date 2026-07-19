#include "common.h"
#include "main.exe.h"
#include <psxsdk/libcd.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void cbCheckCD(void);
 *     OPAUDIO.C:69, 55 src lines, frame 48 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     stack sp+16     unsigned char [8] result
 *     reg   $s0       int ret
 *     reg   $v1       int com
 *     stack sp+24     struct CdlLOC loc
 *     stack sp+24     struct CdlLOC loc
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCdaStatus CdaStatus;
 * END PSX.SYM */

/*
 * STATUS: MATCHING — 488 bytes / 122 instructions.
 *
 * cbCheckCD is the VSync callback that advances CD-audio state, retries or
 * stops out-of-range playback, and updates CdaStatus from the drive result.
 * The switch cases stay in source order 5 then 2, and the case-5 path shares
 * its status-reset tail with the mode-1 range check.
 *
 * The final cross-jump requires two source-level CdControlF(0x11, NULL)
 * calls in the `com == 0x11` if/else. CSE and the first jump pass retain both
 * calls and therefore both a0/a1 materializations. Late delay-branch cleanup
 * merges only the calls, leaving the target's explicit jump and repeated
 * argument setup. This is the same zero-code identical-call barrier used by
 * cbAccess.
 *
 * The target's 0x10-byte working stack window overlays `result` with the two
 * original same-named CdlLOC scopes: the first view begins at sp+24 and the
 * second at sp+21. CdaCheckScratch records that overlap explicitly without
 * changing either access or the 56-byte frame.
 */

typedef struct
{
    s32 StartPos;   /* 0x00 */
    s32 CurPos;     /* 0x04 */
    s32 EndPos;     /* 0x08 */
    s16 mode;       /* 0x0C */
    s16 CheckCount; /* 0x0E */
    u8 status;      /* 0x10 */
    u8 voll;        /* 0x11 */
    u8 volr;        /* 0x12 */
    u8 flag;        /* 0x13 */
    u8 field9_0x14; /* 0x14 */
} TCdaStatus;

extern TCdaStatus CdaStatus;

typedef union
{
    struct
    {
        u8 result[8];
        CdlLOC loc;
    } first;
    struct
    {
        u8 pad[5];
        CdlLOC loc;
    } second;
} CdaCheckScratch;

extern int CdLastCom(void);
extern void SsSetSerialAttr(u8 a, u8 b, u8 c);
extern void SsSetSerialVol(u8 a, u8 voll, u8 volr);
extern void cd_control(u8 param_1, u8 *param_2, u8 *param_3);

void cbCheckCD(void)
{
    TCdaStatus *cs = &CdaStatus;
    CdaCheckScratch scratch;
    s32 ret;
    s32 com;

    if (cs->field9_0x14 == 0x1B) {
        CdIntToPos(CdaStatus.StartPos, &scratch.first.loc);
        if ((cs->flag & 1) &&
            CdControl(0x1B, (u8 *)&scratch.first.loc, NULL) == 0) {
            return;
        }
        cs->field9_0x14 = 0;
        SsSetSerialAttr(0, 0, 1);
        SsSetSerialVol(0, cs->voll, cs->volr);
        return;
    }

    if (cs->CheckCount++ < 0xA) {
        return;
    }
    cs->CheckCount = 0;

    ret = CdSync(1, scratch.first.result);
    com = CdLastCom();
    switch (ret) {
    case 5:
        cs->field9_0x14 = 0x1B;
        goto shared_tail;
    case 2:
        if (com == 9) {
            return;
        }
        if (com == 0x11) {
            cs->CurPos = CdPosToInt(&scratch.second.loc);
            if ((cs->status & 0x20) &&
                (cs->EndPos < cs->CurPos || cs->CurPos < CdaStatus.StartPos - 300)) {
                if (cs->mode != 1) {
                    goto mode_error;
                }
                cs->field9_0x14 = 0x1B;
            shared_tail:
                cs->CheckCount = 0;
                cs->status = 0;
                return;

            mode_error:
                SsSetSerialAttr(0, 0, 1);
                SsSetSerialVol(0, 0, 0);
                cd_control(9, 0, 0);
                cs->status = 0;
                return;
            }
            CdControl(1, NULL, scratch.first.result);
            CdaStatus.status = scratch.first.result[0];
            CdControlF(0x11, NULL);
        } else {
            CdControlF(0x11, NULL);
        }
        break;
    }
}

// Ghidra decompilation reference (retained for type/control-flow provenance):
//
//
// void cbCheckCD(void)
//
// {
//   int iVar1;
//   int iVar2;
//   uchar local_28 [5];
//   undefined1 auStack_23 [11];
//
//   if (CdaStatus.field9_0x14 == '\x1b') {
//     CdIntToPos(CdaStatus.StartPos,(CdlLOC *)(auStack_23 + 3));
//     if (((CdaStatus.flag & 1) != 0) &&
//        (iVar1 = CdControl('\x1b',(u_char *)(auStack_23 + 3),(u_char *)0x0), iVar1 == 0)) {
//       return;
//     }
//     CdaStatus.field9_0x14 = 0;
//     SsSetSerialAttr('\0','\0','\x01');
//     SsSetSerialVol('\0',(ushort)CdaStatus.field6_0x11,(ushort)CdaStatus.field7_0x12);
//     return;
//   }
//   if (CdaStatus.CheckCount < 10) {
//     CdaStatus.CheckCount = CdaStatus.CheckCount + 1;
//     return;
//   }
//   CdaStatus.CheckCount = 0;
//   iVar1 = CdSync(1,local_28);
//   iVar2 = CdLastCom();
//   if (iVar1 == 2) {
//     if (iVar2 == 9) {
//       return;
//     }
//     if (iVar2 == 0x11) {
//       CdaStatus.CurPos = CdPosToInt((CdlLOC *)auStack_23);
//       if (((CdaStatus.status & 0x20) != 0) &&
//          ((CdaStatus.EndPos < CdaStatus.CurPos || (CdaStatus.CurPos < CdaStatus.StartPos + -300))))
//       {
//         if (CdaStatus.mode != 1) {
//           SsSetSerialAttr('\0','\0','\x01');
//           SsSetSerialVol('\0',0,0);
//           cd_control(9,0,0);
//           CdaStatus.status = '\0';
//           return;
//         }
//         goto LAB_8004f90c;
//       }
//       CdControl('\x01',(u_char *)0x0,local_28);
//       CdaStatus.status = local_28[0];
//     }
//     CdControlF('\x11',(u_char *)0x0);
//   }
//   else {
//     if (iVar1 != 5) {
//       return;
//     }
// LAB_8004f90c:
//     CdaStatus.field9_0x14 = '\x1b';
//     CdaStatus.CheckCount = 0;
//     CdaStatus.status = '\0';
//   }
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// s32 CdControl(?, ? *, u8 *);                        /* extern */
// ? CdControlF(?, ?);                                 /* extern */
// ? CdIntToPos(s32, ? *);                             /* extern */
// s32 CdLastCom();                                    /* extern */
// s32 CdPosToInt(? *);                                /* extern */
// s32 CdSync(?, u8 *);                                /* extern */
// ? SsSetSerialAttr(?, ?, ?);                         /* extern */
// ? SsSetSerialVol(?, u8, u8);                        /* extern */
// ? cd_control(?, ?, ?);                              /* extern */
// extern ? CdaStatus;
// extern u8 D_8008EA50;
//
// void cbCheckCD(void) {
//     u8 sp10;
//     ? sp15;
//     ? sp18;
//     s32 temp_s1;
//     s32 temp_v0;
//     s32 temp_v1;
//
//     if (CdaStatus.unk14 == 0x1B) {
//         CdIntToPos(CdaStatus.unk0, &sp18);
//         if (!(CdaStatus.unk13 & 1) || (CdControl(0x1B, &sp18, NULL) != 0)) {
//             CdaStatus.unk14 = 0U;
//             SsSetSerialAttr(0, 0, 1);
//             SsSetSerialVol(0, CdaStatus.unk11, CdaStatus.unk12);
//         }
//     } else {
//         CdaStatus.unkE = (u16) (CdaStatus.unkE + 1);
//         if ((s16) CdaStatus.unkE >= 0xA) {
//             CdaStatus.unkE = 0U;
//             temp_s1 = CdSync(1, &sp10);
//             temp_v1 = CdLastCom();
//             switch (temp_s1) {                      /* switch 1; irregular */
//             case 5:                                 /* switch 1 */
//                 CdaStatus.unk14 = 0x1BU;
// block_15:
//                 CdaStatus.unkE = 0U;
//                 CdaStatus.unk10 = 0U;
//                 return;
//             case 2:                                 /* switch 1 */
//                 switch (temp_v1) {                  /* switch 2; irregular */
//                 case 17:                            /* switch 2 */
//                     temp_v0 = CdPosToInt(&sp15);
//                     CdaStatus.unk4 = temp_v0;
//                     if (CdaStatus.unk10 & 0x20) {
//                         if ((CdaStatus.unk8 < temp_v0) || (temp_v0 < (CdaStatus.unk0 - 0x12C))) {
//                             if (CdaStatus.unkC == 1) {
//                                 CdaStatus.unk14 = 0x1BU;
//                                 goto block_15;
//                             }
//                             SsSetSerialAttr(0, 0, 1);
//                             SsSetSerialVol(0, 0U, 0U);
//                             cd_control(9, 0, 0);
//                             CdaStatus.unk10 = 0U;
//                             return;
//                         }
//                         goto block_18;
//                     }
// block_18:
//                     CdControl(1, NULL, &sp10);
//                     D_8008EA50 = sp10;
//                 default:                            /* switch 2 */
//                     CdControlF(0x11, 0);
//                     break;
//                 }
//                 break;
//             }
//         } else {
//         case 9:                                     /* switch 2 */
//         }
//     }
// }
