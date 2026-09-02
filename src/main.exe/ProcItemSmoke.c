#include "common.h"
#include "main.exe.h"
#include "sound.h"

/*
 * ProcItemSmoke (0x8003ff68) — the smoke bomb item processor. Every frame:
 * roll it (MoveKorogari), mirror its coordinate into the sprite and draw;
 * mode 0: countdown, then pop (SoundEx 0x23) and re-arm 0x78 frames of smoke;
 * mode 1: emit a SetSmoke puff every other frame; every 16th GameClock tick
 * scan HumanGroup for live humans within 2000 units of the bomb and knock
 * them out (motion 0x100B, status 0x10, Sound 6); dispose at 0.
 *
 * Matching notes (all verified against the original bytes; the deepest RTL
 * dive of the item TU — several pass-level levers):
 *  - Direct `param->count--` followed by direct field tests retains the
 *    target's QI field update and SI test expressions without a staging
 *    local. Narrowing the operation through a `u8` local instead introduces
 *    a separate pseudo and an extra move.
 *  - The SetSmoke block's `build_pos` component stores are
 *    COMPONENT refs: an in-struct store invalidates cse's
 *    cached `item->locate` load (MEM_IN_STRUCT alias heuristic), reproducing
 *    the per-line reloads; a raw `((s32 *)&build_pos)[n]` spelling is a
 *    non-struct fixed-address store that cse ignores — locate stays cached
 *    (wrong). Inverse lever in the search setup: `pos = (VECTOR *)
 *    item->locate->locate.coord.t;` reads all three t[] through one pointer
 *    so nothing reloads there.
 *  - Search setup order is `q = &search_state; pos = ...;
 *    q->i = 0; find = &search_state;` —
 *    q's use must precede find's init or the two same-valued pointers
 *    collapse into one register (cse folds find's addiu into `move find,q`
 *    only, keeping both, when something touches q in between).
 *    The particle locals occupy an inner scope that ends before
 *    `search_state` begins, so GCC reuses their stack window naturally.
 *  - The inner scan uses a normal exhaustion guard, then assigns `found = 0`
 *    after the loop.  Moving that assignment to the loop top makes it
 *    loop-invariant; loop.c hoists it into the outer loop's head and shifts
 *    the whole allocation one callee-saved register up (s6 appears).
 *  - The hit handler sits at a `goto hit` label AFTER the tail, with
 *    `found = target;` first and the three find-> stores wrapped in
 *    do{}while(0): (a) creation order past the `check:` test keeps target as
 *    cse's canonical head (make_regs_eqv head-preference is by live-range
 *    end), so `find->find = target` keeps $s0; (b) the dummy loop note
 *    blocks local-alloc's optimize_reg_copy_1 from propagating the copy into
 *    the store (its scan stops at NOTE_INSN_LOOP_BEG); sched1 otherwise
 *    hoists the copy (REG_DEAD rank boost) and lreg rewrites the store.
 *  - `i = STAT_DAMAGE;` is pre-assigned BEFORE the ActionHalt guard (the cookbook
 *    pre-assign lever), reusing the dead loop counter: its li lands in the
 *    guard's delay slot and rides $s1 across the two calls to the sh.
 *  - `extern SVECTOR svec_y_n250[]` (unknown size!) + `svec_y_n250[0]`: an
 *    8-byte scalar extern is small-data-eligible (-G8), SYMBOL_REF_FLAG makes
 *    its address cost 1 and cse's find_best_addr folds the block-move source
 *    address back into the pattern (one-register `la`); the unknown-size
 *    array keeps cost 2 > reg, the high/lo_sum pair survives to the
 *    post-reload block-move split, and the hi half lands in $v0.
 */
/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemSmoke(struct tag_TItem *item);
 *     ITEM.C:1400, 59 src lines, frame 112 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct tag_TItem * item
 *     reg   $s1       struct Sprite3D * model
 *     reg   $s0       struct param_smoke * param
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s3       struct tag_TItem * item
 *     stack sp+16     struct SVECTOR vec
 *     stack sp+24     struct VECTOR pos
 *     stack sp+56     struct TFindItemTarget find
 *     reg   $v1       struct VECTOR * pos
 *     reg   $s1       int i
 *     reg   $s0       struct Humanoid * target
 *     reg   $v0       int dist
 *     reg   $s0       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct Humanoid *HumanGroup[32];
 *     extern short Humans;
 *     extern short ActionHalt;
 * END PSX.SYM */

