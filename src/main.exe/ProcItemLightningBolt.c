#include "common.h"
#include "main.exe.h"

/*
 * ProcItemLightningBolt (0x800460d0) — the lightning bolt item processor.
 * mode 0: arm a 15-frame countdown, play the ready sound if the current
 * camera owner owns it; mode 1: retarget (SearchItemTarget2), snap the item's
 * locate to the result and register a 100-unit conflict box, advance to
 * mode 2; mode 2: every third GameClock tick, drop back to mode 1 to
 * retarget again. Every mode (and the fallthrough default) then always
 * redraws the lightning bolt effect, sprays a periodic bleed/impact, and
 * ticks the countdown, disposing when it reaches zero.
 *
 * Matching notes (see also ProcItemMakibishi.c for the item-TU/collision-box
 * conventions this shares):
 *  - `param = &item->param.lightningbolt;` precedes the entry test,
 *    putting the pointer calculation in its delay slot. Its `start`, `rot`,
 *    and `count` members are the original PSX.SYM names.
 *  - The dispatch is a real switch over {0, 1, 2} (plus the entry's separate
 *    0xff guard): expand_case builds a signed range-split tree (`slti v0,
 *    mode,2`) rather than Goshikimai's plain sequential beq/beqz, since
 *    there are 3+ cases here. The TEST order the tree picks (1, then a
 *    2-way split, then 0 within the low half, then 2 within the high half)
 *    does NOT match the case BODIES' memory order (0, then 1, then 2) —
 *    cases must still be declared ascending (0, 1, 2) in source; the tree
 *    shape is cc1's own doing, invisible from source order.
 *  - No default: falling out of all three cases (or matching none) reaches
 *    the shared "always redraw" tail directly, exactly like ProcItemMakibishi.
 *  - The camera-owner test uses the recovered shared `CamState.Owner` field.
 *  - The countdown test uses the OLD (pre-decrement) count, and the
 *    decrement is written `count + 0xff` (not `count - 1`): the literal
 *    0x00FF (not sign-extended -1) is the actual encoded immediate —
 *    verified against the raw instruction bytes, not just Ghidra's
 *    rendering, which normally simplifies genuine `-1`s.
 *  - `item->locate->locate.coord.t[N] = target.N` (SearchItemTarget2's output)
 *    goes through a fresh item->locate reload per component, matching the
 *    established no-cache idiom (ProcItemTeleport/Kusuri).
 *  - `n = InsertConflict(...)` must be `s32`, not `s16`: a same-statement
 *    sign-extension (right after the jal) vs a point-of-use one is a
 *    scheduling-tie lever (see ProcItemMakibishi's identical fix).
 */
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemLightningBolt(struct tag_TItem *item);
 *     ITEM.C:2856, 57 src lines, frame 56 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     param $s0       struct tag_TItem * item
 *     reg   $s2       struct param_lightningbolt * param
 *     stack sp+24     struct VECTOR target
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $s0       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern long GameClock;
 * END PSX.SYM */

extern Humanoid *SearchItemTarget2(Humanoid *owner, SVECTOR *rot,
                                   VECTOR *start, VECTOR *target);

void ProcItemLightningBolt(TItem *item)
{
    param_lightningbolt *param;
    VECTOR target;
    u8 cnt;
    s32 n;

    param = &item->param.lightningbolt;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = 0;
        return;
    }
    switch (item->mode)
    {
    case 0:
        param->count = 15;
        item->mode++;
        if (item->owner == CamState.Owner)
        {
            SoundEx((VECTOR *)0, 0x39);
        }
        break;

    case 1:
        SearchItemTarget2(item->owner, &item->param.lightningbolt.rot,
                          &param->start, &target);
        item->locate->locate.coord.t[0] = target.vx;
        item->locate->locate.coord.t[1] = target.vy;
        item->locate->locate.coord.t[2] = target.vz;
        DeleteConflict(item->locate);
        n = InsertConflict(item->locate);
        ConflictObject[n].offset.vx = 0;
        ConflictObject[n].offset.vz = 0;
        ConflictObject[n].offset.vy = 0;
        ConflictObject[n].size.vz = 100;
        ConflictObject[n].size.vy = 100;
        ConflictObject[n].size.vx = 100;
        ConflictObject[n].common = CONFLICT_OWNER_ITEM;
        ConflictObject[n].size.pad = CONFLICT_HIT;
        item->collision.size = 100;
        item->collision.ofsY = 0;
        item->collision.mode = CONFLICT_HIT;
        item->collision.pause = 0;
        item->mode++;
        break;

    case 2:
        if (GameClock == (GameClock / 3) * 3)
        {
            item->mode = 1;
        }
        break;
    }
    SetLightning(&param->start, (VECTOR *)item->locate->locate.coord.t,
                 100, 100, 200);
    if ((GameClock & 3) == 0)
    {
        SetBleeds((VECTOR *)item->locate->locate.coord.t, 200, 20, 10, 20, 0xFFFF78);
        SetImpact(&param->start, 0x4000, 1);
    }
    cnt = param->count;
    param->count = cnt + 0xff;
    if (cnt == 0 && item->proc != 0)
    {
        item->mode = ITEM_MODE_DISPOSE;
        item->proc(item);
        DeleteConflict(item->locate);
        if (item->mode != 0)
        {
            AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
        }
        item->owner = 0;
        item->proc = 0;
    }
}
