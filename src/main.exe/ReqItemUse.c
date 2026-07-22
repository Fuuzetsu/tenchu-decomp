#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemUse(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:3631, 386 src lines, frame 440 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     param $s1       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s1       int i
 *     stack sp+16     struct PARAM_ITEM_DROP param
 *     stack sp+56     struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+76     int ry
 *     stack sp+72     int rx
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s2       struct param_launch * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a0       int i
 *     reg   $v1       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *     stack sp+96     struct PARAM_ITEM_LAUNCH param
 *     stack sp+72     struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     stack sp+144    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     stack sp+160    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     stack sp+176    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     stack sp+192    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     stack sp+208    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     stack sp+224    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     stack sp+240    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     stack sp+256    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     stack sp+272    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     stack sp+288    struct PARAM_ITEM_DROP param
 *     stack sp+328    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a0       int i
 *     reg   $v1       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *     stack sp+344    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a0       int i
 *     reg   $v1       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *     stack sp+368    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     stack sp+384    struct VECTOR v
 *     reg   $a0       struct ModelArchiveType * model
 *     reg   $a3       int rz
 *     stack sp+140    int ry
 *     stack sp+136    int rx
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a0       int i
 *     reg   $v1       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *     stack sp+400    struct VECTOR target
 *     stack sp+416    int rx
 *     stack sp+420    int ry
 *     stack sp+136    struct SVECTOR rot
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s2       struct param_napalm * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a0       int i
 *     reg   $v1       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct tag_TItem items[30];
 *     extern struct ModelType *SyurikenModel;
 *     extern long GameClock;
 *     extern struct Sprite3D *sprNapalm;
 * END PSX.SYM */

/*
 * ReqItemUse (0x80048afc) — the item-launch dispatcher: decrements the
 * user's carry count for the item type, then switch(p->type) into 25 case
 * bodies (throw-vector setup + per-type ReqItemXxx call, or an inline
 * item-pool claim for kaginawa/sightshot/teleport/napalm).
 *
 * STATUS: MATCHING — all 5272 bytes match.
 *
 * The final residual was a pure register permutation in the 19-word
 * lightningbolt end-vector tail. RTL dumps showed that its block-local
 * sz pseudo was claimed by local-alloc before the other values reached
 * global allocation. Reusing three function-scope SImode locals later in
 * the kaginawa case gives global allocation byte-neutral anchors: sz holds
 * the items base ($a1), y the ProcKaginawa address ($v0), and z the scaled
 * item index ($v1). Reusing y/z for the final lightningbolt sums then
 * produces the retail register coloring and instruction order.
 *
 * Facts proven while matching (all byte-verified):
 *  - PARAM_ITEM_LAUNCH == item.h's PARAM_ITEM_LAUNCH layout {TItemType type;
 *    Humanoid *user; VECTOR start; VECTOR end;} (psxsym size 40 agrees).
 *  - TWO function-scope 0x28 workspaces: `param`@sp+16, `work`@sp+56.
 *    Sibling case scopes do NOT share stack slots under this cc1 (first
 *    draft with per-case aggregates laddered the frame to 416 bytes), so
 *    the original declared shared function-scope workspaces; the per-case
 *    address-taken rx/ry pairs DO ladder (96..204), giving frame 224.
 *    The kusuri-family "VECTOR v" lives at param's HEAD (*(VECTOR *)&param);
 *    makibishi/jirai stage a PARAM_ITEM_LAUNCH in `work` (memset + 3 field
 *    copies + `param = work`), then reuse work's head as the throw vector.
 *  - Retail case map from the jump table (demo enum differs: 8/9 swapped,
 *    0x16=NAPALM, 0x17=LIGHTNINGBOLT, 0x18=TELEPORT).
 *  - The camera-check template is ReqItemDefault.c's idiom verbatim; the
 *    `st = (VECTOR *)&param;` pointer temp before the guard puts &param in
 *    a callee-saved reg across GetVectorRotation (guard delay slot addiu).
 *  - The p->user == CamState.Owner guard in case 1 reads the OFFSET-0 ALIAS
 *    CURRENTLY_SELECTED_CHARACTER_STATE_PTR and must be spelled as an
 *    UNKNOWN-SIZE ARRAY extern (`extern Humanoid *SYM[]; ... SYM[0]`) so the
 *    load is the two-insn split-address form whose halves sched1 can
 *    interleave with the p->user load ([lui][lw user][lw %lo]); a plain
 *    small extern is ONE macro load insn and cannot interleave. Same
 *    respell for SOME_FIRST_PERSONISH_VIEW_RELATED_CAMERA_STATUS_[0] = 3
 *    (teleport's CamState.Mode store, lui+sw split, sw stolen into the
 *    break jump's delay slot).
 *  - The four pool-claim cases are ReqItemKusuri/Makibishi/Happou's matched
 *    idiom verbatim (cur/it split unnecessary here: single `it`); napalm's
 *    `pp` sits at the found: label like shuriken's nested `param`; reorg
 *    duplicates the
 *    addiu into the loop-exit branch's delay slot for napalm and steals it
 *    into the null-check beqz's slot for shuriken — same source shape.
 *  - The makibishi rand-loop is `while (1) { if (!(i < 5)) break; i++; ... }`
 *    (top test + unconditional back-jump, magic %100/%50 hoisted) with
 *    `i = 0;` AFTER the RotateVector call (sched1 hoists the move above the
 *    call; writing it before the call orders the merge block's addiu/move
 *    backwards).
 *  - jirai's tail needs vx/vy/vz temps for the work-vector reads (multi-use
 *    values in caller-saved regs; matches the batched-loads rule).
 */

