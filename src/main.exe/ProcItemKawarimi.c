#include "common.h"
#include "tuning.h"
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
 *  - `ITEM_MODE_DISPOSE` (u8, ITEM_MODE_DISPOSE) is caller-saved ($a1) here, unlike
 *    Kusuri's $s4: its
 *    only uses are the entry compare and case 2's `item->mode = ITEM_MODE_DISPOSE`, and no
 *    call intervenes on that path.
 *  - The dispatch is a real `switch` (fresh lbu + signed slti tree), bodies
 *    in source order 0,1,2. Cases 0 and 1 both end in a literal duplicated
 *    `item->mode = item->mode + 1; return;` — jump2 cross-jumps them into
 *    the LAST copy (case 1's), leaving case 0 as `j` + the sb in its delay
 *    slot. Writing one shared after-switch `mode++` instead puts the tail
 *    after case 2 (wrong layout).
 *  - Case 1's counter is `u8 frame_count = param->count + 1;` followed by
 *    the store and threshold test — the u8 local re-narrowed after arithmetic
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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct tag_TItem * item
 *     reg   $s4       struct param_drop * param
 *     reg   $s2       int i
 *     stack sp+16     struct VECTOR pos
 *     stack sp+32     struct SVECTOR vec
 *     reg   $s0       struct tag_TItem * item
 * END PSX.SYM */

#include "item.h"

/* The particle loop reuses one 0x20-byte slot. PSX.SYM names its output
 * views `pos` and `vec`; the inner union records how the temporary position
 * is overwritten by the output velocity and its build area. */
typedef struct
{
    VECTOR position;
    union
    {
        VECTOR position_build;
        struct
        {
            SVECTOR velocity;
            SVECTOR velocity_build;
        } vectors;
    } work;
} ProcItemKawarimiScratch;

void ProcItemKawarimi(TItem *item)
{
    enum
    {
        KAWARIMI_MODE_START = 0,
        KAWARIMI_MODE_BLEED = 1,
        KAWARIMI_MODE_FINISH = 2,
        KAWARIMI_BLEED_FRAMES = 0x1f
    };
    param_drop *param;
    s32 particle_index;
    ProcItemKawarimiScratch scratch;

    param = &item->param.drop;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = KAWARIMI_MODE_START;
        return;
    }
    switch (item->mode)
    {
    case KAWARIMI_MODE_START:
        param->count = 0;
        item->mode++;
        return;

    case KAWARIMI_MODE_BLEED:
        particle_index = 0;
        while (1)
        {
            if (particle_index >= 0x14)
                break;
            memset(&scratch.work.position_build, 0, sizeof(VECTOR));
            scratch.work.position_build.vx =
                item->owner->model->locate.coord.t[0] +
                (rand() % 1000 - 500);
            scratch.work.position_build.vy =
                item->owner->model->locate.coord.t[1] +
                (rand() % 1000 - 1200);
            scratch.work.position_build.vz =
                item->owner->model->locate.coord.t[2] +
                (rand() % 1000 - 500);
            scratch.position = scratch.work.position_build;
            memset(&scratch.work.vectors.velocity_build, 0, sizeof(SVECTOR));
            scratch.work.vectors.velocity_build.vy = rand() % 10 - 30;
            scratch.work.vectors.velocity = scratch.work.vectors.velocity_build;
            SetBleed(&scratch.position, &scratch.work.vectors.velocity,
                     rand() % 16 + 15, RGB24(100, 200, 220));
            particle_index++;
        }
        {
            u8 frame_count;

            frame_count = param->count + 1;
            param->count = frame_count;
            if (frame_count < KAWARIMI_BLEED_FRAMES)
                return;
        }
        item->mode++;
        return;

    case KAWARIMI_MODE_FINISH:
        if (item->proc == 0)
            return;
        item->mode = ITEM_MODE_DISPOSE;
        item->proc(item);
        DeleteConflict(item->locate);
        if (item->mode != KAWARIMI_MODE_START)
        {
            AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
        }
        item->owner = 0;
        item->proc = 0;
        return;
    }
}
