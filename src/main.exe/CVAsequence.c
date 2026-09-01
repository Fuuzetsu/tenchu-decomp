#include "common.h"
#include "tuning.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short CVAsequence(short sid);
 *     CHRANIM.C:83, 58 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short sid
 *     reg   $s3       struct SoundEffect * vab
 *     reg   $a0       struct Humanoid * human
 *     reg   $s0       short sound
 *     reg   $s0       short i
 *     reg   $a1       short j
 *
 * Globals it touches, as the original declared them:
 *     extern struct CVAType *CVAdata;
 *     extern struct CVAType *CVAnow;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct Humanoid *StagePlayer;
 *     extern struct Humanoid *CameraTarget;
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 *     extern short ActionHalt;
 *     extern short MotionUpdateMode;
 *     extern struct TCdaStatus CdaStatus;
 *     extern short CVAtime;
 *     extern short VoiceMode;
 * END PSX.SYM */

/*
 * The post-memset cursor is deliberately separate from the scan's `event`.
 * Updating that short-lived carrier in place preserves the source dependency
 * that makes cc1 keep the cursor in v0 while loading StagePlayer into v1.
 *
 * Other load-bearing shapes: the event scan is a hand-rolled goto loop;
 * separate s32 `wanted`/`end_mode` values create the preheader extension and
 * loop-carried -1; and the motion arm's `(motion = 0, test)` comma expression
 * selects the retail allocation and schedule.
 */

#include "item.h"

extern u8 TelopText[];
extern s16 CVAflag; /* set by CVA camera/telop commands */

extern void *memset(void *s, int c, u32 n);
extern s16 CVAupdate(void);
extern void PadShock(s32 port, s32 power, s32 time);
extern void PadProc(void);
extern void PlayMusicFormID(s32 id);
extern s32 CdaGetCurrentLength(void);
extern s16 CVArun(void);
extern void CdaStop(void);

s16 CVAsequence(s16 sid)
{
    CVAType *event;
    CVAType *cursor;
    Humanoid **slot;
    Humanoid *human;
    s16 sound;
    s16 i;
    s16 motion;
    s32 wanted;
    s32 end_mode;
    s32 type_class;
    HumanAnimType *anim_base;

    CVAnow = CVAdata;
    if (CVAdata->mode == -1)
        goto return_zero;

    wanted = sid;
    end_mode = -1;
scan_event:
    event = CVAnow;
    if (event->mode == CVA_CMD_SEQUENCE && event->id == wanted)
        goto event_found;
    CVAnow = event + 1;
    if (event[1].mode != end_mode)
        goto scan_event;

event_found:

    if (CVAnow->mode == -1)
        goto return_zero;

    memset(CVAhuman, 0, sizeof(CVAhuman));
    cursor = CVAnow;
    sound = cursor->p;
    i = 0;
    CameraTarget = StagePlayer;
    cursor++;
    CVAnow = cursor;
    TelopText[0] = 0;
    if (Humans > 0)
    {
        do
        {
            slot = &HumanGroup[i];
            human = *slot;
            if (human->status != STAT_DEAD &&
                (human->attribute & ATTR_SUSPEND) == 0)
            {
                dispose_weapon_data_of_char_(human, ATTACK_CANCEL_ALL);
                NowReturnNormal(*slot);
                (*slot)->pad.data = 0;
            }
            i++;
        } while (i < Humans);
    }

    CVAflag = 0;
    if (CVAupdate() != 0)
        goto run_sequence;

return_zero:
    return 0;

run_sequence:
    if (ActionHalt != -1)
        ActionHalt = 1;
    MotionUpdateMode = 1;
    StagePlayer->target = 0;
    PadShockAR(0, RUMBLE_POWER_OFF, RUMBLE_ATTACK_NONE, RUMBLE_RELEASE_NONE);
    PadShock(0, 0, 0);
    PadProc();

    if (sound > 0)
    {
        PlayMusicFormID(sound);
        while (CdaStatus.status != 0)
        {
            if (CdaGetCurrentLength() > 0)
                break;
        }
    }

    CVAtime = 0;
    VoiceMode = 1;
    do
    {
    } while (CVArun() != 0);
    VoiceMode = 0;
    if (ActionHalt != -1)
        ActionHalt = 0;
    MotionUpdateMode = 0;
    SetCameraMode(CMODE_NORMAL);

    i = 0;
    anim_base = CVAhuman;
    type_class = PAGE_BOSS;
    for (; i < N_CVA_HUMANS; i++)
    {
        human = anim_base[i].human;
        if (human != 0 && human->status != STAT_DEAD)
        {
            motion = MOT_ENGAGE_STANCE;
            if ((human->attribute & ATTR_ALERT) == 0 &&
                (motion = 0, (human->type & PAGE_MASK) == type_class))
                motion = MOT_STATE_DRAW;
            SetNowMotion(human, motion, 1);
        }
    }

    if (sound > 0)
        VSync(60);
    CdaStop();
    PadShockAR(0, RUMBLE_POWER_OFF, RUMBLE_ATTACK_NONE, RUMBLE_RELEASE_NONE);
    PadShock(0, 0, 0);
    PadProc();
    PadProc();
    return 1;
}
