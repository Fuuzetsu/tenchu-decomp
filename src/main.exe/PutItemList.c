#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void PutItemList(void);
 *     INFOVIEW.C:366, 35 src lines, frame 56 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
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
 *     reg   $s3       int i
 *     reg   $s4       int x
 *     reg   $s0       unsigned int s
 *     reg   $v1       int n
 *     reg   $s2       int ou
 *     reg   $s3       int ItemID
 *     reg   $a0       struct GsSPRITE * spr
 *     reg   $s3       int ItemID
 *     reg   $a0       struct GsSPRITE * spr
 *
 * Globals it touches, as the original declared them:
 *     extern short SelectedItem;
 *     extern struct GsSPRITE NumberImage;
 *     extern struct GsSPRITE CursorImage;
 *     extern struct TCameraStatus CamState;
 *     extern struct GsOT *OTablePt;
 *     extern short ItemCursor;
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — exact 504-byte/126-instruction extent; 51 bytes
 * differ (13 aligned assembly lines in 5 blocks, and the whole-image count
 * equals the function-window count, so there is no upstream drift).
 *
 * PutItemList (0x8004ade4, 0x1F8 bytes) — draws the shop/inventory item
 * list: for each of the 25 item kinds (`CamState.Owner->item[i]`, the
 * per-kind carry count — item.h's Humanoid.item[0x1A]@0xB4, NOT PSX.SYM's
 * stale 0xA8), if the count is nonzero draws a right-to-left digit strip
 * (same NumberImage idiom as PutNumber.c/PutLifeBar.c, same TU) unless the
 * count is the 0xFF "unlimited" sentinel, then places the item icon sprite
 * (`ItemImage[i]`, PutItemIcon.c's proven `ItemIconType`) — brighter/
 * highlighted with the moving cursor sprite if `i` is the currently
 * selected kind (`ItemCursor`), dimmer otherwise.
 * `SelectedItem` records which slot actually got drawn
 * selected this frame (reset to -1 up front).
 *
 * The digit loop is the same hand-rolled goto as PutNumber/PutLifeBar (the
 * /10 magic constant re-materializes every iteration in the target rather
 * than hoisting — a real do/while would let loop.c hoist it to a preheader).
 * The outer item loop IS a real loop (top-test + unconditional back-jump,
 * the `while(1){if(!cond)break;...}` shape): `&NumberImage`/`&CursorImage`
 * hoist to the very top of the function (computed once, outside the loop)
 * exactly as loop.c's invariant motion would do for a real loop, matching
 * the target's prologue setting up $s1/$s6 before the loop even starts.
 *
 * `s` (PSX.SYM: a genuine separate `unsigned int`, not just `i`) is the
 * BYTE offset into `ItemImage[]` (+=4 per iteration, parallel to `i`'s
 * +=1) — a second explicit counter alongside `i`, not `ItemImage[i]`
 * re-multiplied each time.
 *
 * The old two-instruction structural residual is solved. PSX.SYM's repeated
 * block-local `ItemID`/`spr` identities are material: spelling each arm with
 * its own locals and its own final GsSortSprite call preserves branch-local
 * ItemImage/OTablePt/a2 setup. jump2 then cross-merges the two calls into the
 * target's one physical jal, while the genuinely shared `x -= 20` remains
 * after the if and fills that jal's delay slot. This is a reusable inverse
 * of the usual source-funnelling instinct: duplicate a mergeable call in C
 * when the target shares the call but not its argument producers.
 *
 * The two one-statement do/while(0) wrappers are byte-neutral allocation and
 * scheduling levers. They put the NumberImage base, `ou`, `i`, and `x` in
 * the target's $s1/$s2/$s3/$s4 homes (greg priorities 4666/4444/4105/4062).
 * Moving CursorImage.x first likewise gives the target's cursor-store order.
 *
 * Remaining residual (root-caused):
 *  - The prologue has the same saves, constants, addresses, and register
 *    homes, but sched places the $s5 save/`s=i` copy before SelectedItem and
 *    places CursorImage's address before the $fp/$s7 constants. The target
 *    schedules those operations later (10 aligned lines total).
 *  - In the digit setup, the target keeps `x+22` live in $v0, materializes
 *    y=100 in $a3, then stores x/y. This draft stores x first and reuses $v0
 *    for 100 (3 aligned lines). Flat statement orders, comma expressions,
 *    direct while/for/do loop spellings, counter-update placement, a
 *    symbol-backed single-`n` digit loop, and a bounded 40-candidate guided
 *    autorules pass did not improve this checkpoint.
 *
 * Tool gap: rtlguide correctly classifies the residue as CSE/coalescing plus
 * scheduling, but neither it nor autorules proposes the decisive combination
 * of repeated debug-local scopes, duplicated branch calls, and a shared
 * post-branch delay-slot producer; nor can it target this constant-vs-store
 * scheduler tie.
 */
typedef struct
{
    u8 pad[0x10];
    Humanoid *Owner;
} TCameraStatus;

typedef struct
{
    Sprite3D model;
    GsSPRITE sprite;
} ItemIconType;

extern GsSPRITE NumberImage;
extern GsSPRITE CursorImage;
extern TCameraStatus CamState;
extern GsOT *OTablePt;
extern ItemIconType *ItemImage[];
extern s16 SelectedItem;
extern s16 ItemCursor;

extern void GsSortSprite(GsSPRITE *sp, GsOT *ot, s32 pri);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/PutItemList", PutItemList);
#else
void PutItemList(void)
{
    s32 i;
    s32 x;
    u32 s;
    s32 n;
    s32 ou;
    s32 t;
    s32 q;

    SelectedItem = -1;
    x = 0x8C;
    i = 0;
    s = i;
    while (1)
    {
        if (!(i < 0x19))
            break;

        n = CamState.Owner->item[i];
        if (n != 0)
        {
            if (n != 0xFF)
            {
                t = n;
                do
                {
                    NumberImage.w = 4;
                } while (0);
                NumberImage.x = x + 0x16;
                ou = NumberImage.u;
                do
                {
                    NumberImage.y = 0x64;
                } while (0);

            numloop:
                q = t / 10;
                NumberImage.u = ou + (t - q * 10) * 4;
                GsSortSprite(&NumberImage, OTablePt, 0);
                NumberImage.x = NumberImage.x - 6;
                t = q;
                if (t != 0)
                    goto numloop;
                NumberImage.u = ou;
            }

            if (ItemCursor == i)
            {
                s32 ItemID;
                GsSPRITE *spr;

                CursorImage.x = x;
                CursorImage.y = 0x5C;
                CursorImage.scalex = 0x1000;
                CursorImage.scaley = 0x1000;
                CursorImage.rotate = CursorImage.rotate - 0x6000;
                GsSortSprite(&CursorImage, OTablePt, 1);

                ItemID = *(s32 *)((u8 *)ItemImage + s);
                SelectedItem = i;
                spr = (GsSPRITE *)(ItemID + 0x68);
                spr->x = x;
                spr->y = 0x5C;
                spr->scalex = 0x1000;
                spr->scaley = 0x1000;
                GsSortSprite(spr, OTablePt, 0);
            }
            else
            {
                s32 ItemID;
                GsSPRITE *spr;

                ItemID = *(s32 *)((u8 *)ItemImage + s);
                spr = (GsSPRITE *)(ItemID + 0x68);
                spr->x = x;
                spr->y = 0x5C;
                spr->scalex = 0xAAA;
                spr->scaley = 0xAAA;
                GsSortSprite(spr, OTablePt, 0);
            }
            x = x - 0x14;
        }
        s = s + 4;
        i = i + 1;
    }
}
#endif