#include "item.h"

extern SVECTOR svec_y_n250[];

extern void MoveKorogari(TItem *item, param_korogari *pp);

void ProcItemSmoke(TItem *item)
{
    enum
    {
        SMOKE_MODE_FUSE = 0,
        SMOKE_MODE_ACTIVE = 1
    };
    Sprite3D *model;
    param_smoke *param;

    model = (Sprite3D *)item->model;
    param = &item->param.smoke;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = SMOKE_MODE_FUSE;
        return;
    }
    MoveKorogari(item, &param->koro);
    if (param->koro.status == KORO_WATER)
    {
        if (item->proc == 0)
            return;
        item->mode = ITEM_MODE_DISPOSE;
        item->proc(item);
        DeleteConflict(item->locate);
        if (item->mode != SMOKE_MODE_FUSE)
        {
            AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
        }
        item->owner = 0;
        item->proc = 0;
        return;
    }
    UpdateCoordinate(item->locate);
    model->locate = item->locate->locate;
    DrawSprite(model);
    param->count--;
    switch (item->mode)
    {
    case SMOKE_MODE_FUSE:
        if (param->count != 0)
            return;
        SoundEx(MODEL_POSITION(item->locate), SE_SMOKE_PUFF);
        param->count = SMOKE_DURATION;
        item->mode++;
        return;

    case SMOKE_MODE_ACTIVE:
        if (param->count == 0)
        {
            if (item->proc == 0)
                return;
            DISPOSE_ITEM(item);
            return;
        }
        if ((param->count & 1) == 0)
        {
            {
                SVECTOR vec;
                VECTOR pos;
                VECTOR build_pos;

                vec = svec_y_n250[0];
                memset(&build_pos, 0, sizeof(VECTOR));
                build_pos.vx = item->locate->locate.coord.t[0];
                build_pos.vy = item->locate->locate.coord.t[1];
                build_pos.vz = item->locate->locate.coord.t[2];
                pos = build_pos;
                SetSmoke(&pos, &vec, 1, 3);
            }
        }
        if ((GameClock & 0xf) != 0)
            return;
        {
            TFindItemTarget search_state;
            TFindItemTarget *q;
            TFindItemTarget *find;
            VECTOR *pos;
            int i;
            Humanoid *target;
            Humanoid *found;
            Humanoid *human;
            int dist;

            q = &search_state;
            pos = MODEL_POSITION(item->locate);
            q->i = 0;
            find = &search_state;
            find->pos.vx = pos->vx;
            find->pos.vy = pos->vy;
            find->pos.vz = pos->vz;
            find->find_dist = 2000;
            while (1)
            {
                i = find->i;
                while (1)
                {
                    if (i >= Humans)
                    {
                        break;
                    }
                    target = HumanGroup[i];
                    if (target->life > 0 && target->motion->mid != MOT_ACTION && (target->attribute & ATTR_SUSPEND) == 0)
                    {
                        dist = GetVectorDistance(&find->pos, target->locate);
                        if (dist < find->find_dist)
                            goto hit;
                    }
                    i++;
                }
                found = 0;
            check:
                if (found == 0)
                    return;
                human = search_state.find;
                if (human != item->owner &&
                    human->life != HUMANOID_LIFE_INACTIVE &&
                    human->motion->mid != MOT_DAMAGE_CHOKE)
                {
                    i = STAT_DAMAGE;
                    if (ActionHalt == ACTION_HALT_NONE && human->life > 0)
                    {
                        dispose_weapon_data_of_char_(human,
                                                     ATTACK_CANCEL_ALL);
                        UpdateMotion(human->motion, MOT_DAMAGE_CHOKE);
                        human->status = i;
                        MoveHumanoid(human,
                                     human->motion->motion->orderspd,
                                     human->motion->motion->sidespd);
                    }
                    Sound(search_state.find, CHAR_VOICE_HURT);
                }
                continue;
            hit:
                found = target;
                /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
                do
                {
                } while (0);
                find->find = target;
                find->dist = dist;
                find->i = i + 1;
                goto check;
            }
        }
    }
}
