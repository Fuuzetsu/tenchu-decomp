#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "model.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct OrnamentType * CreateCloneOrnament(struct OrnamentType *objp);
 *     3DCTRL.C:518, 10 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct OrnamentType * objp
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType World;
 *     extern struct SVECTOR UnitVector;
 * END PSX.SYM */

extern void *valloc(u32 size);

OrnamentType *CreateCloneOrnament(OrnamentType *objp)
{
    OrnamentType *ornament;

    ornament = (OrnamentType *)valloc(sizeof(OrnamentType));
    INITIALIZE_ORNAMENT_INSTANCE(ornament, &World.locate);
    if (objp != 0)
    {
        ornament->object.tmd = objp->object.tmd;
    }
    return ornament;
}
