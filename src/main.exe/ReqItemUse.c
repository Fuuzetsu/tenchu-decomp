#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemUse(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:3631, 386 src lines, frame 440 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 *    The kusuri-family "VECTOR v" lives at the head of `param`;
 *    makibishi/jirai stage a PARAM_ITEM_DROP in `work` (memset + 3 field
 *    copies + `param = work`), then reuse work's head as the throw vector.
 *    ItemRequestWorkspace names all three views without pointer casts.
 *  - Retail case map from the jump table (demo enum differs: 8/9 swapped,
 *    0x16=NAPALM, 0x17=LIGHTNINGBOLT, 0x18=TELEPORT).
 *  - The camera-check template is ReqItemDefault.c's idiom verbatim; the
 *    The `st` pointer temp before the guard puts &param.vector in
 *    a callee-saved reg across GetVectorRotation (guard delay slot addiu).
 *  - The owner guard and teleport transition use the recovered
 *    `CamState.Owner` and `CamState.Mode` fields.  `CMODE_SIGHT` names the
 *    latter's retail value 3, and the ordinary member accesses reproduce the
 *    target's split-address schedule without separate alias objects.
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

/* Every launcher aims the same way: along the first-person view when the
 * thrower is the aimed camera owner (CMODE_DIRECTION), else along the
 * thrower model's own facing. The block is ReqItemDefault.c's idiom,
 * copy-pasted into every case in retail; the macro is reconstruction
 * shorthand for that copy-paste (expands to the identical text). */
#define GET_THROW_ROTATION(mdl, rx, ry, rz)                                   \
    if (CamState.Owner->model == (mdl) && CamState.Mode == CMODE_DIRECTION)   \
    {                                                                         \
        GetVectorRotation((VECTOR *)&ViewInfo, (VECTOR *)&ViewInfo.vrx,       \
                          &(rx), &(ry));                                      \
        (rz) = 0;                                                             \
    }                                                                         \
    else                                                                      \
    {                                                                         \
        (rx) = (mdl)->rotate.vx;                                              \
        (rz) = (mdl)->rotate.vz;                                              \
        (ry) = (mdl)->rotate.vy;                                              \
    }

#define REQUEST_ROTATED_ITEM(vector_, request_)                               \
    VECTOR *st;                                                               \
    ModelArchiveType *model;                                                  \
    s32 rx;                                                                   \
    s32 ry;                                                                   \
    s32 rz;                                                                   \
                                                                              \
    param.vector = vector_[0];                                                \
    st = &param.vector;                                                       \
    model = p->user->model;                                                   \
    GET_THROW_ROTATION(model, rx, ry, rz);                                    \
    RotateVector(st, rx, ry, rz);                                             \
    p->end.vx = param.vector.vx;                                              \
    p->end.vy = param.vector.vy;                                              \
    p->end.vz = param.vector.vz;                                              \
    request_(p)

#define SETUP_ROTATED_DROP(vector_)                                           \
    memset(&work, 0, sizeof(work));                                           \
    work.drop.type = p->type;                                                 \
    work.drop.user = p->user;                                                 \
    work.drop.start = p->start;                                               \
    param = work;                                                             \
    work.vector = vector_[0];                                                 \
    model = p->user->model;                                                   \
    GET_THROW_ROTATION(model, rx, ry, rz);                                    \
    RotateVector(&work.vector, rx, ry, rz)

#define RECLAIM_POOL_ITEM()                                                   \
    cur->mode = ITEM_MODE_DISPOSE;                                            \
    cur->proc(cur);                                                           \
    DeleteConflict(cur->locate);                                              \
    if (cur->mode != ITEM_MODE_START)                                         \
    {                                                                         \
        AdtMessageBox(msg_item_dispose_fail, cur->type, (u32)cur->mode);      \
    }                                                                         \
    it = cur;                                                                 \
    it->owner = 0;                                                            \
    it->proc = 0

#define SETUP_POOL_ITEM(proc_, model_)                                        \
    us = p->user;                                                             \
    ty = p->type;                                                             \
    it->owner = us;                                                           \
    it->proc = proc_;                                                         \
    it->mode = ITEM_MODE_START;                                               \
    it->type = ty;                                                            \
    it->locate->locate.coord.t[0] = p->start.vx;                              \
    st = &p->start;                                                           \
    it->locate->locate.coord.t[1] = st->vy;                                   \
    it->locate->locate.coord.t[2] = st->vz;                                   \
    it->locate->locate.super = 0;                                             \
    UpdateCoordinate(it->locate);                                             \
    it->collision.size = 0;                                                   \
    it->model = model_


