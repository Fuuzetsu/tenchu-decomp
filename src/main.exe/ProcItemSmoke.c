#include "common.h"
#include "main.exe.h"
#include "sound.h"

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
                SVECTOR vec = svec_y_n250[0];
                VECTOR pos = {
                    item->locate->locate.coord.t[0],
                    item->locate->locate.coord.t[1],
                    item->locate->locate.coord.t[2]
                };

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
                /* Empty loop retained for code layout; its original source construct is unknown. */
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
