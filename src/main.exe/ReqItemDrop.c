#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemDrop(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:907, 15 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_drop * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *     reg   $s1       struct param_korogari * param
 *     reg   $v1       int x
 *     reg   $a0       int y
 *     reg   $a1       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

/*
 * ReqItemDrop (0x8003e8ac) — spawn a dropped/tossed item (called e.g. by
 * ProcItemKusuri when the drink is interrupted). Round-robins through the
 * items[] pool (ITEM.C's gp-relative `ic` counter);
 * if all 0x1d slots are busy it force-disposes the current one (the same
 * dispose block as the ProcItem* functions). The drop lands only if the area
 * map has floor at the start position; the item then gets ProcItemDrop as its
 * processor and the toss velocity packed into param (param_korogari view).
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `param = &item->param.drop;` sits BEFORE the null check: its addiu
 *    fills the beqz delay slot, and the longer live range demotes param's
 *    allocation priority so p keeps $s1 (after the check, param stole $s1 and the
 *    return-0 block folded away — 2 instructions short, which shifted every
 *    object after this one by 8 bytes).
 *  - The post-guard item setup and its velocity tail have their own register-only
 *    scopes. `aowner`/`atype` and `pos` remain real batching identities: direct
 *    owner/type stores differ by 10 canonical lines and direct position reads by 5.
 *  - x/y/z are real temps: the target batches `lw` x3 before `sh` x3. Direct
 *    stores differ by 12 lines as a group (x/y/z alone: 2/12/12); the complete
 *    owner/type/position/velocity graph differs by 27.
 *  - `pos = &p->start;` is materialized between the t[0] and t[1] stores (the
 *    vy/vz reads go through it; vx reads p directly).
 *  - First compiled-to-compiled reference: ProcItemKusuri's `jal ReqItemDrop`
 *    now resolves against this object (R_MIPS_26), and both share item.h.
 */
extern void ProcItemDrop(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemDrop(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    param_drop *param;
    s32 i;

    TAKE_ITEM_SLOT();
    param = &item->param.drop;
    if (item == 0)
        return 0;
    if (GetAreaMapLevel(GlobalAreaMap, p->start.vx, p->start.vy, p->start.vz,
                        AREA_LEVEL_DEFAULT) < p->start.vy)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        aowner = p->user.human;
        atype = p->type;
        item->owner.human = aowner;
        item->proc = ProcItemDrop;
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
        {
            s32 x;
            s32 y;
            s32 z;

            x = p->end.vx;
            y = p->end.vy;
            z = p->end.vz;
            param->koro.vx = x;
            param->koro.vy = y;
            param->koro.vz = z;
            item->param.drop.koro.hint = 0;
            param->koro.status = KORO_NORMAL;
        }
    }
    return 1;
}
