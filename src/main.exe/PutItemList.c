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
 * STATUS: NON_MATCHING — exact 504-byte/126-instruction extent; 27 bytes
 * differ (6 aligned assembly lines in 2 blocks, and the whole-image count
 * equals the function-window count, so there is no upstream drift). Down
 * from a 51-byte park (this round: 51->34->33->27, all via a bounded
 * FOREGROUND permuter plus direct verification — see below).
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
 * This round's three permuter-found, individually-verified wins (each ported
 * by hand from the retained candidate and re-measured with matchdiff, never
 * trusted from the proxy score alone):
 *  - `scale` (51->34): the ARM-LOCAL `CursorImage.scalex = 0x1000;` was one of
 *    FOUR uses of the literal 0x1000 in the `ItemCursor==i` arm. Naming just
 *    THIS one site's constant into a local assigned immediately before
 *    `CursorImage.x = x;` (the other three sites stay literal) retunes
 *    allocation enough to fix the $s1/$s6/$s7 preheader hoist order.
 *  - `numY` (34->33): same lever on `NumberImage.y = 0x64;`, but this local's
 *    win is POSITION-DEPENDENT in a way `scale`'s is not — it only pays if
 *    declared+assigned at the very top of the function (right after
 *    `SelectedItem = -1;`); the same substitution assigned at its point of
 *    use, or inside its own do-while fence, measured back to 34 (tested both;
 *    see git history). Declaration-order alone (swapping locals to match
 *    PSX.SYM's reverse-printed order for i/x/s/n/ou) is provably NEUTRAL here
 *    (measured, unlike SetWire's s0/s1 flip) — reverted, not worth the
 *    unmotivated deviation from natural order.
 *  - The identical-arm fence (33->27): `NumberImage.w = 4;` duplicated under
 *    `if (n) {...} else {...}` (condition on `n`, already live from the
 *    enclosing `n != 0`/`n != 0xFF` tests) fixed the $v0/$a3 digit-setup tie
 *    AND, as a side effect, let the `NumberImage.y` do-while fence be deleted
 *    outright (bare statement, byte-neutral once the arm-fence exists) — this
 *    is the book's `identical-arm-condition` lever (cookbook §3.10), and the
 *    enclosing `do{}while(0)` around the if/else itself is NOT load-bearing
 *    (autorules' fence-unwrap confirmed 27->27; removed for a cleaner source
 *    — the if/else alone supplies the block boundary, per the cookbook's
 *    "buys... a real block boundary" mechanism, the do-while adds nothing
 *    once that boundary exists).
 *
 * Remaining residual (root-caused, RTL-read, permuter-searched — 2 clusters,
 * 8 insns, 27 bytes, both MULTISET-EQUAL reorders of already-correct
 * instructions/registers, not missing/wrong content):
 *  - The prologue has the same saves, constants, addresses, and register
 *    HOMES (confirmed via regalloc.py: p84->s5 and p143->fp both match target
 *    exactly), but the relative POSITION of two preheader movable-groups is
 *    swapped: target orders [SelectedItem, &NumberImage, 92(->$fp/s8),
 *    4096(->$s7), &CursorImage, s=i(->$s5)] — i.e. the `s=i` copy is the
 *    LAST thing before the loop test, immediately preceding it, matching the
 *    cookbook's "a giv init is the ONLY thing that can follow a hoist"
 *    shape (§3.14) even though `s` is an explicit source variable, not an
 *    implicit array-index giv. This draft instead schedules `s=i` FIRST
 *    (before even `SelectedItem=-1`) and the 92-constant LAST.
 *  - `tools/cc1says.py`, `tools/schedtrace.py --pass sched2`, and
 *    `tools/sched-deps.py --pass sched2` (self-validated, forward-
 *    reconstructed) all confirm this is decided by sched2's backward list
 *    scheduler on priority-1 "FLOOR" instructions (all these preheader sets
 *    are independent, unit-latency, and tie on priority), falling through to
 *    its LUID/hazard-swap tiebreak — a genuine post-reload scheduling
 *    decision, not a cse/combine/loop content difference.
 *  - Directly tested and REGRESSED or NEUTRAL (all reverted, preserved in
 *    git history): moving `s = i;` to the very top of the function (51);
 *    wrapping just `s = i;` in its own do-while fence (63); swapping `i`/`x`
 *    init order (51); swapping the `NumberImage.x`-store/`NumberImage.y`-fence
 *    source order (50); `s = i;` vs `s = 0;` in the SAME position (neutral,
 *    34->34, confirmed a real edit via nullcheck); an identical-arm fence
 *    around `s = i;` keyed on the always-true `i == 0` (neutral, real edit,
 *    not kept — no live/meaningful condition exists this early, unlike `n`'s
 *    fence); an early-materialized `cursorY` local for the 92 constant
 *    (LENGTH REGRESSION — 492 vs 504 bytes, a whole instruction CSE'd away);
 *    full PSX.SYM-reversed declaration order for i/x/s/n/ou (neutral).
 *  - FOUR bounded foreground permuter rounds this session (~89000 total
 *    iterations: 21762/23022/21520/22851), two of which found the wins
 *    above and two of which (rounds on the 33- and 27-byte checkpoints)
 *    confirmed the running checkpoint as their OWN best candidate — i.e. the
 *    search itself now prefers this state to everything it tried, including
 *    a chained `i = (s = 0);` variant (scored 30, worse). autorules (4 runs
 *    across this round's checkpoints) found nothing beyond the wins above.
 *  - This is evidence-complete for this decomposition, not proof of
 *    unreachability: a joint (not single-factor) source restructuring might
 *    still crack it, but the cheap ladder (reghist -> autorules -> bounded
 *    permuter -> RTL) is exhausted for this round.
 *
 * RE-VERIFICATION (independent round, current toolchain, permute.py post
 * -fno-builtin fix — all the above reconfirmed, nothing beats 27):
 *  - Fresh bounded permuter (22584 iters, --stop-on-zero -j4): authoritative
 *    full-link rescore keeps base.c at 27; every candidate scored >=30. The
 *    -fno-builtin CC_FLAGS change is inert here (this function has no builtin
 *    calls; /10 lowers to mult regardless), so the prior plateau stands.
 *  - regalloc.py --order: EVERY pseudo home already matches the target
 *    (p80->s3 i, p86->s2 ou, p84->s5 s, p96->s1 &Num, p142->s6 &Cur,
 *    p146->fp 92, p83->s7 4096, p81->s4 x). The residual is therefore PURE
 *    sched2 emission order of correctly-allocated registers, not a colour tie
 *    the "seed a redundant copy" lever could touch.
 *  - loop.c dump: `s` is verified biv 84 (init insn 23, value 0), "Cannot
 *    eliminate biv 84: biv used in insn 240" — NOT strength-reduced, so its
 *    init IS a source-position insn (low LUID 23) and sched2's backward
 *    LUID-DESC tiebreak lands it early. The target's late `move s5,s3` is the
 *    same biv init; only sched2's pick order differs.
 *  - Human-structure levers tried and REJECTED (each regressed, all reverted):
 *    `ItemImage[i]` array-index in place of the explicit `s` byte offset ->
 *    484 bytes (loop.c combines base+offset into a hoisted POINTER giv,
 *    deleting the per-arm `&ItemImage` rematerialization the target keeps);
 *    dropping `t` to loop on `n` directly (demo lists no `t`) -> 500 bytes
 *    (the target HAS the separate `v1=s0` loop-var copy `t=n` fills into the
 *    0xFF-test delay slot, so `t` is structurally required, cf. PutNumber's
 *    `cols`); dropping the identical-arm -> i/ou swap to s2/s3 (the fence is
 *    register STEERING for i->s3, per the demo, not just a schedule nudge);
 *    dropping `numY` -> 40 bytes. No clean-at-slightly-worse alternative
 *    exists (the owner's human-structure preference is satisfied by the demo-
 *    faithful core: explicit `s`, per-arm spr/call merged by jump2, goto loop,
 *    `t` copy — the three props are the irreducible schedule/alloc levers).
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
    s32 numY;
    s32 scale;
    u32 s;
    s32 n;
    s32 ou;
    s32 t;
    s32 q;

    SelectedItem = -1;
    numY = 0x64;
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
                if (n)
                {
                    NumberImage.w = 4;
                }
                else
                {
                    NumberImage.w = 4;
                }
                NumberImage.x = x + 0x16;
                ou = NumberImage.u;
                NumberImage.y = numY;

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

                scale = 0x1000;
                CursorImage.x = x;
                CursorImage.y = 0x5C;
                CursorImage.scalex = scale;
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
