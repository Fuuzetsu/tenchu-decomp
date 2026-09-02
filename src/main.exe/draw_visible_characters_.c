#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "afterimage.h"
#include "tmdfast.h"

extern short DrawModelArchive(ModelArchiveType *mad, long gap);
extern short DrawOrnament(OrnamentType *objp);

void draw_visible_characters_(void)
{
    s16 i;
    Humanoid *cs;

    for (i = 0; i < VISIBLE_ENEMIES_; i++)
    {
        cs = VISIBLE_CHARACTERS_ON_STAGE_[i];
        DrawTMDmode = DrawModeSave[i];
        DrawModelArchive(cs->model, -i);
        if (cs->weapon[WEAPON_SLOT_ACTIVE_0] != 0)
        {
            DrawOrnament(cs->weapon[WEAPON_SLOT_ACTIVE_0]);
        }
        if (cs->weapon[WEAPON_SLOT_ACTIVE_1] != 0)
        {
            DrawOrnament(cs->weapon[WEAPON_SLOT_ACTIVE_1]);
        }
        if (cs->illusion[WEAPON_HAND_0] != 0)
        {
            DrawAfterimage(cs->illusion[WEAPON_HAND_0], 1);
        }
        if (cs->illusion[WEAPON_HAND_1] != 0)
        {
            DrawAfterimage(cs->illusion[WEAPON_HAND_1], 1);
        }
    }
}
