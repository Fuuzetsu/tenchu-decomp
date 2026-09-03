#include "common.h"
#include "tuning.h"
#include "sound.h"
#include "main.exe.h"

#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemKusuri(struct tag_TItem *item);
 *     ITEM.C:1485, 60 src lines, frame 80 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct tag_TItem * item
 *     reg   $s0       struct Sprite3D * model
 *     reg   $s1       int i
 *     reg   $s0       struct Humanoid * human
 *     reg   $s2       struct Humanoid * human
 *     reg   $s0       int itemID
 *     stack sp+16     struct PARAM_ITEM_LAUNCH p
 *     reg   $s3       struct tag_TItem * item
 *     stack sp+16     struct VECTOR pos
 *     stack sp+32     struct SVECTOR vec
 *     reg   $s3       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern short ActionHalt;
 * END PSX.SYM */

void ProcItemKusuri(TItem *item)
{
    enum
    {
        KUSURI_MODE_START = 0,
        KUSURI_MODE_DRINK = 1,
        KUSURI_MODE_HEAL = 2
    };
    Sprite3D *model;
    void (*ppu)(TItem *);
    s32 i;

    model = (Sprite3D *)item->model;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = KUSURI_MODE_START;
        return;
    }
    switch (item->mode)
    {
    case KUSURI_MODE_START:
    {
        Humanoid *human;

        human = item->owner;
        if (ActionHalt == ACTION_HALT_NONE && human->life > 0)
        {
            MotionDataType *md;

            dispose_weapon_data_of_char_(human, ATTACK_CANCEL_ALL);
            UpdateMotion(human->motion, MOT_ITEM_DRINK);
            human->status = STAT_ITEM;
            md = human->motion->motion;
            MoveHumanoid(human, md->orderspd, md->sidespd);
        }
    }
        {
            ModelArchiveType *arc;

            arc = item->owner->model;
            if (arc->n > MODEL_PART_WEAPON_HAND_1)
                item->locate->locate.super =
                    &arc->object[MODEL_PART_WEAPON_HAND_1]->locate;
            else
                item->locate->locate.super =
                    &arc->object[MODEL_PART_HEAD]->locate;
        }
        item->locate->locate.coord.t[0] = 0;
        item->locate->locate.coord.t[1] = 50;
        item->locate->locate.coord.t[2] = 0;
        item->mode++;
        return;

    case KUSURI_MODE_DRINK:
    {
        MotionManager *mot;

        mot = item->owner->motion;
        if (mot->mid != MOT_ITEM_DRINK)
        {
            /* animation interrupted: toss the item back out */
            VECTOR *pos;
            Humanoid *human;
            s32 itemID;
            PARAM_ITEM_LAUNCH p;

            pos = GetAbsolutePosition(item->locate, 0, 0, 0);
            human = item->owner;
            itemID = item->type;
            memset(&p, 0, sizeof(p));
            p.type = itemID;
            p.user = human;
            p.start.vx = pos->vx;
            p.start.vy = pos->vy;
            p.start.vz = pos->vz;
            p.end.vx = rand() % 200 - 100;
            p.end.vy = rand() % 100 - 200;
            p.end.vz = rand() % 200 - 100;
            ReqItemDrop(&p);
            ppu = item->proc;
            if (ppu == 0)
                return;
            DISPOSE_ITEM(item);
            return;
        }
        {
            s16 cnt;

            cnt = mot->count;
            if (cnt == 0x37)
            {
                item->mode = KUSURI_MODE_HEAL;
                return;
            }
            if (cnt < 4)
                return;
        }
    }
        UpdateCoordinate(item->locate);
        model->locate = item->locate->locate;
        model->scale = 0x2000;
        DrawSprite(model);
        return;

    case KUSURI_MODE_HEAL:
    {
        i = 0;
        item->owner->life = item->owner->lifemax;
        while (1)
        {
            if (i >= 0x14)
                break;
            {
                VECTOR pos = {
                    .vx = item->owner->model->locate.coord.t[0] +
                        (rand() % 1000 - 500),
                    .vy = item->owner->model->locate.coord.t[1] +
                        (rand() % 1000 - 1200),
                    .vz = item->owner->model->locate.coord.t[2] +
                        (rand() % 1000 - 500)
                };
                SVECTOR vec = {
                    .vx = 0,
                    .vy = rand() % 10 - 30,
                    .vz = 0
                };

                SetBleed(&pos, &vec, rand() % 0x10 + 0xf,
                         RGB24(255, 255, 126));
            }
            i++;
        }
        SoundEx(item->owner->locate, SE_MEDICINE);
        ppu = item->proc;
        if (ppu == 0)
            return;
        DISPOSE_ITEM(item);
    }
    }
}
