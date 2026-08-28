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
 *  - `smode` and `sstage` are the original APPEAR.C static names. Retail
 *    preserves their adjacent halfword layout and their mode-cache/stage-cache
 *    roles despite other globals inserted ahead of them since the demo.
 *  - `resource` gives the two HumanData name stores one shared array base and
 *    is later reused for the StageMotion null-check/free. Keeping one pointer
 *    pseudo is load-bearing: it makes global.c color the early appearance
 *    flag/HumanData base as the target's $v1/$a0. The neutral pointer type is
 *    intentional because those two uses point at unrelated object types.
 */
extern s16 ARMOUR_EQUIPPED_;
extern s16 smode;
extern s16 sstage;
extern u8 str_rikimaua[];                     /* RIKIMAUA */
extern u8 str_ayamea[];                       /* AYAMEA */
extern u8 str_ayames[];                       /* AYAMES */
extern char fmt_motion_stage_amd[];           /* %sMOTION\\STAGE%d.AMD */
extern char path_human[];                     /* K:\\WORK\\CDIMAGE\\HUMAN\\ */
extern char path_human_motion_common_amd[];   /* K:\\WORK\\CDIMAGE\\HUMAN\\MOTION\\COMMON.AMD */
extern char path_human_motion_rikimaru_amd[]; /* K:\\WORK\\CDIMAGE\\HUMAN\\MOTION\\RIKIMARU.AMD */
extern char path_human_motion_ayame_amd[];    /* K:\\WORK\\CDIMAGE\\HUMAN\\MOTION\\AYAME.AMD */
extern int strcmp(const char *a, const char *b);
extern int sprintf(char *dst, const char *fmt, ...);

void SetupAppearance(short mode, short stage)
{
    short i;
    short j;
    u8 name[100];
    u8 *pt;
    void *resource;
    u8 appearance;

    NowStage = stage;
    pt = (u8 *)TENCHU_PERSISTENT_STATE_ADDRESS;
    EngageLevel = 3 - pt[0x58]; /* 3 - gNannido (difficulty) */
    appearance = pt[0x1a];
    if (appearance != 0)
    {
        resource = HumanData;
        ((HumanDataType *)resource)[0].name = str_rikimaua;
        ((HumanDataType *)resource)[1].name =
            appearance != 0xff ? str_ayamea : str_ayames;
        *(u8 *)(TENCHU_PERSISTENT_STATE_ADDRESS + 0x1a) = 0;
        ARMOUR_EQUIPPED_ = -1;
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
        resource = StageMotion;
        if (resource != 0)
        {
            vfree(resource);
            StageMotion = 0;
        }
    }
    else
    {
        if (StageMotion != 0)
        {
            vfree(StageMotion);
        }
        sstage = stage;
        sprintf((char *)name, fmt_motion_stage_amd, path_human, (int)stage);
        StageMotion = LoadMotion(FileRead(name));
        if (stage != 0)
        {
            if (CommonMotion == 0)
            {
                CommonMotion = LoadMotion(FileRead((u8 *)path_human_motion_common_amd));
                SetupMotionRegist(MOTcommon);
            }
            if (PlayerMotion != 0)
            {
                if (mode != smode)
                {
                    vfree(PlayerMotion);
                    PlayerMotion = 0;
                }
                if (PlayerMotion != 0)
                {
                    return;
                }
            }
            smode = mode;
            if (mode == 0)
            {
                pt = (u8 *)path_human_motion_rikimaru_amd;
            }
            else
            {
                pt = (u8 *)path_human_motion_ayame_amd;
            }
            PlayerMotion = LoadMotion(FileRead(pt));
        }
    }
}
