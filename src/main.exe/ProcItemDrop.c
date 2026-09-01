#include "common.h"
#include "sound.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemDrop(struct tag_TItem *item);
 *     ITEM.C:835, 68 src lines, frame 40 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * reports ground contact/settling (KORO_GRAND/KORO_STAY: register a 0xB4
 * conflict box and become pick-up-able) or water (KORO_WATER: dispose);
 * mode 1: wait for a
 * character to touch the conflict box, freeze it into the pick-up animation
 * (0x810) and remember it as owner; mode 2: when the animation ends, count
 * the item back into the owner's pouch (item[type]) and dispose.
 *
 * Matching notes (see docs/matching-cookbook.md; item-TU conventions as in
 * ProcItemKusuri/ProcItemManebue — no $gp here, ActionHalt is absolute):
 *  - `model`/`param` are assigned before the ITEM_MODE_DISPOSE entry test
 *    (ReqItemDrop's double lever): the addiu fills the bne delay slot and the
 *    long live ranges demote both, so item/param land in $s3/$s4 after the
 *    shorter-lived case locals take $s0-$s2).
 *  - `model->locate = item->locate->locate` is the 0x50-byte GsCOORDINATE2
 *    struct assignment -> the 16-bytes-per-iteration word copy loop.
 *  - Both dispatches are real `switch`es (fresh index reload + signed slti).
 *    The constant 1 of the outer case tree is CSE'd along the taken path
 *    into the inner (status) tree's `case KORO_WATER` compare: one pseudo, live
 *    across MoveKorogari, hence callee-saved $s0 set in DrawSprite's delay
 *    slot. Plain nested switches produce all of it — no source trick.
 *  - `collision_mode = CONFLICT_SOFT` feeding BOTH `size.pad` (sh) and `collision.mode` (sw) is
 *    load-bearing: written as literals, pad's 8 becomes an HImode pseudo and
 *    a separate collision.mode literal becomes a second SImode pseudo (two
 *    `li`s, function one insn too long). cse can only reuse a WIDER-mode
 *    constant reg that already
 *    exists, so the shared int variable is the original's shape.
 *  - Mode 2's tosses are two-statement temps: `x = rand(); x = x % 200;`.
 *    The in-place `mult $s1` + `subu $s1,$s1` prove raw value and remainder
 *    are the same variable; the -100/-200 offsets belong to the stores
 *    (`param->koro.vx = x - 100`), which is why they sit after the third rand.
 *    The 0x51EB851F magic is shared by %200/%100 via cse in $s2.
 *  - `cnt`/`count` are u8 temps (Manebue's timer idiom): increment-then-store
 *    with the compare on the masked register (andi 0xFF), no reload; count's
 *    `+ 1` lands in the beq delay slot.
 *  - The dispose tail is written out twice (KORO_WATER + mode-2); cross-jump
 *    merges from the jalr on. Null-check via `ppu` but call through
 *    `item->proc(item)` (Kusuri's rule) so the pointer stays in $v0.
 */

extern void MoveKorogari(TItem *item, param_korogari *pp);
extern s32 is_humanoid_on_stage_(Humanoid *h);
/* The conflict pool (Ghidra: ConflictObject). */

void ProcItemDrop(TItem *item)
{
    enum
    {
        DROP_MODE_ROLL = 0,
        DROP_MODE_WAIT = 1,
        DROP_MODE_TRANSFER = 2
    };
    Sprite3D *model;
    param_drop *param;
    void (*ppu)(TItem *);
    Humanoid *human;
    MotionDataType *md;
    s32 i;
    s32 conflict_id;
    ConflictClass collision_mode;
    s32 x;
    s32 y;
    s32 z;
    u8 cnt;
    u8 count;

    model = (Sprite3D *)item->model;
    param = &item->param.drop;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = DROP_MODE_ROLL;
        return;
    }
    model->locate = item->locate->locate;
    DrawSprite(model);
    switch (item->mode)
    {
    case DROP_MODE_ROLL:
        MoveKorogari(item, &param->koro);
        switch (param->koro.status)
        {
        case KORO_WATER:
            ppu = item->proc;
            if (ppu == 0)
                return;
            DISPOSE_ITEM(item);
            return;
        case KORO_GRAND:
        case KORO_STAY:
            DeleteConflict(item->locate);
            conflict_id = InsertConflict(item->locate);
            collision_mode = CONFLICT_SOFT;
            SET_ITEM_COLLISION(conflict_id, 180, CONFLICT_OWNER_ITEM,
                               collision_mode);
            item->mode++;
            return;
        }
        return;

    case DROP_MODE_WAIT:
        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
            i = CONFLICT_NONE;
        else
            i = GetConflictResult(item->locate, CONFLICT_NONE);
        if (i == CONFLICT_NONE)
            return;
        human = (Humanoid *)ConflictObject[i].common;
        if (is_humanoid_on_stage_(human) == 0)
            return;
        if (human->motion->mid == MOT_STATE_PICKUP)
            return;
        if ((human->status != STAT_CHASE) && (human->status != STAT_MOVE))
            return;
        if (ActionHalt == 0 && human->life > 0)
        {
            dispose_weapon_data_of_char_(human, ATTACK_CANCEL_ALL);
            UpdateMotion(human->motion, MOT_STATE_PICKUP);
            human->status = STAT_STATE;
            md = human->motion->motion;
            MoveHumanoid(human, md->orderspd, md->sidespd);
        }
        item->owner = human;
        item->mode++;
        param->count = 0;
        return;

    case DROP_MODE_TRANSFER:
        if (item->owner->motion->mid != MOT_STATE_PICKUP)
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
            param->koro.status = KORO_NORMAL;
            param->koro.vz = z - 100;
            item->mode = DROP_MODE_ROLL;
        }
        cnt = param->count + 1;
        param->count = cnt;
        if (cnt == 10)
        {
            SoundEx(item->owner->locate, SE_ITEM_TRANSFER);
            count = item->owner->item[item->type];
            if (count != ITEM_INFINITE)
            {
                item->owner->item[item->type] = count + 1;
            }
            ppu = item->proc;
            if (ppu == 0)
                return;
            DISPOSE_ITEM(item);
        }
        return;
    }
}
