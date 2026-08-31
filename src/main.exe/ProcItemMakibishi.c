#include "common.h"
#include "main.exe.h"

/*
 * ProcItemMakibishi (0x8003f304) — the makibishi (caltrop) item processor.
 * mode 0: roll it (MoveKorogari) — KORO_WATER disposes after entering water,
 * while KORO_STAY registers a 100-unit conflict box and advances to mode 1;
 * mode 1: once a live character is found in the conflict box, spray a
 * SetBleeds burst + step-on sound and dispose. Every mode (and the
 * mode==neither-0-nor-1 default) falls through to a shared tail that redraws
 * the sprite from the current locate every frame.
 *
 * Matching notes (see also ProcItemDrop.c for the item-TU/ConflictObject
 * conventions this shares):
 *  - `model = (Sprite3D *)item->model; param = &item->param.drop;` both sit
 *    before the entry mode==ITEM_MODE_DISPOSE test (ProcItemDrop's double
 *    lever): model's
 *    load is sequential, param's addiu fills the entry branch's delay slot.
 *  - The mode dispatch is a real switch (fresh reload distinct from the
 *    entry ff-check's load, matching the switch rule); with no case for
 *    "neither 0 nor 1", falling out of the switch reaches the shared draw
 *    tail directly — the SAME tail case 0/case 1 reach via `break`.
 *  - `one = 1;` is a real shared variable, not a literal: the
 *    status==KORO_WATER compare needs `1` in a register anyway (MIPS beq has
 *    no immediate
 *    form), and the SAME register then feeds `item->mode + one`, `.common =
 *    (void *)one`, `.size.pad = one`, and `item->collision.mode = one` — five
 *    uses, unmistakably `addu` (register) not `addiu` (immediate) at the
 *    mode-increment, so it must survive as a live variable across the
 *    DeleteConflict/InsertConflict calls (callee-saved, like `ff`/`m`).
 *  - The dispose after status==KORO_WATER and the dispose after mode 1's
 *    conflict-hit are the SAME code written out TWICE (cross-jump merges
 *    from the jalr on): the KORO_WATER path reuses `ff` (still live,
 *    untouched since entry); the mode-1 path materializes a fresh
 *    ITEM_MODE_DISPOSE value since nothing carries `ff` that far — same
 *    asymmetry as ProcItemKusuri's mode-2 vs mode-1 dispose.
 *  - Collision box field-store order (offset.vx/vz/vy, then size.vz/vy/vx,
 *    then common, then size.pad) exactly mirrors ProcItemDrop's
 *    KORO_GRAND/KORO_STAY case, just different numbers (100 not 0xb4,
 *    one(1) not m(8)).
 */
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemMakibishi(struct tag_TItem *item);
 *     ITEM.C:1171, 60 src lines, frame 48 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct tag_TItem * item
 *     reg   $s4       struct Sprite3D * model
 *     reg   $s2       struct param_drop * param
 *     reg   $a0       int cid
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $s0       struct tag_TItem * item
 *     reg   $a0       struct ModelType * model
 *     reg   $s1       struct Humanoid * human
 *     reg   $s1       struct Humanoid * human
 *     reg   $s0       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */

extern void MoveKorogari(TItem *item, param_korogari *pp);
extern s32 is_humanoid_on_stage_(Humanoid *h);

void ProcItemMakibishi(TItem *item)
{
    Sprite3D *model;
    param_drop *param;
    void (*ppu)(TItem *);
    u8 ff;
    u8 st;
    s32 one;
    s32 i;
    s32 n;

    model = (Sprite3D *)item->model;
    param = &item->param.drop;
    ff = ITEM_MODE_DISPOSE;
    if (item->mode == ff)
    {
        item->mode = 0;
        return;
    }
    switch (item->mode)
    {
    case 0:
        MoveKorogari(item, &param->koro);
        one = 1;
        st = param->koro.status;
        switch (st)
        {
        case KORO_STAY:
            item->mode = item->mode + one;
            DeleteConflict(item->locate);
            n = InsertConflict(item->locate);
            SET_ITEM_COLLISION(n, 100, (void *)one, one);
            break;

        case KORO_WATER:
            ppu = item->proc;
            if (ppu == 0)
                return;
            item->mode = ff;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != 0)
            {
                AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
            return;
        }
        break;

    case 1:
        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
            i = -1;
        else
            i = GetConflictResult(item->locate, -1);
        if (i != -1 && is_humanoid_on_stage_(ConflictObject[i].common) != 0)
        {
            SetBleeds((VECTOR *)item->locate->locate.coord.t, 0, 20, 10, 15, 0x7f0000);
            SoundEx((VECTOR *)item->locate->locate.coord.t, 0x30);
            ppu = item->proc;
            if (ppu == 0)
                return;
            DISPOSE_ITEM(item);
            return;
        }
        break;
    }
    UpdateCoordinate(item->locate);
    model->locate = item->locate->locate;
    DrawSprite(model);
}
