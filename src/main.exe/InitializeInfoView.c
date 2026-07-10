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
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
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
 *     extern unsigned char fInitialize;
 * END PSX.SYM */

/*
 * InitializeInfoView (0x8004a790, 0x160 bytes) — one-time HUD/inventory init,
 * called from DoInfoViewProc the first frame (guarded by fInitialize) and
 * from main(). Sets up the cursor/digit sprites, a 26-slot pool of debug
 * item-icon sprites (D_8008E5BC, images 0x14.. then padded with image 0xF),
 * and the 4-entry ItemSprite3Ds array, then resets enemy layout/info-view
 * state and marks fInitialize.
 *
 * STATUS: NON_MATCHING — 4 of 352 bytes differ (1 extra instruction; the
 * loop-shape/gp/drift fixes below are all verified correct against the
 * target). Residual, in loop 1's body: the target reloads `*piVar3` into
 * `$v1` then fills the load-delay slot with `addiu $s1,$s1,1` (iVar4++)
 * before the `li v0,0x1c` / `sh v0,0x5a($v1)` attribute store; the draft's
 * scheduler instead floats the SAME increment to right after the two `sw`s
 * (an independent instruction with nothing else ready to fill that earlier
 * slot), leaving a genuine `nop` in loop 2's equivalent delay slot later —
 * moving the source statement (`iVar4 = iVar4 + 1;`) to every tried position
 * around the loop body produced byte-identical output each time, so this is
 * sched1's own placement, not reachable from source order. A second,
 * related residual: loop 2's `attr2 = 0x1C;` compiles as `move $s2,$v1`
 * (copy-propagated from loop 1's last-iteration `$v1`, itself dead code the
 * scheduler chose to reuse) instead of the target's fresh `li $s2,0x1c`.
 * Not yet permuter-cracked (bounded run in progress / inconclusive at time
 * of writing) — both are scheduler/copy-propagation ties below the C level,
 * the same class as SelectCameraOwnerOption's parked residual.
 *
 * Matching notes:
 *  - Loops 1 and 2 (the D_8008E5BC pool) are HAND-ROLLED GOTO LOOPS, not
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
 *  - `D_8008E5BC` (26 Sprite3D* pointer slots) is walked as a plain `s32 *`
 *    (Ghidra's own cast: `*piVar3 = (int)pSVar2;`), not a typed
 *    `Sprite3D **` — matches the raw `(int)` store and the later
 *    `*(short *)(*piVar3 + 0x5A)` reload-through-the-slot (NOT through the
 *    just-stored `pSVar2` register): the target re-reads the slot from
 *    memory for the attribute store instead of reusing the SetupSprite
 *    result still live in a register, so the source must do the same
 *    (index the array again rather than keep a `sprite->attribute` access).
 *  - `fInitialize` is this TU's gp small; maspsxGpExterns is PER FILE (each
 *    split function is its own assembly unit), so this file needs its OWN
 *    Build.hs entry — DoInfoViewProc.c's entry only covers DoInfoViewProc.c.
 *  - `D_8008E5BC`'s auto-computed address DRIFTS depending on which nearby
 *    functions are still raw-asm vs compiled C (this function converting to
 *    C removed the last raw xref that had been anchoring it, and it
 *    resolved +0x28 off) — bound explicitly in config/symbols.main.exe.txt
 *    (plain name: ReqItemDrop/ReqItemMakibishi/
 *    ReqItemManebue already reference this exact symbol as
 *    `extern Sprite3D *D_8008E5BC[];` and need the SAME name fixed under
 *    them, unlike the 0x80097Dxx string-table drift where nothing else
 *    referenced the bad name).
 */
extern GsSPRITE CursorImage;
extern GsSPRITE NumberImage;
extern s32 D_8008E5BC[];
extern GsSPRITE ItemSprite3Ds[4];
extern u8 fInitialize;

extern GsIMAGE *GetImage(s32 id);
extern void InitSprite(GsIMAGE *im, GsSPRITE *sp);
extern Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image);
extern void leResetEnemyLayout(void);
extern void ResetInfoview(s32 stage);
extern void FUN_8004a6bc(void);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/InitializeInfoView", InitializeInfoView);
#else
void InitializeInfoView(void)
{
    GsIMAGE *pGVar1;
    Sprite3D *pSVar2;
    s32 *piVar3;
    GsSPRITE *sprite;
    int iVar4;
    s32 scale1;
    s32 scale2;
    s16 attr2;

    pGVar1 = GetImage(0x32);
    InitSprite(pGVar1, &CursorImage);
    CursorImage.attribute = 0x50000000;
    pGVar1 = GetImage(0x33);
    InitSprite(pGVar1, &NumberImage);
    iVar4 = 0;
    piVar3 = D_8008E5BC;
    scale1 = 0x3000;
loop1:
    pGVar1 = GetImage(iVar4 + 0x14);
    pSVar2 = SetupSprite((Sprite3D *)0, pGVar1);
    *piVar3 = (s32)pSVar2;
    pSVar2->scale = scale1;
    *(s16 *)(*piVar3 + 0x5A) = 0x1C;
    iVar4 = iVar4 + 1;
    piVar3 = piVar3 + 1;
    if (iVar4 < 0x14)
        goto loop1;
    if (iVar4 < 0x1A)
    {
        scale2 = 0x3000;
        attr2 = 0x1C;
        piVar3 = D_8008E5BC + iVar4;
    loop2:
        pGVar1 = GetImage(0xF);
        pSVar2 = SetupSprite((Sprite3D *)0, pGVar1);
        *piVar3 = (s32)pSVar2;
        pSVar2->scale = scale2;
        *(s16 *)(*piVar3 + 0x5A) = attr2;
        iVar4 = iVar4 + 1;
        piVar3 = piVar3 + 1;
        if (iVar4 < 0x1A)
            goto loop2;
    }
    iVar4 = 0;
    sprite = ItemSprite3Ds;
    do
    {
        pGVar1 = GetImage(iVar4 + 0x2E);
        InitSprite(pGVar1, sprite);
        sprite->attribute = 0x50000000;
        iVar4 = iVar4 + 1;
        sprite = sprite + 1;
    } while (iVar4 < 4);
    leResetEnemyLayout();
    ResetInfoview(-1);
    FUN_8004a6bc();
    fInitialize = 1;
}
#endif /* NON_MATCHING */
