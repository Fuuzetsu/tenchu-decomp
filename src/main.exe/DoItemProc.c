#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DoItemProc(void);
 *     ITEM.C:3205, 52 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

/*
 * DoItemProc (0x8004a12c, 0xac bytes) — the item pool's per-frame driver
 * (main's game loop / CVArun): lazily runs InitializeItem() once (guarded
 * by ITEM.C's original `fInitial`, qualified here as `Item_fInitial` because
 * the split decomp also exposes MISC.C's same-named static), then
 * every 10th GameClock tick runs UpdateItemState(), then dispatches every
 * live slot's own `proc` callback.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `GameClock % 10 == 0` written as `GameClock == (GameClock/10)*10`
 *    (Ghidra's own rendering) reproduces the magic-multiply (0x66666667)
 *    div-by-10 sequence automatically — not hand-derived.
 *  - Top-test (`slti`+`beqz`) + unconditional back-jump (no jump-to-bottom
 *    on entry) is the `while (1) { if (!(cond)) break; ...; i++; }` shape,
 *    NOT the plain-`for` bottom-test shape PackItemLayout got from the
 *    identical i=0/i<0x1e bounds — the two provably-30-iteration loops in
 *    this same TU compile differently, so the SOURCE shape (for vs
 *    while(1)+break), not the bound, decides it.
 *  - `it->proc(it)` is a plain indirect call through the struct's own
 *    `proc` field (no null-check/cast dance) — the field's own type
 *    (`void (*proc)(TItem *)` in item.h) already supplies the call
 *    signature.
 */

extern void InitializeItem(void);
extern void UpdateItemState(void);

void DoItemProc(void)
{
    TItem *it;
    s32 i;

    if (Item_fInitial == 0)
    {
        InitializeItem();
    }
    if (GameClock == (GameClock / 10) * 10)
    {
        UpdateItemState();
    }
    i = 0;
    it = items;
    while (1)
    {
        if (!(i < 0x1e))
            break;
        if (it->proc != 0)
        {
            it->proc(it);
        }
        it++;
        i++;
    }
}
