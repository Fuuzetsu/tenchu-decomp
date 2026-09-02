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

/*
 * InitConflict (0x8001a308, 0x13c bytes) — resets the whole conflict-box
 * pool at boot: zero every ConflictObject[i]'s model/common, seed
 * .position/.offset/.size from the shared UnitVector2 (VECTOR)/UnitVector
 * (SVECTOR) identity constants exactly like InsertConflict.c/
 * DeleteConflict.c, clear each slot's retail-expanded 80-byte `.result`,
 * then clear ConflictModel/ConflictDistance/
 * ConflictObjects. The `.vector = UnitVector` writes are plain SVECTOR struct
 * assignments (align-2 lwl/lwr+swl/swr); Ghidra's SUB42/uVar4 byte-shuffle
 * rendering is that same idiom (cookbook "Stack objects": cast-type
 * alignment drives copy code), not a manual field-by-field copy.
 *
 * The loop counter is `short i` (PSX.SYM's own `reg $s0 short i`; Ghidra's
 * `iVar7 * 0x10000 >> 0x10` bound-test is the short-loop-counter idiom —
 * cookbook Loops), a guarded `for (i = 0; i < N_CONFLICT_OBJECTS; i++)`.
 */

extern void *memset(void *s, int c, u32 n);

void InitConflict(void)
{
    short i;

    for (i = 0; i < N_CONFLICT_OBJECTS; i++)
    {
        ConflictObject[i].model = 0;
        ConflictObject[i].common = (void *)CONFLICT_OWNER_NONE;
        ConflictObject[i].position = UnitVector2;
        ConflictObject[i].offset.vector = UnitVector;
        ConflictObject[i].size.vector = UnitVector;
        memset(ConflictObject[i].result, 0, sizeof(ConflictObject[i].result));
    }
    ConflictModel = 0;
    ConflictDistance = UnitVector;
    ConflictObjects = 0;
}