/*
 * Retail case map (jump table order; retail case values differ from the
 * demo enum for 8/9 and 0x16-0x18):
 *   0 KAGINAWA  1 SHURIKEN  2 MAKIBISHI  3 KUSURI  4 FIRE  5 SMOKE
 *   6 JIRAI  7 DOKUDANGO  8 GOSHIKIMAI  9 NEMURI  a KAWARIMI  b HENSHIN
 *   c GOSIN  d SHINSOKU  e NINGYO  f HAPPOU  10 NINKEN  11 KAENGEKI
 *   12 MANEBUE  13 (nothing/default)  14 GUN  15 ARROW  16 NAPALM
 *   17 LIGHTNINGBOLT  18 TELEPORT
 */

#include "item.h"

extern Humanoid *CURRENTLY_SELECTED_CHARACTER_STATE_PTR[]; /* == CamState.Owner */
extern s32 SOME_FIRST_PERSONISH_VIEW_RELATED_CAMERA_STATUS_[]; /* == CamState.Mode */
extern s32 COUNTER_FOR_ITEM_ARRAY_;
extern ModelType *SyurikenModel;    /* shuriken/launch model (gp) */
extern Sprite3D *sprNapalm;     /* napalm model (gp) */

/* Per-item-type throw/offset vector constants (ITEM.C file data). */
extern VECTOR D_80012258[];
extern VECTOR D_80012268[];
extern VECTOR D_80012278[];
extern VECTOR D_80012288[];
extern VECTOR D_80012298[];
extern VECTOR D_800122A8[];
extern VECTOR D_800122B8[];
extern VECTOR D_800122C8[];
extern VECTOR D_800122D8[];

extern void GetVectorRotation(VECTOR *from, VECTOR *to, s32 *rx, s32 *ry);
extern Humanoid *SearchItemTarget2(Humanoid *owner, SVECTOR *rot,
                                   VECTOR *start, VECTOR *target);
