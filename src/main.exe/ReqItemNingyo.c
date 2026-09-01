#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemNingyo(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2018, 24 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_ningyo * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *     reg   $s1       struct param_korogari * param
 *     reg   $v0       int x
 *     reg   $v1       int y
 *     reg   $a0       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

/*
 * ReqItemNingyo (0x800429f0) — spawn a "ningyo" (doll/puppet decoy?) item.
 * Twin of ReqItemDrop/ReqItemJirai/ReqItemDokudango/ReqItemKaengeki/
 * ReqItemMakibishi/ReqItemLightningBolt (same item TU, same pool round-robin
 * on ic and the same dispose-on-exhaustion block); like
 * ReqItemJirai/ReqItemDokudango there is no GetAreaMapLevel floor check. It
 * gets ProcItemNingyo as its processor, packs the end vector into the
 * embedded param_ningyo.koro record, initializes count and hp, randomizes
 * the model's rotation (rotate.vx=0, rotate.vy = rand()%0x1000, rotate.vz =
 * rand()%0x44), then calls SetNowMotion on the user.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - Unlike ReqItemMakibishi/ReqItemLightningBolt, `item` is NOT split into a
 *    separate loop-cursor pseudo here — it stays in one register ($s0) for
 *    the whole function (lower register pressure at the loop/found-label
 *    join than those two, despite this function's bigger body overall).
 *  - `param = &item->param.ningyo;` sits BEFORE the null check, same
 *    lever as the other twins (addiu fills the beqz delay slot).
 *  - `pos = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as the other twins.
 *  - aowner/atype and x/y/z (end vector, into the nested
 *    `param_korogari *param`) are real temps, same shape as
 *    ReqItemDrop/ReqItemJirai/ReqItemDokudango. The block-scoped shadow is
 *    the second `param` recorded by PSX.SYM.
 *  - `item->param.ningyo.koro.hint = 0;` uses the direct union path (not
 *    `param`) for this one store, same as the other twins.
 *  - `rotate.vx = 0;` is scheduled into the FIRST `rand()` call's delay slot
 *    (it's independent of the call and textually precedes it — ordinary
 *    scheduler delay-slot filling, no special source shape needed).
 *  - `rotate.vy = rand() % ANGLE_FULL;` — power-of-2 modulo, automatic from a
 *    plain `%` (fully compiler-generated sign/shift dance, same idiom family
 *    as the cookbook's magic-multiply rule but the power-of-2 special case).
 *    Its STORE is scheduled into the SECOND `rand()` call's delay slot
 *    (independent, textually precedes it — same mechanism as rotate.vx).
 *  - `rotate.vz = rand() % 68;` — non-power-of-2 modulo, the
 *    canonical magic-multiply (0x78787879, shift 5, sign correction).
 *  - SetNowMotion is called with the SAME 3 args as ReqItemDokudango
 *    (`item->owner.human, 0xf02, 1`) — m2c reports a spurious 4th argument in $a3,
 *    but that register is simply the UNCORRECTED mfhi intermediate from the
 *    `% 0x44` magic-multiply (mfhi->sra 5->subu sign is the real quotient,
 *    kept in a DIFFERENT register and consumed by the `q*68` subtraction;
 *    $a3 itself is never touched again and happens to still hold that value
 *    at the call) — not a real argument. Same m2c-over-counts-args situation
 *    as ReqItemMakibishi's SoundEx call; only 3 args reproduce the bytes.
 *  - PSX.SYM's param_ningyo names the trailing bytes count and hp; using
 *    those fields preserves the original `sb` stores at offsets 0xC/0xD.
 */
extern void ProcItemNingyo(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemNingyo(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    param_ningyo *param;
    s32 x;
    s32 y;
    s32 z;
    s32 i;

    TAKE_ITEM_SLOT();
    param = &item->param.ningyo;
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        aowner = p->user.human;
        atype = p->type;
        item->owner.human = aowner;
        item->proc = ProcItemNingyo;
        item->mode = ITEM_MODE_START;
        item->type = atype;
        item->locate->locate.coord.t[0] = p->start.vx;
        pos = &p->start;
        item->locate->locate.coord.t[1] = pos->vy;
        item->locate->locate.coord.t[2] = pos->vz;
        item->locate->locate.super = 0;
        UpdateCoordinate(item->locate);
        item->collision.size = 0;
        item->model.sprite = ItemImage[item->type];
    }
    {
        param_korogari *param; /* shadows the outer `param`, as PSX.SYM has it;
                                * byte-required -- writing through the full
                                * member path re-colors the stores */

        param = &item->param.ningyo.koro;
        x = p->end.vx;
        y = p->end.vy;
        z = p->end.vz;
        setVector(param, x, y, z);
        item->param.ningyo.koro.hint = 0;
        param->status = KORO_NORMAL;
    }
    param->count = NINGYO_DURATION;
    item->locate->rotate.vx = 0;
    item->locate->rotate.vy = rand() % ANGLE_FULL;
    item->locate->rotate.vz = rand() % 68;
    param->hp = NINGYO_HP;
    SetNowMotion(item->owner.human, MOT_ITEM_THROW, 1);
    return 1;
}
