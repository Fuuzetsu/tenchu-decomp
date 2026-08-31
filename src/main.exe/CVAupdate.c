#include "common.h"
#include "sound.h"
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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 *     extern short CameraSpeed;
 *     extern struct CVAType *CVAdata;
 *     extern int StageID;
 * END PSX.SYM */

/*
 * CVAupdate (0x80050628) — interpret character-animation and camera events.
 *
 * Byte-matching. Jump-table function (switch on the event kind).
 *
 * Four register-allocation ties were closed; each
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
 *   - A staged manual absolute value ties each load into its dying base's
 *     quantity; the direct `__builtin_abs` test expands via abssi2 and unties
 *     them.
 *   - `anim`: ONE cursor across case 2 and case 3 is one pseudo, hence one hard
 *     register everywhere ($a0). Case 2's preheader overlaps `human->motion` in
 *     $v1 and so must take $a0, and that choice was being carried into case 3,
 *     where the target uses $v1. Case 3 needs its OWN cursor (`slot`).
 *
 * PSX.SYM's three-locals record (vect, human, i) was the through-line: `y` and
 * `active_status` were draft-invented locals the original did not have.
 * `delta` disappears into the direct absolute-value test, while the shared
 * `anim` cursor still needs splitting by case.
 */

extern s16 CVAflag; /* set by CVA camera/telop commands */
extern u8 TelopText[];
extern u8 ctype_tab[]; /* BSD _ctype_+1: &4 = digit */
extern u8 CHOSEN_CHARACTER;
extern Sprite3D *TANKA_SPRITES_[6];

extern s16 PlayMotion(MotionManager *motion, s16 mode);
extern int ReqLifeBar(Humanoid *h);
extern void CdaStop(void);
extern void AVCameraSetup(void);
extern void SetBlood(VECTOR *pos, s16 n, s16 time);
extern void set_fade_(u8 arg0, u8 arg1, u8 arg2, long arg3);
extern char *strcpy(char *dst, const char *src);
extern void SetupTelop(u8 *telop, s16 line);

