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
 * DeleteConflict.c, memset each slot's `.result` (0x50 bytes — deliberately
 * overflows the declared 64-byte field into the trailing 0x10 pad, same as
 * InsertConflict.c), then clear ConflictModel/ConflictDistance/
 * ConflictObjects. `.offset`/`.size = UnitVector;` are plain SVECTOR struct
 * assignments (align-2 lwl/lwr+swl/swr); Ghidra's SUB42/uVar4 byte-shuffle
 * rendering is that same idiom (cookbook "Stack objects": cast-type
 * alignment drives copy code), not a manual field-by-field copy.
 *
 * The loop counter is `short i` (PSX.SYM's own `reg $s0 short i`; Ghidra's
 * `iVar7 * 0x10000 >> 0x10` bound-test is the short-loop-counter idiom —
 * cookbook Loops), a guarded `for (i = 0; i < 0x50; i++)`.
 */

/* Conflict slot (Ghidra: ConflictObjectType, 0x78 bytes; see
 * InsertConflict.c/DeleteConflict.c — identical per-TU local declaration). */
typedef struct
{
    ModelType *model;            /* 0x00 */
    VECTOR position;             /* 0x04 */
    SVECTOR offset;              /* 0x14 */
    SVECTOR size;                /* 0x1C */
    void *common;                /* 0x24 */
    u8 result[64];               /* 0x28 */
    u8 pad[0x10];                /* 0x68 */
} ConflictObjectType;            /* 0x78 */

extern ConflictObjectType ConflictObject[];
extern s16 ConflictObjects;
extern VECTOR UnitVector2;
extern SVECTOR UnitVector;
extern SVECTOR ConflictDistance; /* 0x80097EC8 (vx/vy/vz) */
extern ModelType *ConflictModel; /* 0x80097ED0 */
extern void *memset(void *s, int c, u32 n);

void InitConflict(void)
{
    short i;

    for (i = 0; i < 0x50; i++)
    {
        ConflictObject[i].model = 0;
        ConflictObject[i].common = 0;
        ConflictObject[i].position = UnitVector2;
        ConflictObject[i].offset = UnitVector;
        ConflictObject[i].size = UnitVector;
        memset(ConflictObject[i].result, 0, 0x50);
    }
    ConflictModel = 0;
    ConflictDistance = UnitVector;
    ConflictObjects = 0;
}
