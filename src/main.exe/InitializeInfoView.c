#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitializeInfoView(void);
 *     INFOVIEW.C:119, 74 src lines, frame 64 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     reg   $s0       int i
 *     stack sp+16     unsigned char [25] image
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE CursorImage;
 *     extern struct Sprite3D *ItemImage[25];
 *     extern unsigned char fInitialize;
 * END PSX.SYM */

/*
 * InitializeInfoView (0x8004a790, 0x160 bytes) — one-time HUD/inventory init,
 * called from DoInfoViewProc the first frame (guarded by fInitialize) and
 * from main(). Sets up the cursor/digit sprites, the shared 26-slot item-image
 * table (ItemImage, images 0x14.. then padded with image 0xF),
 * and the 4-entry ItemSprite3Ds array, then resets enemy layout/info-view
 * state and marks fInitialize.
 *
 * STATUS: MATCHING — 352 bytes. The loop-1 SetupSprite result must be
 * assigned through `*slot` in one expression: that assignment chain keeps
 * the result live long enough for the following slot reload to take `$v1`,
 * leaving `$v0` for the 0x1C attribute constant.
 *
 * Matching notes:
 *  - Loops 1 and 2 (the ItemImage pool) are HAND-ROLLED GOTO LOOPS, not
 *    do-whiles, despite Ghidra/m2c rendering plain `do{}while`: loop 1's
 *    0x1C attribute constant is RE-MATERIALIZED every iteration
 *    (`li v0,0x1c` inside the loop body each pass) rather than hoisted to a
 *    preheader, which only happens with NO loop notes for loop.c to act on
 *    (same lesson as PutNumber's /10 magic constant and
 *    SelectCameraOwnerOption's `&targets[i]`). The 0x3000 scale, by
 *    contrast, IS hoisted before loop 1 — but that isn't loop.c invariant
 *    motion either: it's a plain NAMED VARIABLE (`scale1`) assigned once
 *    before the (goto) loop and read every iteration, exactly like
 *    `buffer`/`ppHVar4` persist across SelectCameraOwnerOption's goto loop.
 *    Loop 2 hoists BOTH its scale and attribute the same way (two more
 *    named locals, `scale2`/`attr2`, assigned right before loop 2's own
 *    goto-loop body, in registers distinct from loop 1's).
 *  - Loop 3 (ItemSprite3Ds) stays a genuine `do{}while`: Ghidra's rendering
 *    already matches (its one constant, 0x50000000, is likewise just a
 *    plain pre-loop variable read, and the array is walked with a typed
 *    `GsSPRITE *` pointer with no strength-reduction concern).
 *  - `ItemImage` is walked through its real `Sprite3D **` element type.
 *    `item` retains SetupSprite's result for the scale store, while
 *    `(*slot)->attribute` deliberately re-reads the pointer from the array;
 *    that is the target's slot reload without erasing the pointer type or
 *    spelling the recovered +0x5A field as a raw offset.
 *  - `fInitialize` is this TU's gp small; maspsxGpExterns is PER FILE (each
 *    split function is its own assembly unit), so this file needs its OWN
 *    Build.hs entry — DoInfoViewProc.c's entry only covers DoInfoViewProc.c.
 *  - `ItemImage`'s auto-computed address DRIFTS depending on which nearby
 *    functions are still raw-asm vs compiled C (this function converting to
 *    C removed the last raw xref that had been anchoring it, and it
 *    resolved +0x28 off) — bound explicitly in config/symbols.main.exe.txt
 *    (plain name: ReqItemDrop/ReqItemMakibishi/
 *    ReqItemManebue already reference this exact symbol as
 *    `extern Sprite3D *ItemImage[];` and need the SAME name fixed under
 *    them, unlike the 0x80097Dxx string-table drift where nothing else
 *    referenced the bad name).
 */
extern GsSPRITE ItemSprite3Ds[4];
extern u8 fInitialize;

extern GsIMAGE *GetImage(s32 id);
extern Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image);
extern void leResetEnemyLayout(void);
extern void ResetInfoview(s32 stage);
extern void FUN_8004a6bc(void);

void InitializeInfoView(void)
{
    GsIMAGE *image;
    Sprite3D *item;
    Sprite3D **slot;
    Sprite3D **base2;
    GsSPRITE *sprite;
    int i;
    s32 scale1;
    s32 scale2;
    s32 attr3;
    s32 attr2;

    image = GetImage(0x32);
    InitSprite(image, &CursorImage);
    CursorImage.attribute = 0x50000000;
    image = GetImage(0x33);
    InitSprite(image, &NumberImage);
    i = 0;
    scale1 = 0x3000;
    slot = ItemImage;
loop1:
    image = GetImage(i + 0x14);
    item = (*slot = SetupSprite(0, image));
    item->scale = scale1;
    (*slot)->attribute = 0x1C;
    slot = slot + 1;
    if (++i < 0x14)
        goto loop1;
    if (i < 0x1A)
    {
        scale2 = 0x3000;
        attr2 = 0x1C;
        base2 = ItemImage;
        slot = base2 + i;
    loop2:
        image = GetImage(0xF);
        item = SetupSprite(0, image);
        *slot = item;
        item->scale = scale2;
        (*slot)->attribute = attr2;
        slot = slot + 1;
        if (++i < 0x1A)
            goto loop2;
    }
    i = 0;
    attr3 = 0x50000000;
    sprite = ItemSprite3Ds;
    do
    {
        image = GetImage(i + 0x2E);
        InitSprite(image, sprite);
        sprite->attribute = attr3;
        i = i + 1;
        sprite = sprite + 1;
    } while (i < 4);
    leResetEnemyLayout();
    ResetInfoview(-1);
    FUN_8004a6bc();
    fInitialize = 1;
}
