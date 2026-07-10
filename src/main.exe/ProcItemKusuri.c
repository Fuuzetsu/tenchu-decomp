#include "common.h"
#include "main.exe.h"

/*
 * ProcItemKusuri (0x80040500) — the kusuri (healing potion) item processor.
 * mode 0: freeze the drinker (dispose weapon, drink animation 0xF01, status 0xF),
 * attach the item model to the character's hand and nudge it into place; mode 1:
 * while the drink animation plays, track the hand (copy the item coordinate into
 * the sprite and draw it); at animation frame 0x37 switch to mode 2; if the
 * animation was interrupted, drop the item (ReqItemDrop with a random toss) and
 * dispose; mode 2: heal to full, spray 20 random SetBleed particles, play the
 * gulp sound, and dispose of the item.
 *
 * Matching notes (each verified against the original bytes; see also
 * ProcItemManebue.c for the item-TU conventions):
 *  - `ff` holds 0xFF in a callee-saved reg ($s4) across calls: used by the
 *    entry test and the drop path's `item->mode = ff`; mode 2's dispose writes
 *    a literal 0xff instead ($s4 is &buf by then).
 *  - The dispatch is a real `switch`: it reloads item->mode (fresh index load)
 *    and compares it SIGNED (slti) — an if-ladder CSEs the load and compares
 *    unsigned. Case bodies sit in source order (0, 1, 2).
 *  - `i = 0` is case 2's first statement (reorg hoists it into the case-2
 *    branch delay slot). The bleed loop is `while (1) { if (!(i < 0x14))
 *    break; ...; i++; }` — a for/while-with-condition gets its exit test
 *    duplicated at the entry (jump.c duplicate_loop_exit_test) and then
 *    constant-folded away; the while(1)+break form keeps the original's
 *    top-test + unconditional back-jump while still letting loop.c hoist the
 *    invariants (&buf, &buf[0x10], the two magic divisors).
 *  - mode 2's jitter is written `t[n] + (rand() % 1000 - 500)`: fold's
 *    associate step canonicalizes it to the original's (t[n]-500) + rem shape,
 *    whereas writing `t[n] - 500 + rand() % 1000` gets reassociated the wrong
 *    way (constant pulled onto the remainder).
 *  - The position stores go through the same base as the VECTOR copy
 *    (((s32 *)(buf + 0x10))[n]) so the scheduler sees the store/copy
 *    dependence; a separate `buf + 0x18` base lets it interleave the copy
 *    into the divide latency.
 *  - The dispose tail is written out twice (drop path + mode 2); GCC's
 *    cross-jump merges the common suffix from the jalr on. The null check
 *    reads `ppu = item->proc` but the call is `item->proc(item)` (cse reuses
 *    the load) — checking and calling through `ppu` allocates $v1 instead of
 *    the original's $v0.
 */
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemKusuri(struct tag_TItem *item);
 *     ITEM.C:1485, 60 src lines, frame 80 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     param $s3       struct tag_TItem * item
 *     reg   $s0       struct Sprite3D * model
 *     reg   $s1       int i
 *     reg   $s0       struct Humanoid * human
 *     reg   $s2       struct Humanoid * human
 *     reg   $s0       int itemID
 *     stack sp+16     struct PARAM_ITEM_LAUNCH p
 *     reg   $s3       struct tag_TItem * item
 *     stack sp+16     struct VECTOR pos
 *     stack sp+32     struct SVECTOR vec
 *     reg   $s3       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern short ActionHalt;
 * END PSX.SYM */

