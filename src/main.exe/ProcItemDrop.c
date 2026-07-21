#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemDrop(struct tag_TItem *item);
 *     ITEM.C:835, 68 src lines, frame 40 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     param $s3       struct tag_TItem * item
 *     reg   $s4       struct param_drop * param
 *     reg   $a0       int cid
 *     reg   $t1       struct Sprite3D * model
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s3       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       struct ModelType * model
 *     reg   $s0       struct Humanoid * human
 *     reg   $s0       struct Humanoid * human
 *     reg   $s0       struct Humanoid * human
 *     reg   $s4       struct param_korogari * param
 *     reg   $s1       int x
 *     reg   $s0       int y
 *     reg   $v0       int z
 *     reg   $s3       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern short ActionHalt;
 * END PSX.SYM */

/*
 * ProcItemDrop (0x8003e454) — the tossed/dropped item processor installed by
 * ReqItemDrop. Every frame it copies the item coordinate into the sprite and
 * draws it; mode 0: bounce/roll physics (MoveKorogari) until the param status
 * says it came to rest (2/4: register a 0xB4 conflict box and become
 * pick-up-able) or fell out of the world (1: dispose); mode 1: wait for a
 * character to touch the conflict box, freeze it into the pick-up animation
 * (0x810) and remember it as owner; mode 2: when the animation ends, count
 * the item back into the owner's pouch (item[type]) and dispose.
 *
 * Matching notes (see docs/matching-cookbook.md; item-TU conventions as in
 * ProcItemKusuri/ProcItemManebue — no $gp here, ActionHalt is absolute):
 *  - `sprt`/`param` are assigned before the 0xff entry test (ReqItemDrop's
 *    double lever: the addiu fills the bne delay slot and the long live
 *    ranges demote both, so item/param land in $s3/$s4 after the shorter-lived
 *    case locals take $s0-$s2).
 *  - `sprt->locate = item->locate->locate` is the 0x50-byte GsCOORDINATE2
 *    struct assignment -> the 16-bytes-per-iteration word copy loop.
 *  - Both dispatches are real `switch`es (fresh index reload + signed slti).
 *    The constant 1 of the outer case tree is CSE'd along the taken path
 *    into the inner (status) tree's `case 1` compare: one pseudo, live
 *    across MoveKorogari, hence callee-saved $s0 set in DrawSprite's delay
 *    slot. Plain nested switches produce all of it — no source trick.
 *  - `m = 8` (int) feeding BOTH `size.pad` (sh) and `coll_mode` (sw) is
 *    load-bearing: written as literals, pad's 8 becomes an HImode pseudo and
 *    coll_mode's a second SImode one (two `li`s, function one insn too
 *    long). cse can only reuse a WIDER-mode constant reg that already
 *    exists, so the shared int variable is the original's shape.
 *  - Mode 2's tosses are two-statement temps: `x = rand(); x = x % 200;`.
 *    The in-place `mult $s1` + `subu $s1,$s1` prove raw value and remainder
 *    are the same variable; the -100/-200 offsets belong to the stores
 *    (`param->koro.vx = x - 100`), which is why they sit after the third rand.
 *    The 0x51EB851F magic is shared by %200/%100 via cse in $s2.
 *  - `cnt`/`ic` are u8 temps (Manebue's timer idiom): increment-then-store
 *    with the compare on the masked register (andi 0xFF), no reload; ic's
 *    `+ 1` lands in the beq delay slot.
 *  - The dispose tail is written out twice (status-1 + mode-2); cross-jump
 *    merges from the jalr on. Null-check via `ppu` but call through
 *    `item->proc(item)` (Kusuri's rule) so the pointer stays in $v0.
 */

extern void MoveKorogari(tag_TItem *item, param_korogari *pp);
extern s16 GetConflictResult(ModelType *m, s32 n);
extern s16 InsertConflict(ModelType *m);
extern s32 is_character_state_present_on_stage_(Humanoid *h);
/* The conflict pool (Ghidra: ConflictObject). */
extern ConflictObjectType ConflictObject[];

void ProcItemDrop(tag_TItem *item)
{
    Sprite3D *sprt;
    param_drop *param;
    void (*ppu)(tag_TItem *);
    Humanoid *human;
    MotionDataType *md;
    s32 i;
    s32 n;
    s32 m;
    s32 x;
    s32 y;
    s32 z;
    u8 cnt;
    u8 ic;

    sprt = item->model;
    param = (param_drop *)item->param;
    if (item->mode == 0xff)
    {
        item->mode = 0;
        return;
    }
    sprt->locate = item->locate->locate;
    DrawSprite(sprt);
    switch (item->mode)
    {
    case 0:
        MoveKorogari(item, &param->koro);
        switch (param->koro.status)
        {
        case 1:
            ppu = item->proc;
            if (ppu == 0)
                return;
            item->mode = 0xff;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != 0)
            {
                AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
            return;
        case 2:
        case 4:
            DeleteConflict(item->locate);
            n = InsertConflict(item->locate);
            m = 8;
            ConflictObject[n].offset.vx = 0;
            ConflictObject[n].offset.vz = 0;
            ConflictObject[n].offset.vy = 0;
            ConflictObject[n].size.vz = 0xb4;
            ConflictObject[n].size.vy = 0xb4;
            ConflictObject[n].size.vx = 0xb4;
            ConflictObject[n].common = (void *)0x1;
            ConflictObject[n].size.pad = m;
            item->coll_size = 0xb4;
            item->coll_ofsY = 0;
            item->coll_mode = m;
            item->coll_pause = 0;
            item->mode = item->mode + 1;
            return;
        }
        return;

    case 1:
        if ((item->locate->attribute & 0x8000) == 0)
            i = -1;
        else
            i = GetConflictResult(item->locate, -1);
        if (i == -1)
            return;
        human = (Humanoid *)ConflictObject[i].common;
        if (is_character_state_present_on_stage_(human) == 0)
            return;
        if (human->motion->mid == 0x810)
            return;
        if ((human->status != 6) && (human->status != 2))
            return;
        if (ActionHalt == 0 && 0 < human->life)
        {
            dispose_weapon_data_of_char_(human, 3);
            UpdateMotion(human->motion, 0x810);
            human->status = 8;
            md = human->motion->motion;
            MoveHumanoid(human, md->orderspd, md->sidespd);
        }
        item->owner = human;
        item->mode = item->mode + 1;
        param->count = 0;
        return;

    case 2:
        if (item->owner->motion->mid != 0x810)
        {
            x = rand();
            x = x % 200;
            y = rand();
            y = y % 100;
            z = rand();
            z = z % 200;
            param->koro.vx = x - 100;
            param->koro.vy = y - 200;
            param->koro.hint = 0;
            param->koro.status = 0;
            param->koro.vz = z - 100;
            item->mode = 0;
        }
        cnt = param->count + 1;
        param->count = cnt;
        if (cnt == 10)
        {
            SoundEx(item->owner->locate, 0xd);
            ic = item->owner->item[item->type];
            if (ic != 0xff)
            {
                item->owner->item[item->type] = ic + 1;
            }
            ppu = item->proc;
            if (ppu == 0)
                return;
            item->mode = 0xff;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != 0)
            {
                AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
        }
        return;
    }
}
