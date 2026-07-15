#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short CVAupdate(void);
 *     CHRANIM.C:145, 89 src lines, frame 56 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     stack sp+24     struct SVECTOR vect
 *     reg   $s1       struct Humanoid * human
 *     reg   $s0       long i
 *
 * Globals it touches, as the original declared them:
 *     extern struct CVAType *CVAnow;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct SVECTOR UnitVector;
 *     extern struct Humanoid *StagePlayer;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct Humanoid *CameraTarget;
 *     extern short CameraPanMode;
 *     extern struct CVAType *CVAdata;
 *     extern int StageID;
 * END PSX.SYM */

/*
 * CVAupdate (0x80050628) — interpret character-animation and camera events.
 *
 * STATUS: NON_MATCHING — full guarded semantic reconstruction. The draft is
 * exact-length at 2104/2104 bytes and 526/526 instructions, with fuzzy 95.44
 * (up from 89.14), 29 linked differing bytes, and 24 raw aligned lines in 13
 * blocks. Structural filtering is empty and all 19 calls match; the remaining
 * differences are register allocation only. Build with
 * `NON_MATCHING=CVAupdate ./Build`. On a full match, delete the guards and the
 * _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAupdate", CVAupdate);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAupdate", switchD_80050694__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAupdate", switchD_80050694__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAupdate", switchD_80050694__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAupdate", switchD_80050694__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAupdate", switchD_80050694__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAupdate", switchD_80050694__caseD_5);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAupdate", switchD_80050694__caseD_6);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAupdate", switchD_80050694__caseD_7);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAupdate", switchD_80050694__caseD_8);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAupdate", switchD_80050694__caseD_1);

/* jump-table pool @ 0x80013664 (9 words; tables at 0x80013664) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 CVAupdate_jtbl[9] = {
    0x8005069C, 0x80050E04, 0x800506C4, 0x80050954,
    0x80050B44, 0x80050B5C, 0x80050C40, 0x80050C6C,
    0x80050D14,
};

#else /* NON_MATCHING */

typedef struct
{
    s16 kind;
    s16 mode;
    s16 x;
    s16 y;
    s16 z;
    s16 param;
} CVAEvent;

typedef struct
{
    Humanoid *human;
    s16 loop;
    s16 motid;
} HumanAnimType;

typedef struct
{
    long vpx, vpy, vpz;
    long vrx, vry, vrz;
    long rz;
    void *super;
} GsRVIEW2;

typedef struct
{
    u8 pad[0x5A];
    u16 attribute;
} PositionalEntry;

extern CVAEvent *CVAnow;
extern u8 *CVAdata;
extern HumanAnimType CVAhuman[5];
extern GsRVIEW2 ViewInfo;
extern SVECTOR UnitVector;
extern Humanoid *StagePlayer;
extern u32 *GlobalAreaMap;
extern Humanoid *CameraTarget;
extern s16 CameraPanMode;
extern s16 D_80097CC8;
extern s16 D_80097CCC;
extern u8 D_800C2C50[];
extern u8 D_8008FFB9[];
extern u8 CHOSEN_CHARACTER;
extern s32 StageID;
extern PositionalEntry *TENCHU_POSITIONAL_DATA_AREA_[6];

extern Humanoid *GetHumanoid(s16 type);
extern s16 SetNowMotion(Humanoid *human, s16 mid, s16 move);
extern s16 PlayMotion(MotionManager *motion, s16 mode);
extern s32 ReqLifeBar(Humanoid *human);
extern void CdaStop(void);
extern long GetAreaMapLevel(void *area, long x, long y, long z, int mode);
extern void AVCameraSetup(void);
extern void GsSetRefView2(GsRVIEW2 *view);
extern void SetBlood(VECTOR *pos, s16 n, s16 time);
extern void FUN_80038fdc(u8 arg0, u8 arg1, u8 arg2, long arg3);
extern char *strcpy(char *dst, const char *src);
extern void SetupTelop(u8 *telop, s16 line);

