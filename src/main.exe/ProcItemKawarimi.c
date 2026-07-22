#include "common.h"
#include "main.exe.h"

/*
 * ProcItemKawarimi (0x80040c0c) — the kawarimi (substitution/decoy) item
 * processor. mode 0: reset the frame counter; mode 1: each frame spray 20
 * random SetBleed particles (color 0x64C8DC) around the owner, and after 0x1F
 * frames advance to mode 2; mode 2: dispose of the item (call its proc with
 * mode=ITEM_MODE_DISPOSE, remove its collision, complain if the proc didn't
 * clear mode).
 *
 * Matching notes (all verified against the original bytes; this is
 * ProcItemKusuri's mode-2 bleed loop verbatim — see that file for the loop
 * conventions: while(1)+break keeps the top test while loop.c hoists &buf,
 * &buf[0x10] and the %1000 magic divisor; the %10 magic stays inline; the
 * jitter is written `t[n] + (rand() % 1000 - K)` for fold's reassociation):
 *  - `param = &item->param.drop;` is declared before the entry
 *    ITEM_MODE_DISPOSE test — reorg hoists the addiu into that branch's
 *    delay slot.
 *  - `ff` (u8, ITEM_MODE_DISPOSE) is caller-saved ($a1) here, unlike
 *    Kusuri's $s4: its
 *    only uses are the entry compare and case 2's `item->mode = ff`, and no
 *    call intervenes on that path.
 *  - The dispatch is a real `switch` (fresh lbu + signed slti tree), bodies
 *    in source order 0,1,2. Cases 0 and 1 both end in a literal duplicated
 *    `item->mode = item->mode + 1; return;` — jump2 cross-jumps them into
 *    the LAST copy (case 1's), leaving case 0 as `j` + the sb in its delay
 *    slot. Writing one shared after-switch `mode++` instead puts the tail
 *    after case 2 (wrong layout).
 *  - Case 1's counter is `u8 cnt = param->count + 1; param->count = cnt;
 *    if (cnt < 0x1f) return;` — the u8 local re-narrowed after arithmetic
 *    gives the defensive andi 0xff + sltiu.
 *  - Case 2's dispose checks `if (item->proc == 0)` INLINE (allocates $v0
 *    for both the test and the jalr; Kusuri's named `ppu` temp allocates
 *    $v1 there).
 */

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemKawarimi(struct tag_TItem *item);
 *     ITEM.C:1571, 35 src lines, frame 80 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
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
 *     reg   $s4       struct param_drop * param
 *     reg   $s2       int i
 *     stack sp+16     struct VECTOR pos
 *     stack sp+32     struct SVECTOR vec
 *     reg   $s0       struct tag_TItem * item
 * END PSX.SYM */

#include "item.h"

/* The particle loop reuses one 0x20-byte slot. PSX.SYM names its output
 * views `pos` and `vec`; the inner union records how the temporary VECTOR is
 * overwritten by the output velocity and its SVECTOR build area. */
typedef struct
{
    VECTOR pos;
    union
    {
        VECTOR position;
        struct
        {
            SVECTOR vec;
            SVECTOR velocity;
        } vectors;
    } work;
} ProcItemKawarimiScratch;

void ProcItemKawarimi(TItem *item)
{
    param_drop *param;
    u8 ff;
    s32 i;
    ProcItemKawarimiScratch scratch;

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
        param->count = 0;
        item->mode = item->mode + 1;
        return;

    case 1:
        i = 0;
        while (1)
        {
            if (!(i < 0x14))
                break;
            memset(&scratch.work.position, 0, sizeof(VECTOR));
            scratch.work.position.vx =
                item->owner->model->locate.coord.t[0] +
                (rand() % 1000 - 500);
            scratch.work.position.vy =
                item->owner->model->locate.coord.t[1] +
                (rand() % 1000 - 0x4b0);
            scratch.work.position.vz =
                item->owner->model->locate.coord.t[2] +
                (rand() % 1000 - 500);
            scratch.pos = scratch.work.position;
            memset(&scratch.work.vectors.velocity, 0, sizeof(SVECTOR));
            scratch.work.vectors.velocity.vy = rand() % 10 - 30;
            scratch.work.vectors.vec = scratch.work.vectors.velocity;
            SetBleed(&scratch.pos, &scratch.work.vectors.vec,
                     rand() % 0x10 + 0xf, 0x64C8DC);
            i++;
        }
        {
            u8 cnt;

            cnt = param->count + 1;
            param->count = cnt;
            if (cnt < 0x1f)
                return;
        }
        item->mode = item->mode + 1;
        return;

    case 2:
        if (item->proc == 0)
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
}
