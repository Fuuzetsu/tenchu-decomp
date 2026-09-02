#include "common.h"
#include "tuning.h"
#include "sound.h"
#include "main.exe.h"

/*
 * ProcItemKusuri (0x80040500) — the kusuri (healing potion) item processor.
 * mode 0: freeze the drinker (dispose weapon, drink animation 0xF01, status 0xF),
 * attach the item model to the character's hand and nudge it into place; mode 1:
 * while the drink animation plays, track the hand (copy the item coordinate into
 * the sprite and draw it); at animation frame 0x37 switch to mode 2; if the
 * animation was interrupted, drop the item (ReqItemDrop with a random toss) and
 * dispose; mode 2: heal to full, spray 20 random SetBleed particles, play the
 * gulp sound, and dispose of the item.
 *
 * Matching notes (each verified against the original bytes; see also
 * ProcItemManebue.c for the item-TU conventions):
 *  - `ITEM_MODE_DISPOSE` holds ITEM_MODE_DISPOSE in a callee-saved reg ($s4) across calls:
 *    used by the entry test and the drop path's `item->mode = ITEM_MODE_DISPOSE`; mode 2's
 *    dispose rematerializes its 0xff value instead ($s4 holds the particle
 *    position workspace by then).
 *  - The dispatch is a real `switch`: it reloads item->mode (fresh index load)
 *    and compares it SIGNED (slti) — an if-ladder CSEs the load and compares
 *    unsigned. Case bodies sit in source order (0, 1, 2).
 *  - `i = 0` is case 2's first statement (reorg hoists it into the case-2
 *    branch delay slot). The bleed loop is `while (1) { if (!(i < 0x14))
 *    break; ...; i++; }` — a for/while-with-condition gets its exit test
 *    duplicated at the entry (jump.c duplicate_loop_exit_test) and then
 *    constant-folded away; the while(1)+break form keeps the original's
 *    top-test + unconditional back-jump while still letting loop.c hoist the
 *    invariants (&build, &pos, and the two magic divisors).
 *  - mode 2's jitter is written `t[n] + (rand() % 1000 - 500)`: fold's
 *    associate step canonicalizes it to the original's (t[n]-500) + rem shape,
 *    whereas writing `t[n] - 500 + rand() % 1000` gets reassociated the wrong
 *    way (constant pulled onto the remainder).
 *  - PSX.SYM places the interrupted-drop `p` and mode-2 `pos` at sp+16,
 *    and `vec` at sp+32. Keeping the launch request and particle workspace in
 *    their actual switch arms lets GCC reuse those disjoint stack lifetimes;
 *    no source union is needed. The work VECTOR at sp+32 is copied into `pos`,
 *    then its now-dead storage builds and holds the short velocity vector.
 *  - The dispose tail is written out twice (drop path + mode 2); GCC's
 *    cross-jump merges the common suffix from the jalr on. The null check
 *    reads `ppu = item->proc` but the call is `item->proc(item)` (cse reuses
 *    the load) — checking and calling through `ppu` allocates $v1 instead of
 *    the original's $v0.
 */
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
            item->mode = ITEM_MODE_DISPOSE;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != KUSURI_MODE_START)
            {
                AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
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
        VECTOR pos;
        VECTOR build;
        SVECTOR *vec;

        i = 0;
        item->owner->life = item->owner->lifemax;
        while (1)
        {
            if (i >= 0x14)
                break;
            memset(&build, 0, sizeof(build));
            build.vx = item->owner->model->locate.coord.t[0] +
                       (rand() % 1000 - 500);
            build.vy = item->owner->model->locate.coord.t[1] +
                       (rand() % 1000 - 1200);
            build.vz = item->owner->model->locate.coord.t[2] +
                       (rand() % 1000 - 500);
            pos = build;
            memset(&((SVECTOR *)&build)[1], 0, sizeof(SVECTOR));
            ((SVECTOR *)&build)[1].vy = rand() % 10 - 30;
            vec = (SVECTOR *)&build;
            vec[0] = vec[1];
            SetBleed(&pos, vec, rand() % 0x10 + 0xf,
                     RGB24(255, 255, 126));
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
