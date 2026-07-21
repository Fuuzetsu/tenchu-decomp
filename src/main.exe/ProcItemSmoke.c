#include "common.h"
#include "main.exe.h"

/*
 * ProcItemSmoke (0x8003ff68) — the smoke bomb item processor. Every frame:
 * roll it (MoveKorogari), mirror its coordinate into the sprite and draw;
 * mode 0: countdown, then pop (SoundEx 0x23) and re-arm 0x78 frames of smoke;
 * mode 1: emit a SetSmoke puff every other frame; every 16th GameClock tick
 * scan HumanGroup for live humans within 2000 units of the bomb and knock
 * them out (motion 0x100B, status 0x10, Sound 6); dispose at 0.
 *
 * Matching notes (all verified against the original bytes; the deepest RTL
 * dive of the item TU — several pass-level levers):
 *  - `cnt` is `int` with `(u8)cnt` casts at the two zero-tests: a `u8 cnt`
 *    local makes the QI variable and the SI decrement temp separate pseudos
 *    that fail to coalesce here (an extra `move`); `int` + explicit re-narrow
 *    keeps ONE pseudo and still emits the `andi 0xff`s.
 *  - The SetSmoke block's scratch stores are spelled `((VECTOR *)(buf +
 *    0x18))->vx = ...` (COMPONENT refs): an in-struct store invalidates cse's
 *    cached `item->locate` load (MEM_IN_STRUCT alias heuristic), reproducing
 *    the per-line reloads; the `((s32 *)(buf + 0x18))[n]` spelling is a
 *    non-struct fixed-address store that cse ignores — locate stays cached
 *    (wrong). Inverse lever in the search setup: `pos = (VECTOR *)
 *    item->locate->locate.coord.t;` reads all three t[] through one pointer
 *    so nothing reloads there.
 *  - Search setup order is `q = buf; pos = ...; q->i = 0; find = buf;` —
 *    q's use must precede find's init or the two same-valued pointers
 *    collapse into one register (cse folds find's addiu into `move find,q`
 *    only, keeping both, when something touches q in between).
 *  - `found = 0` lives INSIDE the exhaust break-arm (`if (i < Humans) {...
 *    continue;} found = 0; break;`): written at the loop top it is
 *    loop-invariant, loop.c hoists it into the outer loop's head, and the
 *    whole allocation shifts one callee-saved register up (s6 appears).
 *  - The hit handler sits at a `goto hit` label AFTER the tail, with
 *    `found = target;` first and the three find-> stores wrapped in
 *    do{}while(0): (a) creation order past the `check:` test keeps target as
 *    cse's canonical head (make_regs_eqv head-preference is by live-range
 *    end), so `find->find = target` keeps $s0; (b) the dummy loop note
 *    blocks local-alloc's optimize_reg_copy_1 from propagating the copy into
 *    the store (its scan stops at NOTE_INSN_LOOP_BEG); sched1 otherwise
 *    hoists the copy (REG_DEAD rank boost) and lreg rewrites the store.
 *  - `i = 0x10;` is pre-assigned BEFORE the ActionHalt guard (the cookbook
 *    pre-assign lever), reusing the dead loop counter: its li lands in the
 *    guard's delay slot and rides $s1 across the two calls to the sh.
 *  - `extern SVECTOR D_80097AD8[]` (unknown size!) + `D_80097AD8[0]`: an
 *    8-byte scalar extern is small-data-eligible (-G8), SYMBOL_REF_FLAG makes
 *    its address cost 1 and cse's find_best_addr folds the block-move source
 *    address back into the pattern (one-register `la`); the unknown-size
 *    array keeps cost 2 > reg, the high/lo_sum pair survives to the
 *    post-reload block-move split, and the hi half lands in $v0.
 */
/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemSmoke(struct tag_TItem *item);
 *     ITEM.C:1400, 59 src lines, frame 112 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     reg   $s1       struct Sprite3D * model
 *     reg   $s0       struct param_smoke * param
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s3       struct tag_TItem * item
 *     stack sp+16     struct SVECTOR vec
 *     stack sp+24     struct VECTOR pos
 *     stack sp+56     struct TFindItemTarget find
 *     reg   $v1       struct VECTOR * pos
 *     reg   $s1       int i
 *     reg   $s0       struct Humanoid * target
 *     reg   $v0       int dist
 *     reg   $s0       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct Humanoid *HumanGroup[32];
 *     extern short Humans;
 *     extern short ActionHalt;
 * END PSX.SYM */