s16 CVAupdate(void)
{
    Humanoid *human;
    HumanAnimType *anim;
    HumanAnimType *anim_base;
    ModelArchiveType *model;
    CVAEvent *event;
    CVAEvent *cursor;
    VECTOR vect;
    long level;
    long y;
    long delta;
    long position;
    s32 i;
    s32 active_status;
    s32 invalid;
    s32 pan_value;
    u32 packed;
    s16 value;
    u8 ch;

    cursor = CVAnow;
    if (cursor->kind != 1)
    {
        invalid = -1;
        anim_base = CVAhuman;
        do
        {
            switch (cursor->kind)
            {
            case 0:
                if (CVAnow->param == invalid)
                    CdaStop();
                break;

            case 2:
                human = GetHumanoid(CVAnow->mode);
                if (human == 0)
                    return 0;
                i = 0;

                human->attribute &= 0xFF7F;
                human->motion->mask = 0x7FFF;
                anim = anim_base;
            scan_case2_empty:
                if (anim->human == 0)
                    goto scan_case2_done;
                i++;
                if (i < 5)
                {
                    anim++;
                    goto scan_case2_empty;
                }
            scan_case2_done:
                if (i == 5)
                    SetNowMotion(human, 0x501, 1);

                human->vector = UnitVector;
                human->model->object[0]->attribute |= 0x4000;
                model = human->model;
                i = 0;
                if (model->n > 0)
                {
                    do
                    {
                        model->object[i]->attribute &= 0xFFFE;
                        i++;
                    } while (i < model->n);
                }

                if (StagePlayer != human && human->life == invalid)
                {
                    human->attribute |= 4;
                    human->life = human->lifemax;
                }

                if (CVAnow->param != invalid)
                {
                    position = CVAnow->x * 1000;
                    human->point[0] = position;
                    human->locate->vx = position;
                    position = CVAnow->z * 1000;
                    human->point[1] = position;
                    human->locate->vz = position;
                    y = CVAnow->y * 1000;
                    level = GetAreaMapLevel(GlobalAreaMap, human->locate->vx,
                                            y - 1000, human->locate->vz, 0);
                    human->locate->vy = level;
                    if (y < human->locate->vy || human->locate->vy == (long)0x80000000)
                        human->locate->vy = y;
                    human->rotate->vy = CVAnow->param;
                    UpdateCoordinate((ModelType *)human->model);
                    delta = human->locate->vy - StagePlayer->locate->vy;
                    if (delta < 0)
                        delta = -delta;
                    if (delta > 20000)
                        human->attribute |= 0x80;
                }
                break;

            case 3:
                human = GetHumanoid(CVAnow->mode);
                if (human == 0)
                    return 0;

                packed = (u32)(u16)CVAnow->x << 16;
                active_status = 0x11;
                if ((s32)packed >> 16 == invalid)
                {
                    human->life = invalid;
                    human->attribute = (human->attribute | 0x82) & 0xFFFB;
                    human->motion->mid = invalid;
                    SetNowMotion(human, 0, 1);
                    PlayMotion(human->motion, 1);
                    human->motion->count--;
                }
                else
                {
                    i = (s32)packed >> 24;
                    if (human->status == active_status && (u32)(i - 0x10) > 1)
                        return 0;
                    if (human->life > 0)
                    {
                        if (i == 0x11)
                        {
                            human->life = 0;
                            ReqLifeBar(human);
                        }
                        i = 0;
                    }
                    else
                    {
                        i = 0;
                    }

                    anim = anim_base;
                scan_case3_human:
                    if (anim->human == human)
                        goto scan_case3_human_done;
                    i++;
                    if (i < 5)
                    {
                        anim++;
                        goto scan_case3_human;
                    }
                scan_case3_human_done:
                    if (i == 5)
                    {
                        i = 0;
                        anim = anim_base;
                    scan_case3_empty:
                        if (anim->human == 0)
                            goto scan_case3_empty_done;
                        i++;
                        if (i < 5)
                        {
                            anim++;
                            goto scan_case3_empty;
                        }
                    scan_case3_empty_done:
                        if (i == 5)
                            return 0;
                    }

                    human->motion->mid = invalid;
                    SetNowMotion(human, CVAnow->x, 1);
                    PlayMotion(human->motion, 1);
                    human->motion->count--;
                    anim_base[i].human = human;

                    value = CVAnow->y;
                    if (CVAnow->y < 1)
                        value = 0x7FFF;
                    anim_base[i].loop = value;
                    value = CVAnow->z;
                    if (CVAnow->z == 0)
                        value = 0x501;
                    anim_base[i].motid = value;

                    if (human->type == 0xA7 && CVAnow->x == 0x1100)
                        SoundEx(0, 0x41);
                }
                break;

            case 4:
                AVCameraSetup();
                D_80097CCC = 1;
                break;

            case 5:
                if (CVAnow->mode == 0)
                {
                    ViewInfo.vrx = CVAnow->x * 100;
                    ViewInfo.vry = CVAnow->y * 100;
                    ViewInfo.vrz = CVAnow->z * 100;
                }
                else
                {
                    human = GetHumanoid(CVAnow->param);
                    if (human == 0)
                        return 0;
                    ViewInfo.vrx = human->locate->vx;
                    ViewInfo.vry = human->locate->vy - human->height + 300;
                    ViewInfo.vrz = human->locate->vz;
                    CameraTarget = human;
                }
                GsSetRefView2(&ViewInfo);
                break;

            case 6:
                CameraPanMode = (u16)CVAnow->mode;
                pan_value = 20;
                if (CVAnow->param != 0)
                    pan_value = CVAnow->param;
                D_80097CC8 = pan_value;
                break;

            case 7:
                event = CVAnow;
                vect.vx = event->x * 10;
                vect.vy = event->y * 10;
                vect.vz = event->z * 10;
                switch (event->mode)
                {
                case 1:
                    SetBlood(&vect, event->param, 30);
                    break;
                case 3:
                    FUN_80038fdc((u8)event->x, (u8)event->y,
                                 (u8)event->z, event->param);
                    break;
                }
                break;

            case 8:
                if (CVAnow->mode != invalid)
                {
                    SetupTelop((u8 *)strcpy((char *)D_800C2C50,
                                            (char *)CVAdata + CVAnow->mode), 0);
                    D_80097CCC = 1;
                    if (StageID != 10 || CHOSEN_CHARACTER != 0)
                        break;

                    ch = D_800C2C50[0];
                    if ((D_8008FFB9[ch] & 4) == 0)
                        break;

                    i = ch - '0';
                    if (i == 0)
                    {
                        do
                        {
                            TENCHU_POSITIONAL_DATA_AREA_[i]->attribute |= 1;
                            i++;
                        } while (i < 6);
                    }
                    else
                    {
                        TENCHU_POSITIONAL_DATA_AREA_[ch - '1']->attribute &= 0xFFFE;
                    }
                }
                D_800C2C50[0] = 0;
                break;
            }

            CVAnow++;
            cursor = CVAnow;
        } while (cursor->kind != 1);
    }

    return CVAnow->mode != 0;
}

#endif /* NON_MATCHING */