void ProcItemKusuri(tag_TItem *item)
{
    Sprite3D *sprt;
    void (*ppu)(tag_TItem *);
    u8 ff;
    s32 i;
    /* One shared 40-byte scratch buffer (Ghidra's local_40): the drop path views
     * it as a PARAM_ITEM_USE; mode 2 views buf as the SetBleed position VECTOR,
     * buf+0x10 as its build area / the velocity SVECTOR, and buf+0x18 as the
     * velocity build area. The casts (not typed locals) are what reproduce the
     * original's stack layout and copy alignments (VECTOR copies are word moves,
     * SVECTOR copies are lwl/lwr). The repeated casts are load-bearing too: a
     * pointer temp (PARAM_ITEM_USE *prm = ...; / VECTOR *w = ...;) does NOT
     * fold away — it allocates a register and shifts the frame (verified). */
    u8 buf[0x28];

    sprt = item->model;
    ff = 0xff;
    if (item->mode == ff)
    {
        item->mode = 0;
        return;
    }
    switch (item->mode)
    {
    case 0:
    {
        Humanoid *own;

        own = item->owner;
        if (ActionHalt == 0 && 0 < own->life)
        {
            MotionDataType *md;

            dispose_weapon_data_of_char_(own, 3);
            UpdateMotion(own->motion, 0xf01);
            own->status = 0xf;
            md = own->motion->motion;
            MoveHumanoid(own, md->orderspd, md->sidespd);
        }
    }
        {
            ModelArchiveType *arc;

            arc = item->owner->model;
            if (0xe < arc->n)
                item->locate->locate.super = &arc->object[0xe]->locate;
            else
                item->locate->locate.super = &arc->object[2]->locate;
        }
        item->locate->locate.coord.t[0] = 0;
        item->locate->locate.coord.t[1] = 0x32;
        item->locate->locate.coord.t[2] = 0;
        item->mode = item->mode + 1;
        return;

    case 1:
    {
        MotionManager *mot;

        mot = item->owner->motion;
        if (mot->mid != 0xf01)
        {
            /* animation interrupted: toss the item back out */
            VECTOR *pos;
            Humanoid *own;
            s32 ty;

            pos = GetAbsolutePosition(item->locate, 0, 0, 0);
            own = item->owner;
            ty = item->type;
            memset(buf, 0, sizeof(PARAM_ITEM_USE));
            ((PARAM_ITEM_USE *)buf)->type = ty;
            ((PARAM_ITEM_USE *)buf)->user = own;
            ((PARAM_ITEM_USE *)buf)->start.vx = pos->vx;
            ((PARAM_ITEM_USE *)buf)->start.vy = pos->vy;
            ((PARAM_ITEM_USE *)buf)->start.vz = pos->vz;
            ((PARAM_ITEM_USE *)buf)->end.vx = rand() % 200 - 100;
            ((PARAM_ITEM_USE *)buf)->end.vy = rand() % 100 - 200;
            ((PARAM_ITEM_USE *)buf)->end.vz = rand() % 200 - 100;
            ReqItemDrop((PARAM_ITEM_USE *)buf);
            ppu = item->proc;
            if (ppu == 0)
                return;
            item->mode = ff;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != 0)
            {
                AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
            return;
        }
        {
            s16 cnt;

            cnt = mot->count;
            if (cnt == 0x37)
            {
                item->mode = 2;
                return;
            }
            if (cnt < 4)
                return;
        }
    }
        UpdateCoordinate(item->locate);
        sprt->locate = item->locate->locate;
        sprt->scale = 0x2000;
        DrawSprite(sprt);
        return;

    case 2:
    {
        i = 0;
        item->owner->life = item->owner->lifemax;
        while (1)
        {
            if (!(i < 0x14))
                break;
            memset(buf + 0x10, 0, sizeof(VECTOR));
            ((s32 *)(buf + 0x10))[0] = item->owner->model->locate.coord.t[0] + (rand() % 1000 - 500);
            ((s32 *)(buf + 0x10))[1] = item->owner->model->locate.coord.t[1] + (rand() % 1000 - 0x4b0);
            ((s32 *)(buf + 0x10))[2] = item->owner->model->locate.coord.t[2] + (rand() % 1000 - 500);
            *(VECTOR *)buf = *(VECTOR *)(buf + 0x10);
            memset(buf + 0x18, 0, sizeof(SVECTOR));
            ((SVECTOR *)(buf + 0x18))->vy = rand() % 10 - 30;
            *(SVECTOR *)(buf + 0x10) = *(SVECTOR *)(buf + 0x18);
            SetBleed((VECTOR *)buf, (SVECTOR *)(buf + 0x10), rand() % 0x10 + 0xf, 0xffff7e);
            i++;
        }
        SoundEx(item->owner->locate, 0x24);
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
    }
}
