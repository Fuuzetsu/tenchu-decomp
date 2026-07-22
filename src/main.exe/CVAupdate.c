#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
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
 * Byte-matching. Jump-table function (switch on the event kind).
 *
 * Four register-allocation ties were closed by removing draft-only locals; each
 * one is a cc1-2.8.1 mechanism worth knowing (see the commits for the RTL/pinned
 * -source evidence):
 *   - `y = CVAnow->y * 1000`: a dedicated `y` local made the last shift write a
 *     BLOCK-LOCAL temp whose copy to `y` sched1 sank past the GetAreaMapLevel
 *     call, so local-alloc's combine_regs tied the whole x1000 chain into one
 *     call-crossing quantity and forced it callee-saved. combine_regs refuses to
 *     tie into a pseudo that is not block-local, so reusing the long-lived `i`
 *     (PSX.SYM: `long i` in $s0, provably dead here) keeps the chain in $v0.
 *   - `active_status = 0x11`: an explicit local spanned the enclosing `if`,
 *     making the constant GLOBAL, so local-alloc gave block-local `human->status`
 *     $v0 and global-alloc took $v1 — the inverse of the target. Inlining the
 *     literal keeps the constant local to the else-block.
 *   - `delta`: `if (delta < 0) delta = -delta;` tied each load into its dying
 *     base's quantity; `__builtin_abs` expands via abssi2 and unties them.
 *   - `anim`: ONE cursor across case 2 and case 3 is one pseudo, hence one hard
 *     register everywhere ($a0). Case 2's preheader overlaps `human->motion` in
 *     $v1 and so must take $a0, and that choice was being carried into case 3,
 *     where the target uses $v1. Case 3 needs its OWN cursor (`slot`).
 *
 * PSX.SYM's three-locals record (vect, human, i) was the through-line: every one
 * of these was a draft-invented local that the original did not have.
 */


extern s16 D_80097CCC;
extern u8 D_800C2C50[];
extern u8 D_8008FFB9[];
extern u8 CHOSEN_CHARACTER;
extern Sprite3D *TENCHU_POSITIONAL_DATA_AREA_[6];

extern s16 PlayMotion(MotionManager *motion, s16 mode);
extern int ReqLifeBar(Humanoid *h);
extern void CdaStop(void);
extern void AVCameraSetup(void);
extern void SetBlood(VECTOR *pos, s16 n, s16 time);
extern void FUN_80038fdc(u8 arg0, u8 arg1, u8 arg2, long arg3);
extern char *strcpy(char *dst, const char *src);
extern void SetupTelop(u8 *telop, s16 line);

s16 CVAupdate(void)
{
    Humanoid *human;
    HumanAnimType *anim;
    HumanAnimType *slot;
    HumanAnimType *anim_base;
    ModelArchiveType *model;
    CVAType *event;
    CVAType *cursor;
    VECTOR vect;
    long level;
    long delta;
    long position;
    s32 i;
    s32 invalid;
    s32 pan_value;
    u32 packed;
    s16 value;
    u8 ch;

    cursor = CVAnow;
    if (cursor->mode != 1)
    {
        invalid = -1;
        anim_base = CVAhuman;
        do
        {
            switch (cursor->mode)
            {
            case 0:
                if (CVAnow->p == invalid)
                    CdaStop();
                break;

            case 2:
                human = GetHumanoid(CVAnow->id);
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

                if (CVAnow->p != invalid)
                {
                    position = CVAnow->x * 1000;
                    human->point[0] = position;
                    human->locate->vx = position;
                    position = CVAnow->z * 1000;
                    human->point[1] = position;
                    human->locate->vz = position;
                    i = CVAnow->y * 1000;
                    level = GetAreaMapLevel(GlobalAreaMap, human->locate->vx,
                                            i - 1000, human->locate->vz, 0);
                    human->locate->vy = level;
                    if (i < human->locate->vy || human->locate->vy == (long)0x80000000)
                        human->locate->vy = i;
                    human->rotate->vy = CVAnow->p;
                    UpdateCoordinate((ModelType *)human->model);
                    delta = __builtin_abs(human->locate->vy - StagePlayer->locate->vy);
                    if (delta > 20000)
                        human->attribute |= 0x80;
                }
                break;

            case 3:
                human = GetHumanoid(CVAnow->id);
                if (human == 0)
                    return 0;

                packed = (u32)(u16)CVAnow->x << 16;
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
                    if (human->status == 0x11 && (u32)(i - 0x10) > 1)
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

                    slot = anim_base;
                scan_case3_human:
                    if (slot->human == human)
                        goto scan_case3_human_done;
                    i++;
                    if (i < 5)
                    {
                        slot++;
                        goto scan_case3_human;
                    }
                scan_case3_human_done:
                    if (i == 5)
                    {
                        i = 0;
                        slot = anim_base;
                    scan_case3_empty:
                        if (slot->human == 0)
                            goto scan_case3_empty_done;
                        i++;
                        if (i < 5)
                        {
                            slot++;
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
                if (CVAnow->id == 0)
                {
                    ViewInfo.vrx = CVAnow->x * 100;
                    ViewInfo.vry = CVAnow->y * 100;
                    ViewInfo.vrz = CVAnow->z * 100;
                }
                else
                {
                    human = GetHumanoid(CVAnow->p);
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
                CameraPanMode = (u16)CVAnow->id;
                pan_value = 20;
                if (CVAnow->p != 0)
                    pan_value = CVAnow->p;
                CameraSpeed = pan_value;
                break;

            case 7:
                event = CVAnow;
                vect.vx = event->x * 10;
                vect.vy = event->y * 10;
                vect.vz = event->z * 10;
                switch (event->id)
                {
                case 1:
                    SetBlood(&vect, event->p, 30);
                    break;
                case 3:
                    FUN_80038fdc((u8)event->x, (u8)event->y,
                                 (u8)event->z, event->p);
                    break;
                }
                break;

            case 8:
                if (CVAnow->id != invalid)
                {
                    SetupTelop((u8 *)strcpy((char *)D_800C2C50,
                                            (char *)CVAdata + CVAnow->id), 0);
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
                            *(u16 *)&TENCHU_POSITIONAL_DATA_AREA_[i]->attribute |= 1;
                            i++;
                        } while (i < 6);
                    }
                    else
                    {
                        *(u16 *)&TENCHU_POSITIONAL_DATA_AREA_[ch - '1']->attribute &= 0xFFFE;
                    }
                }
                D_800C2C50[0] = 0;
                break;
            }

            CVAnow++;
            cursor = CVAnow;
        } while (cursor->mode != 1);
    }

    return CVAnow->id != 0;
}
