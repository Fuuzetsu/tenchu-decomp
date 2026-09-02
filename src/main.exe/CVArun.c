#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short CVArun(void);
 *     CHRANIM.C:238, 47 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a2       struct MotionManager * mmp
 *     reg   $s1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern int StageID;
 *     extern struct GsOT *OTablePt;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern short CVAtime;
 *     extern struct CVAType *CVAnow;
 * END PSX.SYM */

extern Sprite3D *TANKA_SPRITES_[N_TANKA_SPRITES];
extern u8 CHOSEN_CHARACTER;

extern void AVCameraControl(void);
extern void DrawConstruction(void);
extern void DrawEffect(void);
extern void DoItemProc(void);
extern void DoMiscProc(void);
extern void DrawTelop(void);
extern void draw_visible_characters_(void);
extern short CVAupdate(void);

short CVArun(void)
{
    s16 i;
    Sprite3D *e;
    MotionManager *mmp;
    Humanoid *human;
    Humanoid *reload;
    motion_id motid;

    ComputeAllConflict();
    StartDrawing();
    AVCameraControl();
    ControlAllHumanoid();
    DrawConstruction();
    DrawEffect();
    DoItemProc();
    DoMiscProc();
    DrawTelop();
    draw_visible_characters_();

    if (StageID == STAGE_ID_CORRUPT_MINISTER && CHOSEN_CHARACTER == RIKIMARU_0)
    {
        for (i = 0; i < N_TANKA_SPRITES; i++)
        {
            e = TANKA_SPRITES_[i];
            if ((e->attribute & MODEL_ATTR_HIDDEN) == 0)
            {
                if ((s8)e->sprite.r >= 0)
                {
                    e->sprite.r = e->sprite.g = e->sprite.b =
                        e->sprite.b + 8;
                }
                GsSortSprite(&TANKA_SPRITES_[i]->sprite, OTablePt, 0);
            }
        }
    }

    EndDrawing(-2);

    for (i = 0; i < N_CVA_HUMANS; i++)
    {
        human = CVAhuman[i].human;
        if (human != 0 && CVAhuman[i].loop <= human->motion->loop)
        {
            mmp = human->motion;
            motid = CVAhuman[i].motid;
            if (motid == MOTION_ID_NONE)
            {
                mmp->loop = MOTION_LOOP_DISABLED;
                reload = CVAhuman[i].human;
                reload->vector.vz = 0;
                reload->vector.vx = 0;
            }
            else if (human->status != STAT_DEAD)
            {
                SetNowMotion(human, motid, MOTION_MOVE_APPLY);
                CVAhuman[i].human = 0;
            }
        }
    }

    CVAtime++;
    if (CVAtime >= CVAnow->payload.wait.frames)
    {
        CVAnow++;
        return CVAupdate();
    }
    return 1;
}
