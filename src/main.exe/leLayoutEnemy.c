#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "appear.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void leLayoutEnemy(int mode);
 *     WORLD.C:1197, 58 src lines, frame 104 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s5       int mode
 *     reg   $s3       int i
 *     reg   $s1       struct Humanoid * target
 *     stack sp+24     struct VECTOR pos
 *     reg   $s0       struct TraceLine * t
 *     reg   $s0       struct TEnemyLayout * en
 *     reg   $s1       struct Humanoid * human
 *     reg   $s1       struct Humanoid * human
 *     reg   $s0       struct TEnemyLayout * en
 *     reg   $a1       struct TracePoint * tp
 *     reg   $a0       int i
 *     stack sp+40     struct VECTOR pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *HumanGroup[32];
 *     extern short Humans;
 *     extern enum TSystemFlag SystemFlag;
 *     extern struct TEnemyLayout enemy[30];
 *     extern struct TCameraStatus CamState;
 *     extern long EmergencyNotice;
 *     extern long GameClock;
 * END PSX.SYM */

#include "item.h"

extern void reset_effects_(void);
extern void SetupThinkFunction(Humanoid *human, TThinkType type);
extern void *valloc(u32 size);
extern void vfree(void *ptr);

/*
 * Matching notes (see docs/matching-cookbook.md):
 *  - Both table scans need the explicit top-tested `while (1)`/`break`
 *    shape.  Keeping `HumanGroup` in `group` also reproduces the removal
 *    loop's base-pointer lifetime.
 *  - The two address-taken VECTORs receive stack slots in declaration
 *    order: `tmp` must precede the memset scratch `pos`, even though `pos`
 *    is referenced first.
 *  - The one-shot loop around the rotation store emits no control flow; its
 *    loop note is the scheduler barrier needed for the target's two load
 *    delay nops.  This was isolated with the RTL-guided autorules pass.
 *  - Direct `tp[i]` indexing gives the target's single induction value;
 *    a walking TracePoint pointer introduces a second one.
 */
void leLayoutEnemy(int mode)
{
    s32 i;
    Humanoid *target;
    VECTOR tmp;
    VECTOR pos;
    TraceLine *t;
    Humanoid **group;

    reset_effects_();
    group = HumanGroup;
    while (1)
    {
        if (Humans <= 1)
        {
            break;
        }
        target = group[1];
        if ((SystemFlag & SYSFLAG_DEBUGMODE) != 0)
        {
            memset(&pos, 0, sizeof(pos));
            pos.vx = target->model->locate.coord.t[0];
            pos.vy = target->model->locate.coord.t[1] - 1200;
            pos.vz = target->model->locate.coord.t[2];
            tmp = pos;
            SetBleeds(&tmp, 600, 20, 10, 10, COLOR_YELLOW);
        }
        t = target->trace;
        if (t != 0)
        {
            vfree(t->point);
            vfree(t);
        }
        KillHumanoid(target);
    }

    i = 0;
    while (1)
    {
        TEnemyLayout *en;

        if (i >= MAX_ENEMIES)
        {
            break;
        }
        en = &enemy[i];
        if (en->type != CHARACTER_KIND_END)
        {
            Humanoid *human;
            ModelArchiveType *owner_model;

            human = BreedLife(en->type, en->x, en->y, en->z, 0);
            human->model->rotate.vy = en->r;
            /* Staged owner_model straddling the |= is byte-required
             * (inlining the read reorders the pair; measured). */
            owner_model = CamState.Owner->model;
            human->attribute |= ATTR_SUSPEND;
            human->target.archive = owner_model;
            human->model->object[MODEL_PART_WAIST]->attribute &= ~MODEL_ATTR_COLLIDE;
            if (mode == 1)
            {
                SetupThinkFunction(human, en->ThinkType);
                if (en->nPath != 0)
                {
                    TracePoint *tp;
                    s32 i;

                    tp = valloc((en->nPath + 1) * sizeof(TracePoint));
                    for (i = 0; i < en->nPath; i++)
                    {
                        tp[i].x = en->path[i].vx;
                        tp[i].z = en->path[i].vz;
                        tp[i].range = 1500;
                        tp[i].pad = 0;
                    }
                    tp[i].pad = TRACE_POINT_END;
                    SetupTraceLine(human, tp);
                }
                if (human->trace != 0)
                {
                    human->attribute |= ATTR_TRACE;
                }
            }
            if ((SystemFlag & SYSFLAG_DEBUGMODE) != 0)
            {
                memset(&pos, 0, sizeof(pos));
                pos.vx = human->model->locate.coord.t[0];
                pos.vy = human->model->locate.coord.t[1] - 1200;
                pos.vz = human->model->locate.coord.t[2];
                tmp = pos;
                SetBleeds(&tmp, 400, 0, 20, 15, COLOR_WHITE);
            }
        }
        i++;
    }
    EmergencyNotice = 0;
    GameClock = 0;
}