#include "item.h"

extern s32 GameClock;
extern s16 Humans;
extern Humanoid *HumanGroup[];
extern SVECTOR D_80097AD8[];

extern void MoveKorogari(tag_TItem *item, param_korogari *pp);
extern void Sound(Humanoid *h, int id);
extern int GetVectorDistance(VECTOR *v1, VECTOR *v2);

void ProcItemSmoke(tag_TItem *item)
{
    Sprite3D *model;
    param_smoke *param;
    u8 ff;
    int cnt;
    u8 buf[0x28];

    model = item->model;
    param = (param_smoke *)item->param;
    ff = 0xff;
    if (item->mode == ff)
    {
        item->mode = 0;
        return;
    }
    MoveKorogari(item, &param->koro);
    if (param->koro.status == 1)
    {
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
    UpdateCoordinate(item->locate);
    model->locate = item->locate->locate;
    DrawSprite(model);
    cnt = param->count - 1;
    param->count = cnt;
    switch (item->mode)
    {
    case 0:
        if ((u8)cnt != 0)
            return;
        SoundEx((VECTOR *)item->locate->locate.coord.t, 0x23);
        param->count = 0x78;
        item->mode = item->mode + 1;
        return;

    case 1:
        if ((u8)cnt == 0)
        {
            if (item->proc == 0)
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
        }
        if ((cnt & 1) == 0)
        {
            *(SVECTOR *)buf = D_80097AD8[0];
            memset(buf + 0x18, 0, sizeof(VECTOR));
            ((VECTOR *)(buf + 0x18))->vx = item->locate->locate.coord.t[0];
            ((VECTOR *)(buf + 0x18))->vy = item->locate->locate.coord.t[1];
            ((VECTOR *)(buf + 0x18))->vz = item->locate->locate.coord.t[2];
            *(VECTOR *)(buf + 8) = *(VECTOR *)(buf + 0x18);
            SetSmoke((VECTOR *)(buf + 8), (SVECTOR *)buf, 1, 3);
        }
        if ((GameClock & 0xf) != 0)
            return;
        {
            TFindItemTarget *q;
            TFindItemTarget *find;
            VECTOR *pos;
            int i;
            Humanoid *target;
            Humanoid *found;
            Humanoid *human;
            int dist;
            MotionDataType *md;

            q = (TFindItemTarget *)buf;
            pos = (VECTOR *)item->locate->locate.coord.t;
            q->i = 0;
            find = (TFindItemTarget *)buf;
            find->pos.vx = pos->vx;
            find->pos.vy = pos->vy;
            find->pos.vz = pos->vz;
            find->find_dist = 2000;
            while (1)
            {
                i = find->i;
                while (1)
                {
                    if (i < Humans)
                    {
                        target = HumanGroup[i];
                        if (0 < target->life && target->motion->mid != 0x100 && (target->attribute & 0x80) == 0)
                        {
                            dist = GetVectorDistance(&find->pos, target->locate);
                            if (dist < find->find_dist)
                                goto hit;
                        }
                        i = i + 1;
                        continue;
                    }
                    found = 0;
                    break;
                }
            check:
                if (found == 0)
                    return;
                human = ((TFindItemTarget *)buf)->find;
                if (human != item->owner && human->life != -1 && human->motion->mid != 0x100b)
                {
                    i = 0x10;
                    if (ActionHalt == 0 && 0 < human->life)
                    {
                        dispose_weapon_data_of_char_(human, 3);
                        UpdateMotion(human->motion, 0x100b);
                        human->status = i;
                        md = human->motion->motion;
                        MoveHumanoid(human, md->orderspd, md->sidespd);
                    }
                    Sound(((TFindItemTarget *)buf)->find, 6);
                }
                continue;
            hit:
                found = target;
                do
                {
                    find->find = target;
                    find->dist = dist;
                    find->i = i + 1;
                } while (0);
                goto check;
            }
        }
    }
}
