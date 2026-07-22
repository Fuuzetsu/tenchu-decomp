#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupAppearance(short mode, short stage);
 *     APPEAR.C:109, 60 src lines, frame 160 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
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
 *     param $s7       short mode
 *     param $fp       short stage
 *     reg   $s2       short i
 *     reg   $s1       short j
 *     stack sp+16     unsigned char [100] name
 *     reg   $a0       unsigned char * pt
 *
 * Globals it touches, as the original declared them:
 *     extern short NowStage;
 *     extern unsigned char gNannido;
 *     extern short EngageLevel;
 *     extern struct HumanDataType HumanData[63];
 *     extern struct WeaponModelType WeaponModel[41];
 *     extern struct MotionPackType *CommonMotion;
 *     extern struct MotionPackType *PlayerMotion;
 *     extern struct MotionPackType *StageMotion;
 * END PSX.SYM */

/*
 * SetupAppearance (0x80029aa4, 0x400 bytes) — frees the current appearance
 * resources and reloads the stage/player motion packs. The disassembly is
 * split at an internal PRT marker, but both pieces are one C function.
 *
 * Matching notes:
 *  - `pt = (u8 *)0x80010000` is the recovered `unsigned char *pt` local. It
 *    keeps the PersistentState base in one register for the +0x58/+0x1a
 *    reads; the later literal +0x1a clear must rematerialize the address
 *    after `pt` is repurposed.
 *  - `human` gives the two HumanData name stores one shared array base.
 *  - Reusing `human` for the much later StageMotion null-check/free is
 *    load-bearing: it changes global.c's pseudo coloring so the early
 *    appearance flag/HumanData base land in the target's $v1/$a0. Without
 *    that semantically equivalent reuse, the complete 256-instruction
 *    function differs only by a six-byte $a0/$v1 swap.
 */
extern s16 NowStage;
extern s16 PLAYER_REDUCE_DAMAGE_DUE_TO_ARMOUR;
extern s16 AMD_LOADED_FOR_CHARACTER_KIND;
extern s16 D_800979A6;
extern u8 D_80011710[];
extern u8 D_800979A8[];
extern u8 D_800979B0[];
extern char D_8001171C[];
extern char D_80011734[];
extern char D_8001174C[];
extern char D_80011774[];
extern char D_800117A0[];
extern int strcmp(const char *a, const char *b);
extern int sprintf(char *dst, const char *fmt, ...);

void SetupAppearance(short mode, short stage)
{
    short i;
    short j;
    u8 name[100];
    u8 *pt;
    HumanDataType *human;
    u8 appearance;

    NowStage = stage;
    pt = (u8 *)TENCHU_PERSISTENT_STATE_ADDRESS;
    EngageLevel = 3 - pt[0x58]; /* 3 - gNannido (difficulty) */
    appearance = pt[0x1a];
    if (appearance != 0)
    {
        human = HumanData;
        human[0].name = D_80011710;
        human[1].name = appearance != 0xff ? D_800979A8 : D_800979B0;
        *(u8 *)(TENCHU_PERSISTENT_STATE_ADDRESS + 0x1a) = 0;
        PLAYER_REDUCE_DAMAGE_DUE_TO_ARMOUR = -1;
    }

    i = 0;
    while (HumanData[i].type != -1)
    {
        if (HumanData[i].model != 0)
        {
            vfree(HumanData[i].model);
            HumanData[i].model = 0;
            j = 0;
            while (HumanData[j].type != -1)
            {
                if (strcmp((char *)HumanData[i].name,
                           (char *)HumanData[j].name) == 0)
                {
                    HumanData[j].model = 0;
                }
                j++;
            }
        }
        HumanData[i].model = 0;
        HumanData[i].mtbl->motion = 0;
        i++;
    }

    i = 0;
    while (WeaponModel[i].wid != -1)
    {
        if (WeaponModel[i].model != 0)
        {
            vfree(WeaponModel[i].model);
        }
        WeaponModel[i].model = 0;
        i++;
    }

    if (stage < 0)
    {
        if (CommonMotion != 0)
        {
            vfree(CommonMotion);
            CommonMotion = 0;
        }
        if (PlayerMotion != 0)
        {
            vfree(PlayerMotion);
            PlayerMotion = 0;
        }
        human = (HumanDataType *)StageMotion;
        if (human != 0)
        {
            vfree(human);
            StageMotion = 0;
        }
    }
    else
    {
        if (StageMotion != 0)
        {
            vfree(StageMotion);
        }
        D_800979A6 = stage;
        sprintf((char *)name, D_8001171C, D_80011734, (int)stage);
        StageMotion = LoadMotion(FileRead(name));
        if (stage != 0)
        {
            if (CommonMotion == 0)
            {
                CommonMotion = LoadMotion(FileRead((u8 *)D_8001174C));
                SetupMotionRegist(MOTcommon);
            }
            if (PlayerMotion != 0)
            {
                if (mode != AMD_LOADED_FOR_CHARACTER_KIND)
                {
                    vfree(PlayerMotion);
                    PlayerMotion = 0;
                }
                if (PlayerMotion != 0)
                {
                    return;
                }
            }
            AMD_LOADED_FOR_CHARACTER_KIND = mode;
            if (mode == 0)
            {
                pt = (u8 *)D_80011774;
            }
            else
            {
                pt = (u8 *)D_800117A0;
            }
            PlayerMotion = LoadMotion(FileRead(pt));
        }
    }
}

