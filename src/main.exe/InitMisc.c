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
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 *  - DoorData/PitfallData's `Model[2]` fields double as int archive-index
 *    slots before this function ever runs (the original static initializer
 *    packs a GetArcData index — or -1 for "none" — into the same word later
 *    overwritten with the loaded ModelType pointer); PSX.SYM's own locals
 *    (`int iDoor1`/`iDoor2`) confirm the field is READ as a plain int here,
 *    cast off the ModelType* field (`(s32)door->Model[0]`), while other
 *    already-matched files access the same field as a ModelType* — no
 *    conflict, this is a different TU's own read of the field's raw bits.
 *    Retail's address delta from PitfallData to SpriteData is 3 records, and
 *    the loop likewise handles 3 variants; the demo declaration had only 2.
 *  - Both `Model[0]`/`Model[1]` are read UNCONDITIONALLY before either `if`
 *    (`iDoor2` cached because the first `if`'s GetArcData/LoadModel calls
 *    would clobber a caller-saved copy of it; `iDoor1` is consumed
 *    immediately by its own call and needs no separate temp beyond the
 *    parameter register) — same "pointer/value cached only when it must
 *    survive a call" shape as ProcMiscDoor's twins.
 *  - SpriteData's own `.spr` field is likewise read (cast to int) as the
 *    GetImage index BEFORE being overwritten with the real Sprite3D*.
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
    tm = tm + (MaxMisc - 1);
    do
    {
        tm->proc = 0;
        i--;
        tm--;
    } while (i >= 0);

    {
        s32 iDoor1;
        s32 iDoor2;
        ModelType *data;

        for (i = 0; i < 11; i++)
        {
            iDoor1 = (s32)DoorData[i].Model[0];
            iDoor2 = (s32)DoorData[i].Model[1];
            if (iDoor1 != -1)
            {
                data = LoadModel(GetArcData(iDoor1));
                DoorData[i].Model[0] = data;
            }
            if (iDoor2 != -1)
            {
                data = LoadModel(GetArcData(iDoor2));
                DoorData[i].Model[1] = data;
            }
        }
    }

    {
        SpriteDataType *spr;
        GsIMAGE *image;
        Sprite3D *sprite;
        u32 attr;

        i = 0;
        attr = SPR_TRANS_ADD;
        spr = SpriteData;
        do
        {
            i++;
            image = GetImage((s32)spr->spr);
            sprite = SetupSprite((Sprite3D *)0, image);
            spr->spr = sprite;
            sprite->sprite.attribute = attr;
            spr->spr->scale = spr->scale;
            spr++;
        } while (i < 2);
    }

    {
        s32 id1;
        s32 id2;
        ModelType *data;

        for (i = 0; i < 3; i++)
        {
            id1 = (s32)PitfallData[i].Model[0];
            id2 = (s32)PitfallData[i].Model[1];
            if (id1 != -1)
            {
                data = LoadModel(GetArcData(id1));
                PitfallData[i].Model[0] = data;
            }
            if (id2 != -1)
            {
                data = LoadModel(GetArcData(id2));
                PitfallData[i].Model[1] = data;
            }
        }
    }

    Misc_fInitial = 1;
}