s16 CVAupdate(void)
{
    Humanoid *human;
    HumanAnimType *anim;
    HumanAnimType *anim_base;
    HumanAnimType *slot;
    ModelArchiveType *model;
    CVAType *cursor;
    VECTOR vect;
    s32 i;
    s32 invalid;
    s32 pan_value;
    u32 packed;
    u8 ch;

    cursor = CVAnow;
    if (cursor->mode != CVA_CMD_WAIT)
    {
        /* Register-held -1 (SetWire's one_value class): byte-required
         * (inlining the literal reorders the entry constants; measured). */
        invalid = -1;
        /* The array-base alias is byte-required (indexing CVAhuman directly
         * recolors the base register; measured). */
        anim_base = CVAhuman;
        do
        {
            switch (cursor->mode)
            {
            case CVA_CMD_SEQUENCE:
                /* A chained header mid-stream: p re-selects the CD
                 * track, and -1 silences it. */
                if (CVAnow->p == invalid)
                    CdaStop();
                break;

            case CVA_CMD_MOTION:
                human = GetHumanoid(CVAnow->id);
                if (human == 0)
                    return 0;
                i = 0;

                human->attribute &= ~ATTR_SUSPEND;
                human->motion->mask = 0x7FFF;
                anim = anim_base;
                while (1)
                {
                    if (anim->human == 0)
                        break;
                    i++;
                    if (i >= 5)
                        break;
                    anim++;
                }
                if (i == 5)
                    SetNowMotion(human, MOT_ENGAGE_STANCE, 1);

                human->vector = UnitVector;
                human->model->object[MODEL_PART_WAIST]->attribute |= MODEL_ATTR_COLLIDE;
                model = human->model;
                i = 0;
                if (model->n > 0)
                {
                    do
                    {
                        model->object[i]->attribute &= ~MODEL_ATTR_HIDDEN;
                        i++;
                    } while (i < model->n);
                }

                if (StagePlayer != human && human->life == invalid)
                {
                    human->attribute |= ATTR_CUSTOMAI;
                    human->life = human->lifemax;
                }

                if (CVAnow->p != invalid)
                {
                    human->locate->vx = human->point[HUMANOID_HOME_X] =
                        CVAnow->x * 1000;
                    human->locate->vz = human->point[HUMANOID_HOME_Z] =
                        CVAnow->z * 1000;
                    i = CVAnow->y * 1000;
                    human->locate->vy = GetAreaMapLevel(
                        GlobalAreaMap, human->locate->vx, i - 1000,
                        human->locate->vz, 0);
                    if (i < human->locate->vy || human->locate->vy == (long)0x80000000)
                        human->locate->vy = i;
                    human->rotate->vy = CVAnow->p;
                    UpdateCoordinate((ModelType *)human->model);
                    if (__builtin_abs(human->locate->vy -
                                      StagePlayer->locate->vy) > 20000)
                        human->attribute |= ATTR_SUSPEND;
                }
                break;

            case CVA_CMD_ACTOR:
                human = GetHumanoid(CVAnow->id);
                if (human == 0)
                    return 0;

                /* For ACTOR commands the x slot packs two bytes: the low
                 * byte rides >>16 into the life/invalid test, the high byte
                 * (>>24) is the think index below. One sll serves both
                 * extractions -- separate (s8)/(s16) spellings do not match. */
                packed = (u32)(u16)CVAnow->x << 16;
                if ((s32)packed >> 16 == invalid)
                {
                    human->life = invalid;
                    human->attribute = (human->attribute | ATTR_SUSPEND | PHASE_ALERT) & ~ATTR_CUSTOMAI;
                    human->motion->mid = invalid;
                    SetNowMotion(human, 0, 1);
                    PlayMotion(human->motion, 1);
                    human->motion->count--;
                }
                else
                {
                    i = (s32)packed >> 24;
                    if (human->status == STAT_DEAD && (u32)(i - (MOT_DAMAGE >> 8)) > 1)
                        return 0;
                    if (human->life > 0)
                    {
                        if (i == (MOT_DEAD >> 8))
                        {
                            human->life = 0;
                            ReqLifeBar(human);
                        }
                    }
                    i = 0;

                    slot = anim_base;
                    while (1)
                    {
                        if (slot->human == human)
                            break;
                        i++;
                        if (i >= 5)
                            break;
                        slot++;
                    }
                    if (i == 5)
                    {
                        i = 0;
                        slot = anim_base;
                        while (1)
                        {
                            if (slot->human == 0)
                                break;
                            i++;
                            if (i >= 5)
                                break;
                            slot++;
                        }
                        if (i == 5)
                            return 0;
                    }

                    human->motion->mid = invalid;
                    SetNowMotion(human, CVAnow->x, 1);
                    PlayMotion(human->motion, 1);
                    human->motion->count--;
                    anim_base[i].human = human;

                    anim_base[i].loop = CVAnow->y < 1 ? 0x7fff : CVAnow->y;
                    anim_base[i].motid = CVAnow->z == 0
                                             ? MOT_ENGAGE_STANCE
                                             : CVAnow->z;

                    if (human->type == S2 && CVAnow->x == MOT_DEAD)
                        SoundEx(0, SE_CUTSCENE_DEATH);
                }
                break;

            case CVA_CMD_CAMERA_CUT:
                AVCameraSetup();
                CVAflag = 1;
                break;

            case CVA_CMD_CAMERA_POSE:
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

            case CVA_CMD_CAMERA_PAN:
                CameraPanMode = CVAnow->id;
                pan_value = 20;
                if (CVAnow->p != 0)
                    pan_value = CVAnow->p;
                CameraSpeed = pan_value;
                break;

            case CVA_CMD_EFFECT:
                vect.vx = CVAnow->x * 10;
                vect.vy = CVAnow->y * 10;
                vect.vz = CVAnow->z * 10;
                switch (CVAnow->id)
                {
                case 1:
                    SetBlood(&vect, CVAnow->p, 30);
                    break;
                case 3:
                    set_fade_((u8)CVAnow->x, (u8)CVAnow->y,
                              (u8)CVAnow->z, CVAnow->p);
                    break;
                }
                break;

            case CVA_CMD_TELOP:
                if (CVAnow->id != invalid)
                {
                    SetupTelop((u8 *)strcpy((char *)TelopText,
                                            (char *)CVAdata + CVAnow->id),
                               0);
                    CVAflag = 1;
                    if (StageID != STAGE_FREE_PRINCESS || CHOSEN_CHARACTER != 0)
                        break;

                    ch = TelopText[0];
                    if ((ctype_tab[ch] & 4) == 0)
                        break;

                    i = ch - '0';
                    if (i == 0)
                    {
                        do
                        {
                            TANKA_SPRITES_[i]->attribute |= MODEL_ATTR_HIDDEN;
                            i++;
                        } while (i < 6);
                    }
                    else
                    {
                        *(u16 *)&TANKA_SPRITES_[i - 1]->attribute &= ~MODEL_ATTR_HIDDEN;
                    }
                }
                TelopText[0] = 0;
                break;
            }

            CVAnow++;
            cursor = CVAnow;
        } while (cursor->mode != CVA_CMD_WAIT);
    }

    return CVAnow->id != 0;
}