// triage: HARD — 256 insns, 3 loop, 6 callees, ~0.06 to SetupMotionRegist
// likely-relevant cookbook sections:
//   - Loops: 3 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void SetupAppearance(short mode,short stage)
//
// {
//   char *pcVar1;
//   int iVar2;
//   ulong *puVar3;
//   short sVar4;
//   short sVar5;
//   MotionRegistType *pMVar6;
//   int iVar7;
//   int iVar8;
//   char acStack_90 [104];
//
//   EngageLevel = 3 - (ushort)(byte)PersistentState._88_1_;
//   if (PersistentState._26_1_ != '\0') {
//     HumanData[0].name = "RIKIMAUA";
//     if (PersistentState._26_1_ == -1) {
//       pcVar1 = s_AYAMES_800979b0;
//     }
//     else {
//       pcVar1 = s_AYAMEA_800979a8;
//     }
//     PersistentState._26_1_ = 0;
//     DAT_800979a2 = 0xffff;
//     HumanData[1].name = (uchar *)pcVar1;
//   }
//   iVar8 = 0;
//   NowStage = stage;
//   sVar4 = HumanData[0].type;
//   while (sVar4 != -1) {
//     sVar4 = (short)iVar8;
//     if (HumanData[sVar4].model != (ulong *)0x0) {
//       iVar7 = 0;
//       vfree((undefined *)HumanData[sVar4].model);
//       HumanData[sVar4].model = (ulong *)0x0;
//       sVar5 = HumanData[0].type;
//       while (sVar5 != -1) {
//         sVar5 = (short)iVar7;
//         iVar2 = strcmp((char *)HumanData[sVar4].name,(char *)HumanData[sVar5].name);
//         iVar7 = iVar7 + 1;
//         if (iVar2 == 0) {
//           HumanData[sVar5].model = (ulong *)0x0;
//         }
//         sVar5 = HumanData[iVar7 * 0x10000 >> 0x10].type;
//       }
//     }
//     pMVar6 = HumanData[sVar4].mtbl;
//     iVar8 = iVar8 + 1;
//     HumanData[sVar4].model = (ulong *)0x0;
//     pMVar6->motion = (MotionDataType *)0x0;
//     sVar4 = HumanData[iVar8 * 0x10000 >> 0x10].type;
//   }
//   iVar8 = 0;
//   if (WeaponModel[0].wid != -1) {
//     iVar7 = 0;
//     do {
//       if (WeaponModel[iVar7 >> 0x10].model != (ulong *)0x0) {
//         vfree((undefined *)WeaponModel[iVar7 >> 0x10].model);
//       }
//       iVar8 = iVar8 + 1;
//       WeaponModel[iVar7 >> 0x10].model = (ulong *)0x0;
//       iVar7 = iVar8 * 0x10000;
//     } while (WeaponModel[iVar8 * 0x10000 >> 0x10].wid != -1);
//   }
//   iVar8 = (int)stage;
//   if (iVar8 < 0) {
//     if (CommonMotion != (MotionPackType *)0x0) {
//       vfree((undefined *)CommonMotion);
//       CommonMotion = (MotionPackType *)0x0;
//     }
//     if (PlayerMotion != (MotionPackType *)0x0) {
//       vfree((undefined *)PlayerMotion);
//       PlayerMotion = (MotionPackType *)0x0;
//     }
//     if (StageMotion != (MotionPackType *)0x0) {
//       vfree((undefined *)StageMotion);
//       StageMotion = (MotionPackType *)0x0;
//     }
//   }
//   else {
//     if (StageMotion != (MotionPackType *)0x0) {
//       vfree((undefined *)StageMotion);
//     }
//     DAT_800979a6 = stage;
//     sprintf(acStack_90,"%sMOTION\\STAGE%d.AMD","K:\\WORK\\CDIMAGE\\HUMAN\\",iVar8);
//     puVar3 = FileRead(acStack_90);
//     StageMotion = LoadMotion(puVar3);
//     if (iVar8 != 0) {
//       if (CommonMotion == (MotionPackType *)0x0) {
//         puVar3 = FileRead("K:\\WORK\\CDIMAGE\\HUMAN\\MOTION\\COMMON.AMD");
//         CommonMotion = LoadMotion(puVar3);
//         SetupMotionRegist(MOTcommon);
//       }
//       if (PlayerMotion != (MotionPackType *)0x0) {
//         if (mode != DAT_800979a4) {
//           vfree((undefined *)PlayerMotion);
//           PlayerMotion = (MotionPackType *)0x0;
//         }
//         if (PlayerMotion != (MotionPackType *)0x0) {
//           return;
//         }
//       }
//       if (mode == 0) {
//         pcVar1 = "K:\\WORK\\CDIMAGE\\HUMAN\\MOTION\\RIKIMARU.AMD";
//       }
//       else {
//         pcVar1 = "K:\\WORK\\CDIMAGE\\HUMAN\\MOTION\\AYAME.AMD";
//       }
//       DAT_800979a4 = mode;
//       puVar3 = FileRead(pcVar1);
//       PlayerMotion = LoadMotion(puVar3);
//     }
//   }
//   return;
// }
