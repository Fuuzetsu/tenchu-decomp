#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemShinsoku(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:1383, 13 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s2       struct param_shinsoku * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

/*
 * ReqItemShinsoku (0x8003fde0) — spawn a "shinsoku" (instant-movement /
 * fire?) item. Twin of ReqItemDrop/ReqItemJirai/ReqItemSmoke/ReqItemFire/
 * ReqItemDokudango (same item TU, same pool round-robin on
 * ic and the same dispose-on-exhaustion block); like
 * ReqItemJirai/ReqItemSmoke/ReqItemFire/ReqItemDokudango there is no
 * GetAreaMapLevel floor check. It gets ProcItemShinsoku as its processor, but
 * differs from every other twin in three ways (all confirmed against the
 * .s, not just Ghidra):
 *  - `item->model` is unconditionally zeroed — no ItemImage[item->type] lookup.
 *  - the end vector is packed into PSX.SYM's `param_shinsoku.vec`, at
 *    offsets 0/2/4. No hint/status/count writes occur in this function.
 *  - both the "pool exhausted" early return and the normal path return 0
 *    (confirmed: both epilogue predecessors set $v0 via `addu $v0,$zero,$zero`)
 *    — unlike the other twins, which return 1 on the normal path.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `param = &item->param.shinsoku;` sits BEFORE the null check, same
 *    lever as the other twins (addiu fills the beqz delay slot).
 *  - `pos = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as the other twins.
 *  - aowner/atype are real temps, same shape as the other twins.
 *  - the end-vector stores are NOT batched through temps here (unlike the
 *    other twins' x/y/z): the asm interleaves each `lhu` with its `sh`
 *    immediately, so they're written inline (`*(s16 *)(...) = p->end.vx;`)
 *    per the cookbook's "narrowing store fed through a temp forces the
 *    full-word load" rule — no temp here means the truncating lhu, matching
 *    the target exactly.
 *  - Only `vec` is initialized here; `count` is left for the processor.
 */
extern void ProcItemShinsoku(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemShinsoku(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    param_shinsoku *param;
    s32 i;

    TAKE_ITEM_SLOT();
    param = &item->param.shinsoku;
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        aowner = p->user;
        atype = p->type;
        item->owner = aowner;
        item->proc = ProcItemShinsoku;
        item->mode = 0;
        item->type = atype;
        item->locate->locate.coord.t[0] = p->start.vx;
        pos = &p->start;
        item->locate->locate.coord.t[1] = pos->vy;
        item->locate->locate.coord.t[2] = pos->vz;
        item->locate->locate.super = 0;
        UpdateCoordinate(item->locate);
        item->collision.size = 0;
        item->model = 0;
    }
    param->vec.vx = p->end.vx;
    param->vec.vy = p->end.vy;
    param->vec.vz = p->end.vz;
    return 0;
}
