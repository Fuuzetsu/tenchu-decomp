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

/*
 * InitMisc (0x8004c1e8, 0x168 bytes) — one-time setup of the misc-object
 * pool: clears all MaxMisc slots (bottom-test walking-pointer loop,
 * `misc + (MaxMisc - 1)` down to `misc`), then loads the door/pitfall
 * pair-of-models tables and the two ambient sprites (rain/snow "steam"),
 * and finally sets the init latch DoMiscProc waits on.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `misc + (MaxMisc - 1)` (a nonzero-offset pointer into a big absolute
 *    extern
 *    array) forces materialization of misc's OWN base address (lui+addiu)
 *    plus a THIRD addiu for the +(MaxMisc-1)*sizeof(TMisc) offset — the
 *    "offset-0 folds, a nonzero offset materializes" rule; ordinary pointer
 *    arithmetic reproduces it with no special spelling.
 *  - DoorData/PitfallData's `Model[2]` fields begin as archive-index words
 *    (or -1 for "none") and are overwritten with loaded ModelType pointers.
 *    MiscModelReference exposes those two lifecycle views directly.
 *    Retail's address delta from PitfallData to SpriteData is
 *    N_PITFALL_TYPES records, and the loop handles the same number of
 *    variants; the demo declaration had only 2.
 *  - Both `Model[0]`/`Model[1]` are read UNCONDITIONALLY before either `if`
 *    (`iDoor2` cached because the first `if`'s GetArcData/LoadModel calls
 *    would clobber a caller-saved copy of it; `iDoor1` is consumed
 *    immediately by its own call and needs no separate temp beyond the
 *    parameter register) — same "pointer/value cached only when it must
 *    survive a call" shape as ProcMiscDoor's twins.
 *  - SpriteData's own `.spr` field is likewise read as the GetImage index
 *    BEFORE being overwritten with the real Sprite3D*.
 *  - The final `Misc_fInitial = 1;` is MISC.C's original file-static
 *    `fInitial`, qualified for the split decomp; DoMiscProc reads it.
 */

extern ModelType *LoadModel(u_long *adr);
extern GsIMAGE *GetImage(s32 index);
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
            iDoor1 = DoorData[i].Model[0].archive_id;
            iDoor2 = DoorData[i].Model[1].archive_id;
            if (iDoor1 != MODEL_ARCHIVE_NONE)
            {
                data = LoadModel(GetArcData(iDoor1));
                DoorData[i].Model[0].model = data;
            }
            if (iDoor2 != MODEL_ARCHIVE_NONE)
            {
                data = LoadModel(GetArcData(iDoor2));
                DoorData[i].Model[1].model = data;
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
            spr->spr.sprite = SetupSprite((Sprite3D *)0,
                                          GetImage(spr->spr.image_id));
            spr->spr.sprite->sprite.attribute = attr;
            spr->spr.sprite->scale = spr->scale;
            spr++;
        } while (i < N_MISC_SPRITE_TYPES);
    }

    {
        ModelArchiveId id1;
        ModelArchiveId id2;
        ModelType *data;

        for (i = 0; i < N_PITFALL_TYPES; i++)
        {
            id1 = PitfallData[i].Model[0].archive_id;
            id2 = PitfallData[i].Model[1].archive_id;
            if (id1 != MODEL_ARCHIVE_NONE)
            {
                data = LoadModel(GetArcData(id1));
                PitfallData[i].Model[0].model = data;
            }
            if (id2 != MODEL_ARCHIVE_NONE)
            {
                data = LoadModel(GetArcData(id2));
                PitfallData[i].Model[1].model = data;
            }
        }
    }

    Misc_fInitial = 1;
}
