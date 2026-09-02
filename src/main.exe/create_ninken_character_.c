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
 *  - The two model-copy phases need separate block-scoped model and saved
 *    pointers. CaptureHenshinModel's index is likewise cloned into each
 *    inline expansion. Reusing one set across both phases joins their
 *    pseudos, rotates the caller-saved registers, and fills three target
 *    load-delay nops; distinct source identities reproduce the exact
 *    allocation.
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

        model = CamState.Owner->model;
        saved = &Item_save;
        CaptureHenshinModel(saved, model);
    }

    {
        Humanoid *human;
        ModelArchiveType *model;
        HenshinModelSnapshot *saved;
        s32 flag;

        flag = (type == AYAME_0);
        human = BreedLife(HensinT[(s16)stage][flag],
                          NINKEN_PARK_POS, NINKEN_PARK_POS, NINKEN_PARK_POS, 0);
        model = human->model;
        saved = &HenshinSnapshot;
        CaptureHenshinModel(saved, model);
        KillHumanoid(human);
    }
}
