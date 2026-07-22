#include "common.h"
#include "main.exe.h"
#include "appear.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void leLayoutEnemy(int mode);
 *     WORLD.C:1197, 58 src lines, frame 104 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
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
 *     extern long GameClock;
 * END PSX.SYM */

#include "item.h"

extern s16 Humans;
extern s32 SystemFlag;
extern TEnemyLayout enemy[30];
extern Humanoid *CURRENTLY_SELECTED_CHARACTER_STATE_PTR;
extern s32 GameClock;

extern void FUN_80039c14(void);
extern void KillHumanoid(Humanoid *human);
extern void SetBleeds(VECTOR *pos, s16 grange, s16 srange, s16 n,
                      s32 time, s32 col);
extern void SetupThinkFunction(Humanoid *human, s16 type);
extern TraceLine *SetupTraceLine(Humanoid *human, TracePoint *point);
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
 *  - Direct `point[i]` indexing gives the target's single induction value;
 *    a walking TracePoint pointer introduces a second one.
 */
void leLayoutEnemy(s32 mode)
{
    s32 i;
    Humanoid *target;
    VECTOR tmp;
    VECTOR pos;
    TraceLine *t;
    Humanoid **group;

    FUN_80039c14();
    group = HumanGroup;
    while (1)
    {
        if (!(1 < Humans))
        {
            break;
        }
        target = group[1];
        if ((SystemFlag & 2) != 0)
        {
            memset(&pos, 0, sizeof(pos));
            pos.vx = target->model->locate.coord.t[0];
            pos.vy = target->model->locate.coord.t[1] - 0x4B0;
            pos.vz = target->model->locate.coord.t[2];
            tmp = pos;
            SetBleeds(&tmp, 600, 20, 10, 10, 0xFFFF00);
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

        if (!(i < 30))
        {
            break;
        }
        en = &enemy[i];
        if (en->type != -1)
        {
            Humanoid *human;
            ModelArchiveType *owner_model;

            human = BreedLife(en->type, en->x, en->y, en->z, 0);
            do
            {
                human->model->rotate.vy = en->r;
            } while (0);
            owner_model = CURRENTLY_SELECTED_CHARACTER_STATE_PTR->model;
            human->attribute |= 0x80;
            human->target = (ModelType *)owner_model;
            human->model->object[0]->attribute &= 0xBFFF;
            if (mode == 1)
            {
                SetupThinkFunction(human, en->ThinkType);
                if (en->nPath != 0)
                {
                    TracePoint *point;
                    s32 i;

                    point = valloc((en->nPath + 1) * sizeof(TracePoint));
                    for (i = 0; i < en->nPath; i++)
                    {
                        point[i].x = en->path[i].vx;
                        point[i].z = en->path[i].vz;
                        point[i].range = 1500;
                        point[i].pad = 0;
                    }
                    point[i].pad = -1;
                    SetupTraceLine(human, point);
                }
                if (human->trace != 0)
                {
                    human->attribute |= 8;
                }
            }
            if ((SystemFlag & 2) != 0)
            {
                memset(&pos, 0, sizeof(pos));
                pos.vx = human->model->locate.coord.t[0];
                pos.vy = human->model->locate.coord.t[1] - 0x4B0;
                pos.vz = human->model->locate.coord.t[2];
                tmp = pos;
                SetBleeds(&tmp, 400, 0, 20, 15, 0xFFFFFF);
            }
        }
        i++;
    }
    FRAMES_UNTIL_END_OF_ALERT = 0;
    GameClock = 0;
}
