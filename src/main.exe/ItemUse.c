#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short ItemUse(void);
 *     THINK_3.C:225, 18 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       short id
 *
 * Globals it touches, as the original declared them:
 *     extern short Degree;
 * END PSX.SYM */

static s16 ItemUse(void)
{
    Humanoid *me;
    s16 id;

    if (Me_THINK_C->motion->count != 0)
    {
        return 5;
    }
    if (Me_THINK_C->status != STAT_ENGAGE)
    {
        return 5;
    }

    if (Me_THINK_C->item[ITEM_KUSURI] != 0 &&
        Me_THINK_C->life < Me_THINK_C->lifemax / 3)
    {
        ReqItemDefault(Me_THINK_C, ITEM_KUSURI);
        return;
    }

    me = Me_THINK_C;
    if (me->item[ITEM_SHURIKEN] != 0)
    {
        id = MOT_SYURI;
        if (__builtin_abs(Degree) < 100)
        {
            goto do_motion;
        }
    }

    if (me->item[ITEM_FIRE] == 0)
    {
        goto end;
    }

    id = MOT_ITEM_THROW;
    if (__builtin_abs(Degree) < 300)
    {
        goto do_motion;
    }
    goto end;

do_motion:
    SetNowMotion(me, id, MOTION_MOVE_APPLY);
    return;

end:
    return;
}
