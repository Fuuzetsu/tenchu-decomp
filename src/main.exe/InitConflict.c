#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitConflict(void);
 *     CONFLICT.C:260, 19 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct VECTOR UnitVector2;
 *     extern struct SVECTOR UnitVector;
 *     extern struct ModelType *ConflictModel;
 *     extern struct SVECTOR ConflictDistance;
 *     extern short ConflictObjects;
 * END PSX.SYM */

extern void *memset(void *s, int c, u32 n);

void InitConflict(void)
{
    short i;

    for (i = 0; i < N_CONFLICT_OBJECTS; i++)
    {
        ConflictObject[i].model = 0;
        ConflictObject[i].common = (void *)CONFLICT_OWNER_NONE;
        ConflictObject[i].position = UnitVector2;
        ConflictObject[i].offset = UnitVector;
        ConflictObject[i].size = UnitVector;
        memset(ConflictObject[i].result, 0, sizeof(ConflictObject[i].result));
    }
    ConflictModel = 0;
    ConflictDistance = UnitVector;
    ConflictObjects = 0;
}
