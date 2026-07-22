#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct INFOVIEW__196fake LifeBarStyle[2];
 * END PSX.SYM */

/*
 * STATUS: MATCHING — 212 bytes.
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
 * Indexed by the SAME loop counter used for the `i < nLifeBarStyle` test
 * (`D_8008E4B4[i]`), not walked with its own incrementing pointer: touching
 * 2+ fields
 * (word0 and both id bytes) per iteration through a raw walking pointer
 * makes cc1's strength reduction split off a SECOND parallel induction
 * register for the byte offsets (verified — every walking-pointer spelling
 * tried materializes `entry+5`/`entry+1` into its own register, 4 bytes /
 * 1 reg-pair too many); `T[i].f` keeps the loop's only address GIV at the
 * table base, matching the cookbook's "index the table" loop lever.
 * Keeping the proven `LifeBarStyle` aggregate is also load-bearing: direct
 * `frame`/`fill` indexing lets loop.c derive the target's one 0x50-byte style
 * offset and hoist the two sprite bases (`s4` and `s5 = s4+0x24`). Flattening
 * those fields into a raw sprite array leaves the right instructions but
 * initializes the offset GIV too early in the prologue.
 *
 * The otherwise-unused `image[25]` declaration accounts for the target's
 * 0x20 bytes of local-frame space. This is source-backed rather than padding:
 * PSX.SYM records that exact local name/type at sp+16 in the demo's combined
 * InitializeInfoView, whose life-bar initialization loop was later split into
 * this retail helper. Finally, assign `rotate` before `attribute` for the first
 * sprite. cc1 then schedules the rotate load before the attribute constant and
 * moves `i++` into GetImage's delay slot, exactly as in the target.
 */
typedef struct
{
    s32 rotate; /* +0x0, forwarded into both sprites' `rotate` verbatim */
    u8 imgA;    /* +0x4 */
    u8 imgB;    /* +0x5 */
} LifeBarSpriteEntry;

extern LifeBarSpriteEntry D_8008E4B4[];
extern GsIMAGE *GetImage(s32 id);

void FUN_8004a6bc(void)
{
    u8 image[25];
    GsSPRITE *slot;
    s32 tmp;
    int i;

    for (i = 0; i < nLifeBarStyle; i++)
    {
        slot = &LifeBarStyle[i].frame;
        InitSprite(GetImage(D_8008E4B4[i].imgA), slot);
        slot->mx = 0;
        slot->my = 0;
        slot->rotate = D_8008E4B4[i].rotate;
        slot->attribute = 0x40000000;

        slot = &LifeBarStyle[i].fill;
        InitSprite(GetImage(D_8008E4B4[i].imgB), slot);
        slot->mx = 0;
        slot->my = 0;
        tmp = D_8008E4B4[i].rotate;
        slot->attribute = 0x50000000;
        slot->rotate = tmp;
    }
}
