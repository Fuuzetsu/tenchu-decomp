#include "common.h"
#include "main.exe.h"

/*
 * ProcItemLaunch (0x80047048) — the launched/thrown item processor (grenade
 * family). Every frame: fly it (MoveFly), tick the arming countdown (at 0
 * register a 300-unit conflict box), spin it (rotate.vy = GameClock * 0x2AA),
 * mirror the coordinate into the shared model and draw + afterimage. If a
 * live character is in the conflict box: SetImpact + SoundEx and dispose.
 * Otherwise, once armed (param+0x28), dispatch on the payload type
 * (param+0xA): 1 = plain dispose; 3 = detonate (SetBleeds/SoundEx 0x31 +
 * reset_alert_duration); 2/4 = drop a pickup (build a PARAM_ITEM_USE at the
 * model's position, dispose, ReqItemDrop).
 *
 * Matching notes (this is ProcItemHappou's skeleton — see that file for the
 * conflict-box and countdown conventions; all deltas verified):
 *  - The scratch model is `item->model` here (Happou draws into its gp-global
 *    HAPPOU_SCRATCH_MODEL instead).
 *  - The spin block's three stores are inline (`item->locate->rotate.v* =`);
 *    each reloads item->locate (the in-struct stores invalidate the cached
 *    load). GameClock * 0x2AA is the plain literal multiply (shift/add
 *    chain), truncated by the sh.
 *  - `switch (pp[0xa])` with bodies in source order 3, 2/4, 1; case 1 is the
 *    shared `dispose:` label the impact path reaches by `goto` — its body
 *    (and the case-2/4 copy) are literal duplicates, NOT cross-jumped (they
 *    have different continuations).
 *  - The drop path builds `param` through a pointer (`p = &param;
 *    memset(p, ...)`) but stores fields DIRECT (`param.type = ...`, sp-folded),
 *    then copies with `rparam = *p;` — the *p spelling is what lets the
 *    0x28-byte block copy's source cursor coalesce with p ($s0) across the
 *    case-2/4 join label (a `rparam = param` spelling re-materializes the
 *    address). ReqItemDrop takes &rparam (the copy, not the original).
 *  - `rparam` is declared before `param` (slot order 0x18/0x40, matching
 *    PSX.SYM's rparam@24/param@72 declaration order).
 */
/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemLaunch(struct tag_TItem *item);
 *     ITEM.C:3089, 83 src lines, frame 128 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     param $s0       struct tag_TItem * item
 *     reg   $s2       struct ModelType * model
 *     reg   $s1       struct param_launch * param
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v0       int t
 *     stack sp+24     struct PARAM_ITEM_STAY rparam
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct PARAM_ITEM_STAY * p
 *     stack sp+72     struct PARAM_ITEM_LAUNCH param
 *     reg   $s0       struct tag_TItem * item
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s1       struct Humanoid * m
 *     reg   $s1       struct Humanoid * human
 *     reg   $s0       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern long GameClock;
 * END PSX.SYM */

#include "item.h"

/* Conflict slot (Ghidra: ConflictObjectType, 0x78 bytes; see ProcItemDrop.c). */
typedef struct
{
    ModelType *model;            /* 0x00 */
    VECTOR position;             /* 0x04 */
    SVECTOR offset;              /* 0x14 */
    SVECTOR size;                /* 0x1C */
    void *common;                /* 0x24 */
    u8 result[64];                /* 0x28 */
    u8 pad[0x10];                 /* 0x68 */
} ConflictObjectType;             /* 0x78 */

extern void MoveFly(tag_TItem *item, u8 *pp);
extern void DisposeAfterimage(s32 effect);
extern void DrawAfterimage(s32 effect, s32 flag);
extern void DrawModel(ModelType *m);
extern s16 InsertConflict(ModelType *m);
extern s16 GetConflictResult(ModelType *m, s32 n);
extern s32 is_character_state_present_on_stage_(Humanoid *h);
extern void SetImpact(VECTOR *pos, s32 a, s32 b);
extern void reset_alert_duration(void);
extern ConflictObjectType ConflictObject[];
extern s32 GameClock;

void ProcItemLaunch(tag_TItem *item)
{
    ModelType *model;
    u8 *pp;
    u8 cnt;
    s32 cid;
    s32 n;
    PARAM_ITEM_USE *p;
    PARAM_ITEM_USE rparam;
    PARAM_ITEM_USE param;

    model = item->model;
    pp = item->param;
    if (item->mode == 0xff)
    {
        DisposeAfterimage(*(s32 *)(pp + 0x2c));
        item->mode = 0;
        return;
    }
    MoveFly(item, pp);
    cnt = pp[0x30] - 1;
    pp[0x30] = cnt;
    if (cnt == 0)
    {
        DeleteConflict(item->locate);
        n = InsertConflict(item->locate);
        ConflictObject[n].offset.vx = 0;
        ConflictObject[n].offset.vz = 0;
        ConflictObject[n].offset.vy = 0;
        ConflictObject[n].size.vz = 300;
        ConflictObject[n].size.vy = 300;
        ConflictObject[n].size.vx = 300;
        ConflictObject[n].common = (void *)1;
        ConflictObject[n].size.pad = 1;
        item->coll_size = 300;
        item->coll_ofsY = 0;
        item->coll_mode = 1;
        item->coll_pause = 0;
    }
    item->locate->rotate.vx = 0;
    item->locate->rotate.vy = GameClock * 0x2aa;
    item->locate->rotate.vz = 0;
    UpdateCoordinate(item->locate);
    model->locate = item->locate->locate;
    DrawModel(model);
    DrawAfterimage(*(s32 *)(pp + 0x2c), 1);
    if ((item->locate->attribute & 0x8000) == 0)
        cid = -1;
    else
        cid = GetConflictResult(item->locate, -1);
    if (cid != -1 && is_character_state_present_on_stage_(ConflictObject[cid].common) != 0)
    {
        SetImpact((VECTOR *)item->locate->locate.coord.t, 0x4000, 2);
        SoundEx((VECTOR *)item->locate->locate.coord.t, 0x30);
        goto dispose;
    }
    if (pp[0x28] == 0)
        return;
    switch (pp[0xa])
    {
    case 3:
        SetBleeds((VECTOR *)item->locate->locate.coord.t, 0, 0x19, 0xa, 0xa, 0xffff00);
        SoundEx((VECTOR *)item->locate->locate.coord.t, 0x31);
        reset_alert_duration();
        return;

    case 2:
    case 4:
        p = &param;
        memset(p, 0, sizeof(PARAM_ITEM_USE));
        param.type = item->type;
        param.user = item->owner;
        param.start.vx = model->locate.coord.t[0];
        param.start.vy = model->locate.coord.t[1];
        param.start.vz = model->locate.coord.t[2];
        rparam = *p;
        if (item->proc != 0)
        {
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
        ReqItemDrop(&rparam);
        return;

    case 1:
    dispose:
        if (item->proc != 0)
        {
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
