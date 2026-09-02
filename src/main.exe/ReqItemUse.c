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

#include "item.h"

#define GET_THROW_ROTATION(mdl, rx, ry, rz)                                   \
    if (CamState.Owner->model == (mdl) && CamState.Mode == CMODE_DIRECTION)   \
    {                                                                         \
        GetVectorRotation(CAMERA_VIEWPOINT(&ViewInfo),                       \
                          CAMERA_REFERENCE(&ViewInfo),                       \
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
        TItem *item;
        TItem *ret;
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;
        s32 i;

        if (p->user == CamState.Owner)
        {
            param_launch *param;

            TAKE_ITEM_SLOT_VIA_CURSOR(found_shuriken);

    found_shuriken:
            param = &item->param.launch;
            if (item == 0)
                return 0;
            INITIALIZE_ITEM_FROM_REQUEST(ProcSightShot);
            item->collision.size = 0;
            item->model = SyurikenModel;
            param->count = 5;
            item->owner->item[ITEM_N] = 1;
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

        if (p->user == CamState.Owner)
        {
            param.vector = vec_z_n4096[0];
            st = &param.vector;
            model = p->user->model;
            GET_THROW_ROTATION(model, rx, ry, rz);
            RotateVector(st, rx, ry, rz);
            p->end.vx = param.vector.vx;
            p->end.vy = param.vector.vy;
            p->end.vz = param.vector.vz;
            p->end.vx += p->start.vx;
            p->end.vy += p->start.vy;
            p->end.vz += p->start.vz;
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
        TItem *item;
        TItem *ret;
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;
        s32 i;

        TAKE_ITEM_SLOT_VIA_CURSOR(found_kaginawa);

    found_kaginawa:
        if (item == 0)
            return 0;
        INITIALIZE_ITEM_FROM_REQUEST(ProcKaginawa);
        item->collision.size = 0;
        item->model = 0;
        item->owner->item[ITEM_N] = 1;
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
        TItem *item;
        TItem *ret;
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;
        s32 i;

        TAKE_ITEM_SLOT_VIA_CURSOR(found_teleport);

    found_teleport:
        if (item == 0)
            return 0;
        INITIALIZE_ITEM_FROM_REQUEST(ProcItemTeleport);
        item->collision.size = 0;
        item->model = 0;
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
        TItem *item;
        TItem *ret;
        param_napalm *pp;
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;
        s32 i;

        TAKE_ITEM_SLOT_VIA_CURSOR(found_napalm);

    found_napalm:
        pp = &item->param.napalm;
        if (item == 0)
            return 0;
        if ((GameClock & 1) == 0)
            return 0;
        INITIALIZE_ITEM_FROM_REQUEST(ProcItemNapalm);
        item->collision.size = 0;
        item->model = (ModelType *)sprNapalm;
        item->param.napalm.vec.vx = p->end.vx - p->start.vx;
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
#undef SETUP_ROTATED_DROP
#undef REQUEST_ROTATED_ITEM
