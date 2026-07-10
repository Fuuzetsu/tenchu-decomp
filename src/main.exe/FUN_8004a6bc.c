#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct WorldType WorldMap[8][8][8];
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 84 of 212 bytes differ (whole-image count matches
 * the window count: 0 downstream drift, so the LENGTH is already exactly
 * right — this is a pure scheduling/statement-order residual, not a
 * structural one).
 *
 * FUN_8004a6bc (0x8004a6bc, 0xd4 bytes) — INFOVIEW.C: initializes two pairs
 * of life-bar element sprites (attribute 0x40000000 / 0x50000000 — likely
 * two different GsSortSprite draw-primitive kinds for the same bar, e.g. a
 * background half and a fill half), one pair per "style" for 2 styles, each
 * pair a `GsSPRITE[2]` 0x50 bytes apart (0x24-byte GsSPRITE pair + 8 bytes
 * of other per-style fields PutLifeBar.c also touches around this same
 * region: D_8008e414/e416/e418/e41a sit immediately before this function's
 * first sprite D_8008E41C, and PutLifeBar indexes the very same
 * D_8008E41C/D_8008E440 sprites by `style * 0x50`). Only called by
 * (still-asm) InitializeInfoView, alongside ResetInfoview/leResetEnemyLayout
 * — i.e. this is InitializeInfoView's life-bar sprite setup, most likely
 * named InitLifeBar or similar (no candidate in reference/psxsym-
 * candidates.tsv to corroborate).
 *
 * GsSPRITE's fields (attribute/mx/my/rotate) are the proven PSY-Q SDK
 * layout already used by InitSprite.c/PutLifeBar.c (include/psxsdk/libgs.h).
 * D_8008E4B4 is a small local per-style source table (not referenced by any
 * other matched function): a `long` forwarded into both sprites' `rotate`
 * field, plus one image-id byte per sprite (fed straight to GetImage).
 * Indexed by the SAME loop counter used for the `i < 2` test (`D_8008E4B4
 * [i]`), not walked with its own incrementing pointer: touching 2+ fields
 * (word0 and both id bytes) per iteration through a raw walking pointer
 * makes cc1's strength reduction split off a SECOND parallel induction
 * register for the byte offsets (verified — every walking-pointer spelling
 * tried materializes `entry+5`/`entry+1` into its own register, 4 bytes /
 * 1 reg-pair too many); `T[i].f` keeps the loop's only address GIV at the
 * table base, matching the cookbook's "index the table" loop lever.
 * The second sprite's base (`&D_8008E41C + 0x24`, one GsSPRITE past the
 * first) is likewise a constant expression, not a separately named
 * variable, so loop.c hoists it on its own — matching the target's
 * precomputed `s5 = s4+0x24` loop invariant.
 *
 * Residual: correct length (212 bytes both), correct register SET (s0-s5
 * per tools/regalloc.py, matching target), but the target computes each
 * slot's address BEFORE evaluating GetImage's argument (and schedules the
 * loop counter's `+1` into the call's delay slot), while our draft
 * schedules the opposite way and ends up with a smaller declared local
 * frame (0x30 vs target's 0x50) despite the same register set. Permuter
 * run in progress (same-length residual — cookbook says try it, not park
 * it) via tools/permute.py FUN_8004a6bc.
 */
typedef struct
{
    s32 rotate; /* +0x0, forwarded into both sprites' `rotate` verbatim */
    u8 imgA;    /* +0x4 */
    u8 imgB;    /* +0x5 */
    u8 pad[2];  /* +0x6 */
} LifeBarSpriteEntry;

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8004a6bc", FUN_8004a6bc);
#else
extern GsSPRITE D_8008E41C[];
extern LifeBarSpriteEntry D_8008E4B4[];
extern GsIMAGE *GetImage(s32 id);
extern void InitSprite(GsIMAGE *image, GsSPRITE *sprite);

void FUN_8004a6bc(void)
{
    GsSPRITE *slot;
    s32 tmp;
    int off;
    int idx;
    int i;

    i = 0;
    off = 0;
    idx = 0;
    do
    {
        slot = (GsSPRITE *)((u8 *)D_8008E41C + off);
        i = i + 1;
        InitSprite(GetImage(D_8008E4B4[idx].imgA), slot);
        slot->mx = 0;
        slot->my = 0;
        slot->attribute = 0x40000000;
        slot->rotate = D_8008E4B4[idx].rotate;

        slot = (GsSPRITE *)((u8 *)D_8008E41C + 0x24 + off);
        off = off + 0x50;
        InitSprite(GetImage(D_8008E4B4[idx].imgB), slot);
        slot->mx = 0;
        slot->my = 0;
        tmp = D_8008E4B4[idx].rotate;
        idx = idx + 1;
        slot->attribute = 0x50000000;
        slot->rotate = tmp;
    } while (i < 2);
}
#endif
