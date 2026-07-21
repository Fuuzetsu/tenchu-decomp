#include "common.h"
#include "main.exe.h"

/*
 * FUN_800568b8 (0x800568b8, 0x58 bytes) — clamps every item's stock in a
 * PersistentState blob down to that item's shop maximum, skipping the ones
 * marked locked (0xFE). Same TU as FUN_800565f0.c/FUN_800566c0.c, and the same
 * proven structs: game_types.h's TLinkInfo (chr@0x4, stock@0x40C) and
 * ShopItemDefault (itemIndex@0x4, maxStock@0x8, stride 0xC).
 *
 * Unlike its siblings this one takes the blob as a PARAMETER rather than going
 * through the `(TLinkInfo *)0x80010000` cast — the target keeps it in $a0
 * and does the `ps + (itemIndex + chr*0x20)` pointer arithmetic off it, so
 * there is no `lui` for the blob at all. It has no `jal` callers (reached
 * through a proc pointer, or a root).
 *
 * Three source-shape choices are load-bearing here:
 *  - Index the table as `SHOP_ITEM_DEFAULTS[i].f`, not via a walking `e++`
 *    pointer: the pointer form makes cc1 strength-reduce the induction variable
 *    to point at the LAST field it touches (`maxStock`, +8), so the accesses
 *    come out as `-4(a2)`/`0(a2)` off a base biased by 8.
 *  - `mx` must be an `int` temp, or the `u8 < u8` compare narrows to `sltu`.
 *  - Leave `ps->stock[n]` INLINE in both operands of the `&&` (cc1 CSEs the
 *    address) and declare `mx` before it. Hoisting the load into a `cur` temp
 *    instead swaps $v1/$a1 between the address and `mx` — a 5-byte register tie
 *    that autorules/regalloc could not name and only the permuter cracked.
 */

extern ShopItemDefault SHOP_ITEM_DEFAULTS[];

void FUN_800568b8(TLinkInfo *ps)
{
    int i;

    for (i = 0; i < 0x13; i++) {
        int n = SHOP_ITEM_DEFAULTS[i].itemIndex + ps->CharType * 0x20;
        int mx = SHOP_ITEM_DEFAULTS[i].maxStock;

        if (ps->stock[n] != 0xFE && mx < ps->stock[n]) {
            ps->stock[n] = mx;
        }
    }
}
