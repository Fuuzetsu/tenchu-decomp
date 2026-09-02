#include "common.h"
#include "main.exe.h"
#include "misc.h"
#include "images.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitMisc(void);
 *     MISC.C:162, 83 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s1       int i
 *     reg   $a0       int iDoor1
 *     reg   $s1       int iDoor2
 *     reg   $v0       struct ModelType * data
 *     reg   $a0       int id1
 *     reg   $s1       int id2
 *     reg   $v0       struct ModelType * data
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TMisc misc[200];
 *     extern struct MISC__183fake DoorData[11];
 *     extern struct MISC__185fake SpriteData[2];
 *     extern struct MISC__184fake PitfallData[2];
 * END PSX.SYM */

extern ModelType *LoadModel(u_long *adr);
extern Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image);

void InitMisc(void)
{
    TMisc *tm;
    s32 i;

    i = MaxMisc - 1;
    tm = misc;
    tm += (MaxMisc - 1);
    do
    {
        tm->proc = 0;
        i--;
        tm--;
    } while (i >= 0);

    {
        ModelArchiveId iDoor1;
        ModelArchiveId iDoor2;
        ModelType *data;

        for (i = 0; i < N_DOOR_TYPES; i++)
        {
            iDoor1 = (ModelArchiveId)DoorData[i].Model[0];
            iDoor2 = (ModelArchiveId)DoorData[i].Model[1];
            if (iDoor1 != MODEL_ARCHIVE_NONE)
            {
                data = LoadModel(GetArcData(iDoor1));
                DoorData[i].Model[0] = data;
            }
            if (iDoor2 != MODEL_ARCHIVE_NONE)
            {
                data = LoadModel(GetArcData(iDoor2));
                DoorData[i].Model[1] = data;
            }
        }
    }

    {
        SpriteDataType *spr;
        u32 attr;

        i = 0;
        attr = GS_ATTR_SEMITRANS_ADD;
        spr = SpriteData;
        do
        {
            i++;
            spr->spr = SetupSprite((Sprite3D *)0,
                                   GetImage((ImageArchiveId)spr->spr));
            spr->spr->sprite.attribute = attr;
            spr->spr->scale = spr->scale;
            spr++;
        } while (i < N_MISC_SPRITE_TYPES);
    }

    {
        ModelArchiveId id1;
        ModelArchiveId id2;
        ModelType *data;

        for (i = 0; i < N_PITFALL_TYPES; i++)
        {
            id1 = (ModelArchiveId)PitfallData[i].Model[0];
            id2 = (ModelArchiveId)PitfallData[i].Model[1];
            if (id1 != MODEL_ARCHIVE_NONE)
            {
                data = LoadModel(GetArcData(id1));
                PitfallData[i].Model[0] = data;
            }
            if (id2 != MODEL_ARCHIVE_NONE)
            {
                data = LoadModel(GetArcData(id2));
                PitfallData[i].Model[1] = data;
            }
        }
    }

    Misc_fInitial = 1;
}