extern void ProcSightShot(TItem *item);
extern void ProcKaginawa(TItem *item);
extern void ProcItemTeleport(TItem *item);
extern void ProcItemNapalm(TItem *item);
extern int ReqItemMakibishi(PARAM_ITEM_LAUNCH *p);
extern int ReqItemLaunch(PARAM_ITEM_LAUNCH *p);
extern int ReqItemSmoke(PARAM_ITEM_LAUNCH *p);
extern int ReqItemDokudango(PARAM_ITEM_LAUNCH *p);
extern int ReqItemNemuri(PARAM_ITEM_LAUNCH *p);
extern int ReqItemNingyo(PARAM_ITEM_LAUNCH *p);
extern int ReqItemGoshikimai(PARAM_ITEM_LAUNCH *p);
extern int ReqItemKaengeki(PARAM_ITEM_LAUNCH *p);
extern int ReqItemNinken(PARAM_ITEM_LAUNCH *p);
extern int ReqItemHappou(PARAM_ITEM_LAUNCH *p);
extern int ReqItemFire(PARAM_ITEM_LAUNCH *p);
extern int ReqItemLightningBolt(PARAM_ITEM_LAUNCH *p);
extern int ReqItemJirai(PARAM_ITEM_LAUNCH *p);
extern int ReqItemShinsoku(PARAM_ITEM_LAUNCH *p);
extern int ReqItemKusuri(PARAM_ITEM_LAUNCH *p);
extern int ReqItemGosin(PARAM_ITEM_LAUNCH *p);
extern void ReqItemGun(PARAM_ITEM_LAUNCH *p);
extern int ReqItemArrow(PARAM_ITEM_LAUNCH *p);
extern int ReqItemHenshin(PARAM_ITEM_LAUNCH *p);
extern int ReqItemKawarimi(PARAM_ITEM_LAUNCH *p);
extern int ReqItemManebue(PARAM_ITEM_LAUNCH *p);