/* Per-item-type throw/offset vector constants (ITEM.C file data). */
extern VECTOR vec_z_n100[];        /* {0,0,-100} */
extern VECTOR vec_z_100[];         /* {0,0,100} */
extern VECTOR vec_z_n60[];         /* {0,0,-60} */
extern VECTOR vec_y_n120_z_n240[]; /* {0,-120,-240} */
extern VECTOR vec_z_n120[];        /* {0,0,-120} */
extern VECTOR vec_y_n120_z_n120[]; /* {0,-120,-120} */
extern VECTOR vec_z_n4096[];       /* {0,0,-4096} */
extern VECTOR vec_z_n1000[];       /* {0,0,-1000} */
extern VECTOR vec_z_n500[];        /* {0,0,-500} */

extern Humanoid *SearchItemTarget2(Humanoid *owner, SVECTOR *rot,
                                   VECTOR *start, VECTOR *target);
extern void ProcSightShot(TItem *item);
extern void ProcKaginawa(TItem *item);
extern void ProcItemTeleport(TItem *item);
extern void ProcItemNapalm(TItem *item);
extern int ReqItemMakibishi(PARAM_ITEM_DROP *p);
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
extern int ReqItemJirai(PARAM_ITEM_DROP *p);
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
    ItemRequestWorkspace param; /* @sp+16: per-case request / vector scratch */
    ItemRequestWorkspace work;  /* @sp+56: drop staging / throw vector */
    s32 sz;
    s32 y;
    s32 z;

    c = p->user->item[p->type];
    if (c != 0 && c != ITEM_INFINITE)
    {
        p->user->item[p->type] = c - 1;
    }

    switch (p->type)
    {
    case ITEM_MAKIBISHI:
    {
        ModelArchiveType *model;
        s32 rx;
        s32 ry;
        s32 rz;
        s32 i;

        SETUP_ROTATED_DROP(vec_z_100);
        i = 0;
        while (1)
        {
            if (i >= 5)
                break;
            i++;
            param.drop.vec.vx = work.vector.vx + rand() % D - D2;
            param.drop.vec.vy = work.vector.vy - rand() % D2 - D2;
            param.drop.vec.vz = work.vector.vz + rand() % D - D2;
            ReqItemMakibishi(&param.drop);
        }
        break;
    }
    case ITEM_SHURIKEN:
    {
        TItem *it;
        TItem *cur;
        VECTOR *st;
        Humanoid *us;
        s32 ty;
        s32 i;

        if (p->user == CamState.Owner)
        {
            param_launch *param;

            i = 0;
            do
            {
                ic++;
                if (ic >= MAX_ITEMS)
                    ic = 0;
                cur = items + ic;
                if (cur->proc == 0)
                {
                    it = cur;
                    goto found_shuriken;
                }
                i++;
            } while (i < MAX_ITEMS - 1);

        RECLAIM_POOL_ITEM();

    found_shuriken:
            param = &it->param.launch;
            if (it == 0)
                return 0;
        SETUP_POOL_ITEM(ProcSightShot, SyurikenModel);
            param->count = 5;
            it->owner->item[ITEM_N] = 1;
        }
        else
        {
            param.launch.type = p->type;
            param.launch.user = p->user;
            param.launch.start.vx = p->start.vx;
            param.launch.start.vy = p->start.vy;
            param.launch.start.vz = p->start.vz;
            SearchItemTarget2(param.launch.user,
                              &param.launch.user->model->rotate,
                              &param.launch.start, &param.launch.end);
            ReqItemLaunch(&param.launch);
            p->user->item[ITEM_N] = 0;
        }
        break;
    }
    case ITEM_SMOKE:
    {
        REQUEST_ROTATED_ITEM(vec_z_n60, ReqItemSmoke);
        break;
    }
    case ITEM_DOKUDANGO:
    {
        REQUEST_ROTATED_ITEM(vec_y_n120_z_n240, ReqItemDokudango);
        break;
    }
    case ITEM_NEMURI:
    {
        REQUEST_ROTATED_ITEM(vec_z_n120, ReqItemNemuri);
        break;
    }
    case ITEM_NINGYO:
    {
        REQUEST_ROTATED_ITEM(vec_y_n120_z_n120, ReqItemNingyo);
        break;
    }
    case ITEM_GOSHIKIMAI:
    {
        REQUEST_ROTATED_ITEM(vec_z_n100, ReqItemGoshikimai);
        break;
    }
    case ITEM_KAENGEKI:
    {
        param.vector = vec_z_n60[0];
        p->end.vx = param.vector.vx;
        p->end.vy = param.vector.vy;
        p->end.vz = param.vector.vz;
        ReqItemKaengeki(p);
        break;
    }
    case ITEM_NINKEN:
    {
        REQUEST_ROTATED_ITEM(vec_y_n120_z_n120, ReqItemNinken);
        break;
    }
    case ITEM_HAPPOU:
    {
        REQUEST_ROTATED_ITEM(vec_y_n120_z_n120, ReqItemHappou);
        break;
    }
    case ITEM_FIRE:
    {
        REQUEST_ROTATED_ITEM(vec_y_n120_z_n120, ReqItemFire);
        break;
    }
    case ITEM_LIGHTNINGBOLT:
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
            param.vector = vec_z_n4096[0];
            st = &param.vector;
            model = p->user->model;
            GET_THROW_ROTATION(model, rx, ry, rz);
            RotateVector(st, rx, ry, rz);
            sx = p->start.vx;
            sz = p->start.vz;
            t = param.vector.vx;
            p->end.vx = t;
            t = param.vector.vy;
            p->end.vy = t;
            t = p->end.vx;
            u = param.vector.vz;
            t += sx;
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
    case ITEM_JIRAI:
    {
        ModelArchiveType *model;
        s32 rx;
        s32 ry;
        s32 rz;
        s32 vx;
        s32 vy;
        s32 vz;

        SETUP_ROTATED_DROP(vec_z_n1000);
        vx = work.vector.vx;
        vy = work.vector.vy;
        vz = work.vector.vz;
        param.drop.start.vx += vx;
        param.drop.vec.vx = vx;
        param.drop.vec.vy = vy;
        param.drop.vec.vz = vz;
        param.drop.start.vy += vy;
        param.drop.start.vz += vz;
        ReqItemJirai(&param.drop);
        break;
    }
    case ITEM_KAGINAWA:
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
            ic++;
            if (ic >= MAX_ITEMS)
                ic = 0;
            z = ic * sizeof(*items);
            cur = (TItem *)(z + sz);
            if (cur->proc == 0)
            {
                it = cur;
                goto found_kaginawa;
            }
            i++;
        } while (i < MAX_ITEMS - 1);

        RECLAIM_POOL_ITEM();

    found_kaginawa:
        if (it == 0)
            return 0;
        /* Its two siblings pass the proc straight to SETUP_POOL_ITEM;
         * this one goes through `y` because `y` is the function-scope
         * scratch the rope case above also uses as a height, and sharing
         * that one pseudo is what matches. Passing ProcKaginawa directly
         * costs 8 lines. */
        y = (s32)ProcKaginawa;
        SETUP_POOL_ITEM((void (*)(TItem *))y, 0);
        it->owner->item[ITEM_N] = 1;
        SetCameraMode(CMODE_SIGHT);
        CamState.DirectionRX = -0x155;
        CamState.DirectionRY = 0;
        break;
    }
    case ITEM_SHINSOKU:
    {
        REQUEST_ROTATED_ITEM(vec_z_n500, ReqItemShinsoku);
        break;
    }
    case ITEM_TELEPORT:
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
            ic++;
            if (ic >= MAX_ITEMS)
                ic = 0;
            cur = items + ic;
            if (cur->proc == 0)
            {
                it = cur;
                goto found_teleport;
            }
            i++;
        } while (i < MAX_ITEMS - 1);

        RECLAIM_POOL_ITEM();

    found_teleport:
        if (it == 0)
            return 0;
        SETUP_POOL_ITEM(ProcItemTeleport, 0);
        CamState.Mode = CMODE_SIGHT;
        break;
    }
    case ITEM_KUSURI:
    {
        REQUEST_ROTATED_ITEM(vec_y_n120_z_n120, ReqItemKusuri);
        break;
    }
    case ITEM_GOSIN:
    {
        REQUEST_ROTATED_ITEM(vec_y_n120_z_n120, ReqItemGosin);
        break;
    }
    case ITEM_GUN:
        ReqItemGun(p);
        break;
    case ITEM_ARROW:
        ReqItemArrow(p);
        break;
    case ITEM_NAPALM:
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
            ic++;
            if (ic >= MAX_ITEMS)
                ic = 0;
            cur = items + ic;
            if (cur->proc == 0)
            {
                it = cur;
                goto found_napalm;
            }
            i++;
        } while (i < MAX_ITEMS - 1);

        RECLAIM_POOL_ITEM();

    found_napalm:
        pp = &it->param.napalm;
        if (it == 0)
            return 0;
        if ((GameClock & 1) == 0)
            return 0;
        SETUP_POOL_ITEM(ProcItemNapalm, (ModelType *)sprNapalm);
        it->param.napalm.vec.vx = p->end.vx - p->start.vx;
        pp->vec.vy = p->end.vy - p->start.vy;
        pp->vec.vz = p->end.vz - p->start.vz;
        break;
    }
    case ITEM_HENSHIN:
        ReqItemHenshin(p);
        break;
    case ITEM_KAWARIMI:
        ReqItemKawarimi(p);
        break;
    case ITEM_MANEBUE:
        ReqItemManebue(p);
    case ITEM_ARMOUR:
    default:
        break;
    }
    return 1;
}
#undef SETUP_POOL_ITEM
#undef RECLAIM_POOL_ITEM
#undef SETUP_ROTATED_DROP
#undef REQUEST_ROTATED_ITEM
