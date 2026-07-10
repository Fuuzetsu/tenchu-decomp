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
 *     extern struct GsSPRITE NumberImage;
 *     extern struct GsSPRITE CursorImage;
 *     extern struct TCameraStatus CamState;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 8 of 504 bytes differ (2 whole instructions short;
 * whole-image count equals the window count once earlier functions in the
 * batch are fixed, so this is the function's OWN residual, not drift).
 *
 * PutItemList (0x8004ade4, 0x1F8 bytes) — draws the shop/inventory item
 * list: for each of the 25 item kinds (`CamState.Owner->item[i]`, the
 * per-kind carry count — item.h's Humanoid.item[0x1A]@0xB4, NOT PSX.SYM's
 * stale 0xA8), if the count is nonzero draws a right-to-left digit strip
 * (same NumberImage idiom as PutNumber.c/PutLifeBar.c, same TU) unless the
 * count is the 0xFF "unlimited" sentinel, then places the item icon sprite
 * (`D_8008E5BC[i]`, PutItemIcon.c's proven `ItemIconType`) — brighter/
 * highlighted with the moving cursor sprite if `i` is the currently
 * selected kind (`CURRENTLY_SELECTED_ITEM_KIND_1_`), dimmer otherwise.
 * `CURRENTLY_SELECTED_ITEM_KIND_0_` records which slot actually got drawn
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
 * BYTE offset into `D_8008E5BC[]` (+=4 per iteration, parallel to `i`'s
 * +=1) — a second explicit counter alongside `i`, not `D_8008E5BC[i]`
 * re-multiplied each time.
 *
 * Residual (root-caused): both branches of the `KIND_1_ == i` test compute
 * the identical `D_8008E5BC + s` address (confirmed byte-identical in the
 * TARGET too — it's NOT hoisted there, each branch pays for its own
 * lui+addiu). In THIS draft, reorg recognizes the shared `lui
 * %hi(D_8008E5BC)` as valid in the branch's own delay slot (executes
 * unconditionally either way) and reuses it for both arms — 2 real
 * instructions cheaper than the target, which instead fills that delay
 * slot with the final shared call's `move a2,zero` (a DIFFERENT, otherwise
 * redundant, dependency-free candidate). Tried and did not change the
 * winner: reordering the `ic=...`/cursor-field statements, a `do{}while(0)`
 * around the selected-branch tail, and hoisting an explicit early `pri`
 * local for the final call's 3rd argument. This is a delay-slot-filler
 * PRIORITY tie between two independent, dependency-free candidates — same
 * family as the cookbook's named guard-delay-slot-fill tie, just between
 * two ordinary candidates instead of a guard's own return-value setup.
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
extern ItemIconType *D_8008E5BC[];
extern s16 CURRENTLY_SELECTED_ITEM_KIND_0_;
extern s16 CURRENTLY_SELECTED_ITEM_KIND_1_;

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
    ItemIconType *ic;
    GsSPRITE *spr;

    CURRENTLY_SELECTED_ITEM_KIND_0_ = -1;
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
                NumberImage.w = 4;
                ou = NumberImage.u;
                NumberImage.x = x + 0x16;
                NumberImage.y = 0x64;

                t = n;
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

            if (CURRENTLY_SELECTED_ITEM_KIND_1_ == i)
            {
                CursorImage.y = 0x5C;
                CursorImage.scalex = 0x1000;
                CursorImage.scaley = 0x1000;
                CursorImage.rotate = CursorImage.rotate - 0x6000;
                CursorImage.x = x;
                GsSortSprite(&CursorImage, OTablePt, 1);

                ic = *(ItemIconType **)((u8 *)D_8008E5BC + s);
                CURRENTLY_SELECTED_ITEM_KIND_0_ = i;
                spr = &ic->sprite;
                spr->x = x;
                spr->y = 0x5C;
                spr->scalex = 0x1000;
                spr->scaley = 0x1000;
            }
            else
            {
                ic = *(ItemIconType **)((u8 *)D_8008E5BC + s);
                spr = &ic->sprite;
                spr->x = x;
                spr->y = 0x5C;
                spr->scalex = 0xAAA;
                spr->scaley = 0xAAA;
            }

            x = x - 0x14;
            GsSortSprite(spr, OTablePt, 0);
        }
        s = s + 4;
        i = i + 1;
    }
}
#endif
