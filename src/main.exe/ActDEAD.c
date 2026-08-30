#include "common.h"
#include "main.exe.h"
#include "effect.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActDEAD(void);
 *     MOTION.C:2046, 89 src lines, frame 64 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     reg   $s0       struct VECTOR * pp
 *     stack sp+16     struct VECTOR p
 *     stack sp+32     struct SVECTOR v
 *     reg   $s2       short bldo
 *     reg   $s3       short blds
 *     reg   $s0       short blood
 * END PSX.SYM */

/*
 * ActDEAD (0x800268bc) — advances and finalizes death motions, handles
 * corpse settling and feedback, dispatches scripted death-frame events, and
 * emits the associated splash/blood effects.
 *
 * Matching notes (1,680 bytes / 420 instructions):
 *  - The explicit splash/event/ordinary labels preserve the target's
 *    dispatch-chain-first layout and its otherwise-elided jump over splash.
 *  - The 0x28 scratch overlay fixes the VECTOR at sp+0x10 and the two
 *    SVECTORs at sp+0x28/sp+0x30 while keeping the source aggregates typed.
 *  - svec_y_n200_z_n240 is an unknown-sized SVECTOR array so its [0] copy retains
 *    the target's split high/low address materialization.
 *  - The event scan keeps count and the sentinel live across its backedge;
 *    its explicit labels prevent cc1 from peeling the known-zero first row.
 *    Advancing it as the natural `i++` also preserves the target's v0/v1
 *    next-row allocation and moves the counter update into the backedge's
 *    delay slot.
 */

typedef struct
{
    s16 frame;
    s16 action;
    s16 argument;
    s16 packed;
} DeadEvent;

/* DeadEvent.action codes (invented names): the killer's grunt, the
 * victim's cry, pad rumble, a blood burst, and the burst that also
 * terminates the event list (the scan stops on it). */
enum
{
    DEADEV_SOUND_PLAYER = 0,
    DEADEV_SOUND_SELF = 1,
    DEADEV_RUMBLE = 2,
    DEADEV_BLOOD = 3,
    DEADEV_END = 4
};

typedef union
{
    struct
    {
        VECTOR p;
        u8 pad[8];
        SVECTOR position;
        SVECTOR vector;
    } dead;
} ActDeadScratch;

extern Humanoid *Me_MOTION_C;
extern Humanoid *DeadHumanoid;
extern DeadEvent *DeadEvents[];
extern SVECTOR svec_y_n200_z_n240[];

