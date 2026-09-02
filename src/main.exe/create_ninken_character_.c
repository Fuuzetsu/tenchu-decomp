#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"

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
