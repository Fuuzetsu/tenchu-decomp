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
        saved = &Item_save;
        CAPTURE_HENSHIN_MODEL(saved, model, i);
    }

    {
        Humanoid *human;
        ModelArchiveType *model;
        HenshinModelSnapshot *saved;
        s32 i;
        s32 flag;

        flag = (type == AYAME_0);
        human = BreedLife(HensinT[(s16)stage].type[flag],
                          NINKEN_PARK_POS, NINKEN_PARK_POS, NINKEN_PARK_POS, 0);
        model = human->model;
        saved = &HenshinSnapshot;
        CAPTURE_HENSHIN_MODEL(saved, model, i);
        KillHumanoid(human);
    }
}