extern s32 rand(void);
extern void *memset(void *dst, s32 value, u32 size);
extern s16 PlayMotion(MotionManager *motion, s16 mode);
extern void TurnAroundAllItems(Humanoid *human);
extern int ReqLifeBar(Humanoid *h);
void ActDEAD(void)
{
    ModelArchiveType *model;
    short blood;
    short bldo;
    short blds;
    short i;
    short mid;
    DeadEvent *pp;
    ActDeadScratch scratch;

    model = Me_MOTION_C->model;
    blood = -1;
    if ((*model->object)->id < 0 && dtM->loop < 0)
        return;

    if (dtM->count == 0)
    {
        if (dtM->loop != 0)
        {
            dtM->loop = -1;
            goto check_motion_end;
        }
    }
    else
    {
    check_motion_end:
        if (dtM->loop < 0 && dtV->vy == 0)
        {
            MotionManager *motion;
            Humanoid *human;
            SVECTOR *velocity;

            motion = dtM;
            motion->count = motion->motion->time;
            motion->loop = 0;
            PlayMotion(motion, 1);
            dtM->loop = -1;
            if (motID != MOT_DEAD_DROWN)
            {
                *(u16 *)&Me_MOTION_C->attribute &= ~ATTR_SEARCH;
            }
            else
            {
                *(u16 *)&Me_MOTION_C->attribute |= ATTR_SEARCH; /* drowned */
                *(u16 *)&Me_MOTION_C->model->attribute |= MODEL_ATTR_HIDDEN;
            }

            velocity = dtV;
            human = Me_MOTION_C;
            velocity->vz = 0;
            velocity->vx = 0;
            if (human != StagePlayer)
            {
                DeleteConflict(*model->object);
                if ((*(u16 *)&Me_MOTION_C->type & PAGE_MASK) != PAGE_BOSS)
                    TurnAroundAllItems(Me_MOTION_C);
            }
            if (dtM->mid < MOT_DEAD_STEALTH_BACK)
                return;
            ActionHalt = 0;
            CamState.snap_pending = 1;
            return;
        }
    }

    if (dtM->mid > MOT_DEAD_DROWN && dtL->vy != StagePlayer->locate->vy)
    {
        dtL->vy--;
        motID = MOT_DEAD;
        ActionHalt = 0;
        motMODE = 1;
        if (dtM->count >= 10)
            return;
        PadShockAR(0, 0xff, 10, 10);
        Sound(Me_MOTION_C, 8);
        Sound(StagePlayer, 5);
        return;
    }

    /* The death-kind dispatch is a hand-written goto ladder, and the
     * evidence is now two-sided: retail's branches jump TO the labeled
     * bodies (test-first layout only `if (c) goto L;` produces — a
     * structured else-if falls INTO its arms instead), and the DEMO's
     * ActDEAD has a simpler single `mid < 0x1109` test here — retail
     * added the DeadEvents 0x1109..0x110e range and extended the demo's
     * test into this ladder by hand. */
    mid = dtM->mid;
    if (mid == MOT_DEAD_DROWN)
        goto splash_dead;
    if (mid < MOT_DEAD_DROWN)
        goto ordinary_dead;
    if (mid > MOT_DEAD_STEALTH_SIDE_AYAME)
        goto ordinary_dead;
    goto event_dead;

splash_dead:
{
    if (rand() % 20 == 0)
        Sound(Me_MOTION_C, 0x16);
    scratch.dead.p.vy = Me_MOTION_C->map.level;
    if ((rand() & 5) == 0)
    {
        i = 0;
        do
        {
            long width;
            int r;

            r = rand();
            width = Me_MOTION_C->width;
            scratch.dead.p.vx = dtL->vx + (r % width) * 2 - width;
            r = rand();
            width = Me_MOTION_C->width;
            scratch.dead.p.vz = dtL->vz + (r % width) * 2 - width;
            SetSplash(&scratch.dead.p, (rand() & 7) << 12,
                      (rand() & 7) << 12, 6);
            i++;
        } while (i < 5);
    }
    goto blood_effect;
}

event_dead:
{
    MotionManager *motion;
    int count;
    int stop;

    motion = dtM;
    pp = DeadEvents[motion->mid - MOT_DEAD_STEALTH_BACK];
    i = 0;
    if (pp[i].action == DEADEV_END)
        goto event_ready;
    count = motion->count;
    stop = DEADEV_END;
scan_event:
    if (pp[i].frame == count)
        goto event_ready;
    i++;
    if (pp[i].action != stop)
        goto scan_event;
event_ready:
    if (dtM->count < pp[i].frame)
        return;

    switch (pp[i].action)
    {
    case DEADEV_SOUND_PLAYER:
        Sound(StagePlayer, pp[i].argument);
        break;
    case DEADEV_SOUND_SELF:
        Sound(Me_MOTION_C, pp[i].argument);
        break;
    case DEADEV_RUMBLE:
        PadShockAR(0, 0xff, pp[i].argument, pp[i].packed);
        break;
    case DEADEV_BLOOD:
    case DEADEV_END:
    {
        u16 packed;

        ReqLifeBar(Me_MOTION_C);
        blood = *(u16 *)&pp[i].argument;
        packed = *(u16 *)&pp[i].packed;
        bldo = packed >> 8;
        blds = packed & 0xff;
        break;
    }
    }
    goto blood_effect;
}

ordinary_dead:
    if ((*(u16 *)&Me_MOTION_C->type & PAGE_MASK) != PAGE_BEAST)
    {
        if (dtM->count == 5 && DeadHumanoid == Me_MOTION_C)
        {
            Sound(DeadHumanoid, 0x38);
            DeadHumanoid = 0;
        }
        blood = 1;
        bldo = 100;
        blds = 0;
    }

blood_effect:
    if ((*(u16 *)&dtM->count & 4) && blood != -1)
    {
        scratch.dead.position = svec_y_n200_z_n240[0];
        memset(&scratch.dead.vector, 0, sizeof(scratch.dead.vector));
        scratch.dead.vector.vy = -blds;
        scratch.dead.vector.vz = -bldo;
        if (blds == 0)
        {
            scratch.dead.position.vx = 0;
            scratch.dead.position.vy = -200;
            scratch.dead.position.vz = -240;
        }
        else
        {
            scratch.dead.position.vx = 0;
            scratch.dead.position.vy = -410;
            scratch.dead.position.vz = 0;
        }
        SetGore(&Me_MOTION_C->model->object[blood]->locate,
                &scratch.dead.position, &scratch.dead.vector);
    }
}