int ReqItemUse(PARAM_ITEM_LAUNCH *p)
{
    enum
    {
        D = 100,
        D2 = 50
    };
    u8 c;
    PARAM_ITEM_LAUNCH param;  /* @sp+16: per-case launch params / vector scratch */
    PARAM_ITEM_LAUNCH work;   /* @sp+56: makibishi/jirai staging + throw vector */
    s32 sz;
    s32 y;
    s32 z;

    c = p->user->item[p->type];
    if (c != 0 && c != 0xff)
    {
        p->user->item[p->type] = c - 1;
    }

    switch (p->type)
    {
    case 2: /* MAKIBISHI */
    {
        ModelArchiveType *model;
        s32 rx;
        s32 ry;
        s32 rz;
        s32 i;

        memset(&work, 0, sizeof(PARAM_ITEM_LAUNCH));
        work.type = p->type;
        work.user = p->user;
        work.start = p->start;
        param = work;
        *(VECTOR *)&work = D_80012268[0];
        model = p->user->model;
        if (CamState.Owner->model == model && CamState.Mode == CMODE_DIRECTION)
        {
            GetVectorRotation((VECTOR *)&ViewInfo, (VECTOR *)&ViewInfo.vrx, &rx, &ry);
            rz = 0;
        }
        else
        {
            rx = model->rotate.vx;
            rz = model->rotate.vz;
            ry = model->rotate.vy;
        }
        RotateVector((VECTOR *)&work, rx, ry, rz);
        i = 0;
        while (1)
        {
            if (!(i < 5))
                break;
            i++;
            param.end.vx = ((VECTOR *)&work)->vx + rand() % D - D2;
            param.end.vy = ((VECTOR *)&work)->vy - rand() % D2 - D2;
            param.end.vz = ((VECTOR *)&work)->vz + rand() % D - D2;
            ReqItemMakibishi(&param);
        }
        break;
    }
    case 1: /* SHURIKEN */
    {
        TItem *it;
        TItem *cur;
        VECTOR *st;
        Humanoid *us;
        s32 ty;
        s32 i;

        if (p->user == CURRENTLY_SELECTED_CHARACTER_STATE_PTR[0])
        {
            param_launch *param;

            i = 0;
            do
            {
                COUNTER_FOR_ITEM_ARRAY_++;
                if (0x1d < COUNTER_FOR_ITEM_ARRAY_)
                    COUNTER_FOR_ITEM_ARRAY_ = 0;
                cur = items + COUNTER_FOR_ITEM_ARRAY_;
                if (cur->proc == 0)
                {
                    it = cur;
                    goto found_shuriken;
                }
                i++;
            } while (i < 0x1d);

            cur->mode = ITEM_MODE_DISPOSE;
            cur->proc(cur);
            DeleteConflict(cur->locate);
            if (cur->mode != 0)
            {
                AdtMessageBox(D_800121CC, cur->type, (u32)cur->mode);
            }
            it = cur;
            it->owner = 0;
            it->proc = 0;

        found_shuriken:
            param = &it->param.launch;
            if (it == 0)
                return 0;
            us = p->user;
            ty = p->type;
            it->owner = us;
            it->proc = ProcSightShot;
            it->mode = 0;
            it->type = ty;
            it->locate->locate.coord.t[0] = p->start.vx;
            st = &p->start;
            it->locate->locate.coord.t[1] = st->vy;
            it->locate->locate.coord.t[2] = st->vz;
            it->locate->locate.super = 0;
            UpdateCoordinate(it->locate);
            it->collision.size = 0;
            it->model = SyurikenModel;
            param->count = 5;
            it->owner->item[0x19] = 1;
        }
        else
        {
            param.type = p->type;
            param.user = p->user;
            param.start.vx = p->start.vx;
            param.start.vy = p->start.vy;
            param.start.vz = p->start.vz;
            SearchItemTarget2(param.user, &param.user->model->rotate, &param.start, &param.end);
            ReqItemLaunch(&param);
            p->user->item[0x19] = 0;
        }
        break;
    }
    case 5: /* SMOKE */
    {
        VECTOR *st;
        ModelArchiveType *model;
        s32 rx;
        s32 ry;
        s32 rz;

        *(VECTOR *)&param = D_80012278[0];
        st = (VECTOR *)&param;
        model = p->user->model;
        if (CamState.Owner->model == model && CamState.Mode == CMODE_DIRECTION)
        {
            GetVectorRotation((VECTOR *)&ViewInfo, (VECTOR *)&ViewInfo.vrx, &rx, &ry);
            rz = 0;
        }
        else
        {
            rx = model->rotate.vx;
            rz = model->rotate.vz;
            ry = model->rotate.vy;
        }
        RotateVector(st, rx, ry, rz);
        p->end.vx = ((VECTOR *)&param)->vx;
        p->end.vy = ((VECTOR *)&param)->vy;
        p->end.vz = ((VECTOR *)&param)->vz;
        ReqItemSmoke(p);
        break;
    }
    case 7: /* DOKUDANGO */
    {
        VECTOR *st;
        ModelArchiveType *model;
        s32 rx;
        s32 ry;
        s32 rz;

        *(VECTOR *)&param = D_80012288[0];
        st = (VECTOR *)&param;
        model = p->user->model;
        if (CamState.Owner->model == model && CamState.Mode == CMODE_DIRECTION)
        {
            GetVectorRotation((VECTOR *)&ViewInfo, (VECTOR *)&ViewInfo.vrx, &rx, &ry);
            rz = 0;
        }
        else
        {
            rx = model->rotate.vx;
            rz = model->rotate.vz;
            ry = model->rotate.vy;
        }
        RotateVector(st, rx, ry, rz);
        p->end.vx = ((VECTOR *)&param)->vx;
        p->end.vy = ((VECTOR *)&param)->vy;
        p->end.vz = ((VECTOR *)&param)->vz;
        ReqItemDokudango(p);
        break;
    }
    case 9: /* NEMURI */
    {
        VECTOR *st;
        ModelArchiveType *model;
        s32 rx;
        s32 ry;
        s32 rz;

        *(VECTOR *)&param = D_80012298[0];
        st = (VECTOR *)&param;
        model = p->user->model;
        if (CamState.Owner->model == model && CamState.Mode == CMODE_DIRECTION)
        {
            GetVectorRotation((VECTOR *)&ViewInfo, (VECTOR *)&ViewInfo.vrx, &rx, &ry);
            rz = 0;
        }
        else
        {
            rx = model->rotate.vx;
            rz = model->rotate.vz;
            ry = model->rotate.vy;
        }
        RotateVector(st, rx, ry, rz);
        p->end.vx = ((VECTOR *)&param)->vx;
        p->end.vy = ((VECTOR *)&param)->vy;
        p->end.vz = ((VECTOR *)&param)->vz;
        ReqItemNemuri(p);
        break;
    }
    case 0xe: /* NINGYO */
    {
        VECTOR *st;
        ModelArchiveType *model;
        s32 rx;
        s32 ry;
        s32 rz;

        *(VECTOR *)&param = D_800122A8[0];
        st = (VECTOR *)&param;
        model = p->user->model;
        if (CamState.Owner->model == model && CamState.Mode == CMODE_DIRECTION)
        {
            GetVectorRotation((VECTOR *)&ViewInfo, (VECTOR *)&ViewInfo.vrx, &rx, &ry);
            rz = 0;
        }
        else
        {
            rx = model->rotate.vx;
            rz = model->rotate.vz;
            ry = model->rotate.vy;
        }
        RotateVector(st, rx, ry, rz);
        p->end.vx = ((VECTOR *)&param)->vx;
        p->end.vy = ((VECTOR *)&param)->vy;
        p->end.vz = ((VECTOR *)&param)->vz;
        ReqItemNingyo(p);
        break;
    }
    case 8: /* GOSHIKIMAI */
    {
        VECTOR *st;
        ModelArchiveType *model;
        s32 rx;
        s32 ry;
        s32 rz;

        *(VECTOR *)&param = D_80012258[0];
        st = (VECTOR *)&param;
        model = p->user->model;
        if (CamState.Owner->model == model && CamState.Mode == CMODE_DIRECTION)
        {
            GetVectorRotation((VECTOR *)&ViewInfo, (VECTOR *)&ViewInfo.vrx, &rx, &ry);
            rz = 0;
        }
        else
        {
            rx = model->rotate.vx;
            rz = model->rotate.vz;
            ry = model->rotate.vy;
        }
        RotateVector(st, rx, ry, rz);
        p->end.vx = ((VECTOR *)&param)->vx;
        p->end.vy = ((VECTOR *)&param)->vy;
        p->end.vz = ((VECTOR *)&param)->vz;
        ReqItemGoshikimai(p);
        break;
    }
    case 0x11: /* KAENGEKI */
    {
        *(VECTOR *)&param = D_80012278[0];
        p->end.vx = ((VECTOR *)&param)->vx;
        p->end.vy = ((VECTOR *)&param)->vy;
        p->end.vz = ((VECTOR *)&param)->vz;
        ReqItemKaengeki(p);
        break;
    }
    case 0x10: /* NINKEN */
    {
        VECTOR *st;
        ModelArchiveType *model;
        s32 rx;
        s32 ry;
        s32 rz;

        *(VECTOR *)&param = D_800122A8[0];
        st = (VECTOR *)&param;
        model = p->user->model;
        if (CamState.Owner->model == model && CamState.Mode == CMODE_DIRECTION)
        {
            GetVectorRotation((VECTOR *)&ViewInfo, (VECTOR *)&ViewInfo.vrx, &rx, &ry);
            rz = 0;
        }
        else
        {
            rx = model->rotate.vx;
            rz = model->rotate.vz;
            ry = model->rotate.vy;
        }
        RotateVector(st, rx, ry, rz);
        p->end.vx = ((VECTOR *)&param)->vx;
        p->end.vy = ((VECTOR *)&param)->vy;
        p->end.vz = ((VECTOR *)&param)->vz;
        ReqItemNinken(p);
        break;
    }
    case 0xf: /* HAPPOU */
    {
        VECTOR *st;
        ModelArchiveType *model;
        s32 rx;
        s32 ry;
        s32 rz;

        *(VECTOR *)&param = D_800122A8[0];
        st = (VECTOR *)&param;
        model = p->user->model;
        if (CamState.Owner->model == model && CamState.Mode == CMODE_DIRECTION)
        {
            GetVectorRotation((VECTOR *)&ViewInfo, (VECTOR *)&ViewInfo.vrx, &rx, &ry);
            rz = 0;
        }
        else
        {
            rx = model->rotate.vx;
            rz = model->rotate.vz;
            ry = model->rotate.vy;
        }
        RotateVector(st, rx, ry, rz);
        p->end.vx = ((VECTOR *)&param)->vx;
        p->end.vy = ((VECTOR *)&param)->vy;
        p->end.vz = ((VECTOR *)&param)->vz;
        ReqItemHappou(p);
        break;
    }
    case 4: /* FIRE */
    {
        VECTOR *st;
        ModelArchiveType *model;
        s32 rx;
        s32 ry;
        s32 rz;

        *(VECTOR *)&param = D_800122A8[0];
        st = (VECTOR *)&param;
        model = p->user->model;
        if (CamState.Owner->model == model && CamState.Mode == CMODE_DIRECTION)
        {
            GetVectorRotation((VECTOR *)&ViewInfo, (VECTOR *)&ViewInfo.vrx, &rx, &ry);
            rz = 0;
        }
        else
        {
            rx = model->rotate.vx;
            rz = model->rotate.vz;
            ry = model->rotate.vy;
        }
        RotateVector(st, rx, ry, rz);
        p->end.vx = ((VECTOR *)&param)->vx;
        p->end.vy = ((VECTOR *)&param)->vy;
        p->end.vz = ((VECTOR *)&param)->vz;
        ReqItemFire(p);
        break;
    }
    case 0x17: /* LIGHTNINGBOLT */
    {
        VECTOR *st;
        ModelArchiveType *model;
        s32 rx;
        s32 ry;
        s32 rz;
        s32 sx;
        s32 t;
        s32 u;

        if (p->user == CamState.Owner)
        {
            *(VECTOR *)&param = D_800122B8[0];
            st = (VECTOR *)&param;
            model = p->user->model;
            if (CamState.Owner->model == model && CamState.Mode == CMODE_DIRECTION)
            {
                GetVectorRotation((VECTOR *)&ViewInfo, (VECTOR *)&ViewInfo.vrx, &rx, &ry);
                rz = 0;
            }
            else
            {
                rx = model->rotate.vx;
                rz = model->rotate.vz;
                ry = model->rotate.vy;
            }
            RotateVector(st, rx, ry, rz);
            sx = p->start.vx;
            sz = p->start.vz;
            t = ((VECTOR *)&param)->vx;
            p->end.vx = t;
            t = ((VECTOR *)&param)->vy;
            p->end.vy = t;
            t = p->end.vx;
            u = ((VECTOR *)&param)->vz;
            t = t + sx;
            p->end.vx = t;
            t = p->end.vy;
            p->end.vz = u;
            u = p->start.vy;
            sx = p->end.vz;
            y = t + u;
            z = sx + sz;
            p->end.vy = y;
            p->end.vz = z;
        }
        ReqItemLightningBolt(p);
        break;
    }
    case 6: /* JIRAI */
    {
        ModelArchiveType *model;
        s32 rx;
        s32 ry;
        s32 rz;
        s32 vx;
        s32 vy;
        s32 vz;

        memset(&work, 0, sizeof(PARAM_ITEM_LAUNCH));
        work.type = p->type;
        work.user = p->user;
        work.start = p->start;
        param = work;
        *(VECTOR *)&work = D_800122C8[0];
        model = p->user->model;
        if (CamState.Owner->model == model && CamState.Mode == CMODE_DIRECTION)
        {
            GetVectorRotation((VECTOR *)&ViewInfo, (VECTOR *)&ViewInfo.vrx, &rx, &ry);
            rz = 0;
        }
        else
        {
            rx = model->rotate.vx;
            rz = model->rotate.vz;
            ry = model->rotate.vy;
        }
        RotateVector((VECTOR *)&work, rx, ry, rz);
        vx = ((VECTOR *)&work)->vx;
        vy = ((VECTOR *)&work)->vy;
        vz = ((VECTOR *)&work)->vz;
        param.start.vx = param.start.vx + vx;
        param.end.vx = vx;
        param.end.vy = vy;
        param.end.vz = vz;
        param.start.vy = param.start.vy + vy;
        param.start.vz = param.start.vz + vz;
        ReqItemJirai(&param);
        break;
    }
    case 0: /* KAGINAWA */
    {
        TItem *it;
        TItem *cur;
        VECTOR *st;
        Humanoid *us;
        s32 ty;
        s32 i;

        i = 0;
        sz = (s32)items;
        do
        {
            COUNTER_FOR_ITEM_ARRAY_++;
            if (0x1d < COUNTER_FOR_ITEM_ARRAY_)
                COUNTER_FOR_ITEM_ARRAY_ = 0;
            z = COUNTER_FOR_ITEM_ARRAY_ * sizeof(*items);
            cur = (TItem *)(z + sz);
            if (cur->proc == 0)
            {
                it = cur;
                goto found_kaginawa;
            }
            i++;
        } while (i < 0x1d);

        cur->mode = ITEM_MODE_DISPOSE;
        cur->proc(cur);
        DeleteConflict(cur->locate);
        if (cur->mode != 0)
        {
            AdtMessageBox(D_800121CC, cur->type, (u32)cur->mode);
        }
        it = cur;
        it->owner = 0;
        it->proc = 0;

    found_kaginawa:
        if (it == 0)
            return 0;
        y = (s32)ProcKaginawa;
        us = p->user;
        ty = p->type;
        it->owner = us;
        it->proc = (void (*)(TItem *))y;
        it->mode = 0;
        it->type = ty;
        it->locate->locate.coord.t[0] = p->start.vx;
        st = &p->start;
        it->locate->locate.coord.t[1] = st->vy;
        it->locate->locate.coord.t[2] = st->vz;
        it->locate->locate.super = 0;
        UpdateCoordinate(it->locate);
        it->collision.size = 0;
        it->model = 0;
        it->owner->item[0x19] = 1;
        SetCameraMode(CMODE_SIGHT);
        CamState.DirectionRX = -0x155;
        CamState.DirectionRY = 0;
        break;
    }
    case 0xd: /* SHINSOKU */
    {
        VECTOR *st;
        ModelArchiveType *model;
        s32 rx;
        s32 ry;
        s32 rz;

        *(VECTOR *)&param = D_800122D8[0];
        st = (VECTOR *)&param;
        model = p->user->model;
        if (CamState.Owner->model == model && CamState.Mode == CMODE_DIRECTION)
        {
            GetVectorRotation((VECTOR *)&ViewInfo, (VECTOR *)&ViewInfo.vrx, &rx, &ry);
            rz = 0;
        }
        else
        {
            rx = model->rotate.vx;
            rz = model->rotate.vz;
            ry = model->rotate.vy;
        }
        RotateVector(st, rx, ry, rz);
        p->end.vx = ((VECTOR *)&param)->vx;
        p->end.vy = ((VECTOR *)&param)->vy;
        p->end.vz = ((VECTOR *)&param)->vz;
        ReqItemShinsoku(p);
        break;
    }
    case 0x18: /* TELEPORT */
    {
        TItem *it;
        TItem *cur;
        VECTOR *st;
        Humanoid *us;
        s32 ty;
        s32 i;

        i = 0;
        do
        {
            COUNTER_FOR_ITEM_ARRAY_++;
            if (0x1d < COUNTER_FOR_ITEM_ARRAY_)
                COUNTER_FOR_ITEM_ARRAY_ = 0;
            cur = items + COUNTER_FOR_ITEM_ARRAY_;
            if (cur->proc == 0)
            {
                it = cur;
                goto found_teleport;
            }
            i++;
        } while (i < 0x1d);

        cur->mode = ITEM_MODE_DISPOSE;
        cur->proc(cur);
        DeleteConflict(cur->locate);
        if (cur->mode != 0)
        {
            AdtMessageBox(D_800121CC, cur->type, (u32)cur->mode);
        }
        it = cur;
        it->owner = 0;
        it->proc = 0;

    found_teleport:
        if (it == 0)
            return 0;
        us = p->user;
        ty = p->type;
        it->owner = us;
        it->proc = ProcItemTeleport;
        it->mode = 0;
        it->type = ty;
        it->locate->locate.coord.t[0] = p->start.vx;
        st = &p->start;
        it->locate->locate.coord.t[1] = st->vy;
        it->locate->locate.coord.t[2] = st->vz;
        it->locate->locate.super = 0;
        UpdateCoordinate(it->locate);
        it->collision.size = 0;
        it->model = 0;
        SOME_FIRST_PERSONISH_VIEW_RELATED_CAMERA_STATUS_[0] = 3;
        break;
    }
    case 3: /* KUSURI */
    {
        VECTOR *st;
        ModelArchiveType *model;
        s32 rx;
        s32 ry;
        s32 rz;

        *(VECTOR *)&param = D_800122A8[0];
        st = (VECTOR *)&param;
        model = p->user->model;
        if (CamState.Owner->model == model && CamState.Mode == CMODE_DIRECTION)
        {
            GetVectorRotation((VECTOR *)&ViewInfo, (VECTOR *)&ViewInfo.vrx, &rx, &ry);
            rz = 0;
        }
        else
        {
            rx = model->rotate.vx;
            rz = model->rotate.vz;
            ry = model->rotate.vy;
        }
        RotateVector(st, rx, ry, rz);
        p->end.vx = ((VECTOR *)&param)->vx;
        p->end.vy = ((VECTOR *)&param)->vy;
        p->end.vz = ((VECTOR *)&param)->vz;
        ReqItemKusuri(p);
        break;
    }
    case 0xc: /* GOSIN */
    {
        VECTOR *st;
        ModelArchiveType *model;
        s32 rx;
        s32 ry;
        s32 rz;

        *(VECTOR *)&param = D_800122A8[0];
        st = (VECTOR *)&param;
        model = p->user->model;
        if (CamState.Owner->model == model && CamState.Mode == CMODE_DIRECTION)
        {
            GetVectorRotation((VECTOR *)&ViewInfo, (VECTOR *)&ViewInfo.vrx, &rx, &ry);
            rz = 0;
        }
        else
        {
            rx = model->rotate.vx;
            rz = model->rotate.vz;
            ry = model->rotate.vy;
        }
        RotateVector(st, rx, ry, rz);
        p->end.vx = ((VECTOR *)&param)->vx;
        p->end.vy = ((VECTOR *)&param)->vy;
        p->end.vz = ((VECTOR *)&param)->vz;
        ReqItemGosin(p);
        break;
    }
    case 0x14: /* GUN */
        ReqItemGun(p);
        break;
    case 0x15: /* ARROW */
        ReqItemArrow(p);
        break;
    case 0x16: /* NAPALM */
    {
        TItem *it;
        TItem *cur;
        param_napalm *pp;
        VECTOR *st;
        Humanoid *us;
        s32 ty;
        s32 i;

        i = 0;
        do
        {
            COUNTER_FOR_ITEM_ARRAY_++;
            if (0x1d < COUNTER_FOR_ITEM_ARRAY_)
                COUNTER_FOR_ITEM_ARRAY_ = 0;
            cur = items + COUNTER_FOR_ITEM_ARRAY_;
            if (cur->proc == 0)
            {
                it = cur;
                goto found_napalm;
            }
            i++;
        } while (i < 0x1d);

        cur->mode = ITEM_MODE_DISPOSE;
        cur->proc(cur);
        DeleteConflict(cur->locate);
        if (cur->mode != 0)
        {
            AdtMessageBox(D_800121CC, cur->type, (u32)cur->mode);
        }
        it = cur;
        it->owner = 0;
        it->proc = 0;

    found_napalm:
        pp = &it->param.napalm;
        if (it == 0)
            return 0;
        if ((GameClock & 1) == 0)
            return 0;
        us = p->user;
        ty = p->type;
        it->owner = us;
        it->proc = ProcItemNapalm;
        it->mode = 0;
        it->type = ty;
        it->locate->locate.coord.t[0] = p->start.vx;
        st = &p->start;
        it->locate->locate.coord.t[1] = st->vy;
        it->locate->locate.coord.t[2] = st->vz;
        it->locate->locate.super = 0;
        UpdateCoordinate(it->locate);
        it->collision.size = 0;
        it->model = (ModelType *)sprNapalm;
        it->param.napalm.vec.vx = p->end.vx - p->start.vx;
        pp->vec.vy = p->end.vy - p->start.vy;
        pp->vec.vz = p->end.vz - p->start.vz;
        break;
    }
    case 0xb: /* HENSHIN */
        ReqItemHenshin(p);
        break;
    case 0xa: /* KAWARIMI */
        ReqItemKawarimi(p);
        break;
    case 0x12: /* MANEBUE */
        ReqItemManebue(p);
    case 0x13:
    default:
        break;
    }
    return 1;
}
