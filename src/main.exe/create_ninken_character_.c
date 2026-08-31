#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"

/*
 * MATCH.
 *
 * Creates the persistent ninken character, then snapshots the selected
 * character's model and a temporary table-selected character model for the
 * disguise logic consumed by ProcItemHenshin.
 *
 * Matching notes:
 *  - Each output buffer has the saved `waist` value followed by ordinary
 *    12-byte model-part snapshots (`tmd`, `x`, `y`, and `z`).
 *  - The two model-copy phases need separate block-scoped model, saved, and
 *    index locals. Reusing one set across both phases joins their pseudos,
 *    rotates the caller-saved registers, and fills three target load-delay
 *    nops; distinct source identities reproduce the exact allocation.
 *  - The selected character model is read through the recovered shared
 *    `CamState.Owner` field.
 */
extern Humanoid *NINKEN_CHARACTER_PTR;

void create_ninken_character_(s16 type, s32 stage)
{
    NINKEN_CHARACTER_PTR = BreedLife(NINKEN, NINKEN_PARK_POS, NINKEN_PARK_POS, NINKEN_PARK_POS, 0);
    NINKEN_CHARACTER_PTR->attribute |= ATTR_SUSPEND;

    {
        ModelArchiveType *model;
        HenshinModelSnapshot *saved;
        s32 i;

        model = CamState.Owner->model;
        i = 0;
        saved = &Item_save;
        saved->waist = model->rotate.pad;
        if (model->n > 0)
        {
            do
            {
                saved->p[i].tmd = model->object[i]->object.tmd;
                saved->p[i].x =
                    model->object[i]->locate.coord.t[0];
                saved->p[i].y =
                    model->object[i]->locate.coord.t[1];
                saved->p[i].z =
                    model->object[i]->locate.coord.t[2];
                i++;
            } while (i < model->n);
        }
    }

    {
        Humanoid *human;
        ModelArchiveType *model;
        HenshinModelSnapshot *saved;
        s32 i;
        s32 flag;

        flag = (type == 1);
        human = BreedLife(HensinT[(s16)stage].type[flag],
                          NINKEN_PARK_POS, NINKEN_PARK_POS, NINKEN_PARK_POS, 0);
        model = human->model;
        i = 0;
        saved = &HenshinSnapshot;
        saved->waist = model->rotate.pad;
        if (model->n > 0)
        {
            do
            {
                saved->p[i].tmd = model->object[i]->object.tmd;
                saved->p[i].x =
                    model->object[i]->locate.coord.t[0];
                saved->p[i].y =
                    model->object[i]->locate.coord.t[1];
                saved->p[i].z =
                    model->object[i]->locate.coord.t[2];
                i++;
            } while (i < model->n);
        }
        KillHumanoid(human);
    }
}
