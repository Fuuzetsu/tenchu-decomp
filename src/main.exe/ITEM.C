#include "common.h"
#include "main.exe.h"
#include "adt.h"
#include "action.h"
#include "item.h"
#include "images.h"
#include "model.h"
#include "appear.h"
#include "tuning.h"
#include "sound.h"
#include <psxsdk/libgs.h>
#include "effect.h"
#include "humanoid.h"
#include "afterimage.h"

/*
 * Retail reorganises ITEM.C, adding several helpers and omitting two demo
 * routines from this text range. The translation-unit manifest retains the
 * earlier debug-symbol order independently.
 */

static u8 fInitial = 0;
static s32 ic = 0;

static __inline__ TItem *TakeItemSlot(void)
{
    TItem *item;
    s32 i;

    i = 0;
    do
    {
        ic++;
        if (ic >= MAX_ITEMS)
            ic = 0;
        item = items + ic;
        if (item->proc == 0)
            return item;
        i++;
    } while (i < MAX_ITEMS - 1);

    DISPOSE_ITEM(item);
    return item;
}

static __inline__ void DropStayedItem(PARAM_ITEM_STAY *request)
{
    PARAM_ITEM_LAUNCH drop_request;

    drop_request.type = request->type;
    drop_request.user = ITEM_OWNER_UNCLAIMED;
    copyVector(&drop_request.start, &request->locate);
    setVector(&drop_request.end, 0, 0, 0);
    drop_request.start.vy = GetAreaMapLevel(
        GlobalAreaMap, drop_request.start.vx,
        drop_request.start.vy, drop_request.start.vz,
        AREA_LEVEL_DEFAULT);
    ReqItemDrop(&drop_request);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static struct Humanoid * SearchItemTarget2(struct Humanoid *owner, struct SVECTOR *rot, struct VECTOR *start, struct VECTOR *target);
 *     ITEM.C:352, 62 src lines, frame 136 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $fp       struct Humanoid * owner
 *     param $s1       struct SVECTOR * rot
 *     param $s5       struct VECTOR * start
 *     param $s7       struct VECTOR * target
 *     reg   $s1       int i
 *     reg   $s4       int dist
 *     reg   $s6       struct Humanoid * ret
 *     stack sp+16     struct MATRIX mat
 *     stack sp+48     struct SVECTOR rrot
 *     stack sp+56     struct SVECTOR v
 *     reg   $s0       struct VECTOR * gpv
 *     reg   $s0       struct Humanoid * human
 *     stack sp+64     struct VECTOR tv
 *     stack sp+80     struct VECTOR lv
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *HumanGroup[32];
 *     extern short Humans;
 * END PSX.SYM */

extern VECTOR vec_z_n17000;


static Humanoid *SearchItemTarget2(Humanoid *owner, SVECTOR *rot, VECTOR *start, VECTOR *target)
{
    int i;
    int dist;
    Humanoid *ret;
    Humanoid *human;
    MATRIX mat;
    SVECTOR rrot;
    VECTOR tv;
    VECTOR lv;
    int cond;
    int z;

    ret = 0;
    tv = vec_z_n17000;
    RotateVector(&tv, rot->vx, rot->vy, rot->vz);
    tv.vx += start->vx;
    tv.vy += start->vy;
    tv.vz += start->vz;
    trace_ground_(start, &tv, &lv, 0);
    setVector(target, lv.vx, lv.vy, lv.vz);
    dist = GetVectorDistance(&lv, start);
    rrot.vx = -rot->vx;
    rrot.vy = -rot->vy;
    rrot.vz = -rot->vz;
    RotMatrix(&rrot, &mat);

    i = 0;
    while (1)
    {
        if (i >= Humans)
            break;
        human = HumanGroup[i];
        tv = *GetAbsolutePosition(*human->model->object, 0, 0, 0);
        lv = tv;
        if (human->life > 0 && human != owner)
        {
            lv.vx -= start->vx;
            lv.vy -= start->vy;
            lv.vz -= start->vz;
            ApplyMatrixLV(&mat, &lv, &lv);
            lv.vz = -lv.vz;
            if (lv.vz > 100)
            {
                cond = 0;
                if (abs(lv.vx) < 500)
                {
                    cond = abs(lv.vy) < 1000;
                }
                if (cond && (z = lv.vz, z < dist))
                {
                    dist = z;
                    *target = tv;
                    ret = human;
                }
            }
        }
        i++;
    }
    return ret;
}

static __inline__ Humanoid *FindItemTarget(TFindItemTarget *find)
{
    int i;
    Humanoid *target;
    int dist;

    for (i = find->i; ; i++)
    {
        if (i >= Humans)
        {
            return 0;
        }
        target = HumanGroup[i];
        if (target->life > 0 && target->motion->mid != MOT_ACTION &&
            (target->attribute & ATTR_SUSPEND) == 0)
        {
            dist = GetVectorDistance(&find->pos, target->locate);
            if (dist < find->find_dist)
            {
                find->find = target;
                find->dist = dist;
                find->i = i + 1;
                return find->find;
            }
        }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PackItemLayout(void *buf, long size);
 *     ITEM.C:473, 30 src lines, frame 224 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       void * buf
 *     param $a1       long size
 *     stack sp+16     unsigned char [200] fn
 *     reg   $a2       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

extern char fmt_item_storing_size_too[]; /* item storing size too small %d/%d */

void PackItemLayout(void *buf, s32 size)
{
    s32 i;
    TItemLayout *slot;
    VECTOR *locate;

    if ((u32)size < sizeof(TItemLayout) * MAX_ITEMS)
    {
        AdtMessageBox(fmt_item_storing_size_too, size,
                      sizeof(TItemLayout) * MAX_ITEMS);
    }
    else
    {
        i = 0;
        do
        {
            slot = &((TItemLayout *)buf)[i];
            if (items[i].proc != 0)
            {
                slot->type = items[i].type;
                slot->locate.vx = items[i].locate->locate.coord.t[0];
                locate = &slot->locate;
                locate->vy = items[i].locate->locate.coord.t[1];
                locate->vz = items[i].locate->locate.coord.t[2];
            }
            else
            {
                slot->type = ITEM_NONE;
            }
            i++;
        } while (i < MAX_ITEMS);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void RestoreItemLayout(void *buf);
 *     ITEM.C:507, 22 src lines, frame 288 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       void * buf
 *     reg   $s3       struct TItemLayout * slot
 *     stack sp+16     unsigned char [200] fn
 *     reg   $s1       int i
 *     reg   $s1       int i
 *     stack sp+216    struct PARAM_ITEM_STAY param
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

/* The loop consumes four {dx,dz} pairs despite this four-short declaration.
 * Keep the historical bound until the symbol can be typed without changing
 * its small-data addressing. */
extern short DropOffsets[4]; /* {dx,dz} probe pairs x1000 */


void RestoreItemLayout(void *buf)
{
    TItem *it;
    TItemLayout *slot;
    s32 i;
    PARAM_ITEM_STAY param;
    s32 level;
    s32 level_mode;
    s32 sentinel;
    s32 x;
    s32 z;

    {
        s32 i;

        for (i = 0; ; i++)
        {
            if (i < MAX_ITEMS)
            {
                it = &items[i];
                if (it->proc != 0)
                {
                    DISPOSE_ITEM(it);
                }
                continue;
            }
            break;
        }
    }

    i = 0;
    level_mode = AREA_LEVEL_STEP_DOWN;
    sentinel = LEVEL_NONE;
    for (slot = buf; ; slot++, i++)
    {
        if (i >= MAX_ITEMS)
            return;
        if (slot->type != ITEM_NONE)
        {
            PARAM_ITEM_STAY tmp = {0};

            tmp.type = slot->type;
            tmp.locate = slot->locate;
            param = tmp;

            level = GetAreaMapLevel(GlobalAreaMap, param.locate.vx, param.locate.vy, param.locate.vz, level_mode);
            if (level == sentinel || abs(level - param.locate.vy) >= 1000)
            {
                s32 k;
                short *offs;

                k = 0;
                offs = DropOffsets;
                for (;;)
                {
                    if (k < 4)
                    {
                        x = param.locate.vx + offs[0] * 1000;
                        z = param.locate.vz + offs[1] * 1000;
                        level = GetAreaMapLevel(GlobalAreaMap, x,
                                                param.locate.vy, z, level_mode);
                        if (level != sentinel &&
                            abs(level - param.locate.vy) < 1000)
                        {
                            param.locate.vx = x;
                            param.locate.vz = z;
                            break;
                        }
                        offs += 2;
                        k++;
                        continue;
                    }
                    break;
                }
                if (k == 4)
                {
                    continue;
                }
            }
            param.locate.vy = level;
            ReqItemStay(&param);
        }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitializeItem(void);
 *     ITEM.C:533, 40 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s1       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType *SyurikenModel;
 *     extern struct ModelType *ArrowModel;
 *     extern struct ModelType *NingyoModel;
 *     extern struct tag_TItem items[30];
 *     extern struct ModelType *HappouModel;
 *     extern struct GsSPRITE TargetSprite[1];
 *     extern struct Sprite3D *sprNapalm;
 *     extern struct Sprite3D *sprNapalm2;
 * END PSX.SYM */

extern GsSPRITE SpriteGoshikimai;


void InitializeItem(void)
{
    s32 i;
    u32 attr;

    SyurikenModel = LoadModel(GetArcData(MODEL_SYURIKEN));
    ArrowModel = LoadModel(GetArcData(MODEL_ARROW));
    NingyoModel = LoadModel(GetArcData(MODEL_NINGYO));
    HappouModel = LoadModel(GetArcData(MODEL_HAPPOU));

    for (i = 0; i < MAX_ITEMS; i++)
    {
        items[i].locate = LoadModel((u_long *)0);
        items[i].proc = 0;
    }

    i = 0;
    attr = GS_ATTR_SEMITRANS_ADD;
    while (1)
    {
        if (i >= 1)
            break;
        InitSprite(GetImage(IMG_SIGHT), &TargetSprite[i]);
        TargetSprite[i].attribute = attr;
        i++;
    }

    sprNapalm = SetupSprite((Sprite3D *)0, GetImage(IMG_BOMB0));
    sprNapalm->sprite.attribute = GS_ATTR_SEMITRANS_ADD;
    sprNapalm2 = SetupSprite((Sprite3D *)0, GetImage(IMG_SMOKE));
    sprNapalm2->sprite.attribute = GS_ATTR_SEMITRANS_SUBTRACT;
    InitSprite(GetImage(IMG_GOSHIKIMAI), &SpriteGoshikimai);

    fInitial = 1;
}

extern Humanoid *NINKEN_CHARACTER_PTR;

void create_ninken_character_(s16 type, s32 stage)
{
    NINKEN_CHARACTER_PTR = BreedLife(NINKEN, NINKEN_PARK_POS, NINKEN_PARK_POS, NINKEN_PARK_POS, 0);
    NINKEN_CHARACTER_PTR->attribute |= ATTR_SUSPEND;

    {
        ModelArchiveType *model;
        HenshinModelSnapshot *saved;

        model = CamState.Owner->model;
        saved = &Item_save;
        CaptureHenshinModel(saved, model);
    }

    {
        Humanoid *human;
        ModelArchiveType *model;
        HenshinModelSnapshot *saved;
        s32 flag;

        flag = (type == AYAME_0);
        human = BreedLife(HensinT[(s16)stage][flag],
                          NINKEN_PARK_POS, NINKEN_PARK_POS, NINKEN_PARK_POS, 0);
        model = human->model;
        saved = &HenshinSnapshot;
        CaptureHenshinModel(saved, model);
        KillHumanoid(human);
    }
}


void draw_map_items_(s32 x, s32 z, MapPlacementType *placement)
{
    s32 divisor;
    s32 sine;
    s32 first_draw_arg_x;
    s32 loop_draw_arg_x;
    s32 cosine;
    s32 draw_x;
    s32 draw_y;
    s32 i;

    divisor = placement->scale_divisor;
    if (divisor <= 0)
    {
        divisor = 1;
    }

    sine = rsin(placement->rotation);
    cosine = rcos(placement->rotation);

    draw_x = (x / divisor) * cosine + (z / divisor) * sine;
    if (draw_x < 0)
    {
        draw_x += FIXED_TRUNC_BIAS;
    }
    first_draw_arg_x = (draw_x >> FIXED_SHIFT) + placement->screen_x;
    draw_y = (x / divisor) * sine - (z / divisor) * cosine;
    if (draw_y < 0)
    {
        draw_y += FIXED_TRUNC_BIAS;
    }
    DrawTargetS(first_draw_arg_x,
                (draw_y >> FIXED_SHIFT) + placement->screen_y, 0,
                RGB24(200, 20, 20));

    i = 0;
    while (1)
    {
        if (i >= MAX_ITEMS)
        {
            break;
        }

        if (items[i].proc != 0 && items[i].type == ITEM_GOSHIKIMAI && items[i].owner == CamState.Owner)
        {
            draw_x = (items[i].locate->locate.coord.t[0] / divisor) * cosine +
                     (items[i].locate->locate.coord.t[2] / divisor) * sine;
            if (draw_x < 0)
            {
                draw_x += FIXED_TRUNC_BIAS;
            }
            draw_y = (items[i].locate->locate.coord.t[0] / divisor) * sine -
                     (items[i].locate->locate.coord.t[2] / divisor) * cosine;
            loop_draw_arg_x =
                (draw_x >> FIXED_SHIFT) + placement->screen_x;
            if (draw_y < 0)
            {
                draw_y += FIXED_TRUNC_BIAS;
            }
            DrawTargetS(loop_draw_arg_x,
                        (draw_y >> FIXED_SHIFT) + placement->screen_y, 0,
                        RGB24(20, 20, 200));
        }

        i++;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void MoveKorogari(struct tag_TItem *item, struct param_korogari *param);
 *     ITEM.C:663, 63 src lines, frame 64 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct tag_TItem * item
 *     param $s2       struct param_korogari * param
 *     stack sp+24     struct MapVector mv
 *     stack sp+40     struct SVECTOR vec
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern short RefrectMove[16][2];
 * END PSX.SYM */

static void MoveKorogari(TItem *item, param_korogari *param)
{
    MapVector mv;
    SVECTOR vec;
    s32 level;

    if (param->status == KORO_STAY)
    {
        return;
    }

    item->locate->locate.coord.t[0] += param->vx;
    item->locate->locate.coord.t[1] += param->vy;
    item->locate->locate.coord.t[2] += param->vz;

    level = CGetLevel(&param->hint,
                      item->locate->locate.coord.t[0],
                      item->locate->locate.coord.t[1],
                      item->locate->locate.coord.t[2], 0);
    if (level < item->locate->locate.coord.t[1])
    {
        item->locate->locate.coord.t[0] -= param->vx;
        item->locate->locate.coord.t[1] -= param->vy;
        item->locate->locate.coord.t[2] -= param->vz;

        GetAreaMapVector(GlobalAreaMap, &mv,
                         MODEL_POSITION(item->locate), 500,
                         AREA_LEVEL_DEFAULT);
        if (param->hint == 0)
        {
            level = CGetLevel(&param->hint,
                              item->locate->locate.coord.t[0],
                              item->locate->locate.coord.t[1],
                              item->locate->locate.coord.t[2], 0);
            if (level == LEVEL_NONE)
            {
                if (param->status == KORO_OUT)
                {
                    param->vx = rand() % 1600 - 800;
                    param->vy = rand() % 1600 - 800;
                    param->vz = rand() % 1600 - 800;
                }
                else
                {
                    setVector(param, 0, 250, 0);
                }
                param->status = KORO_OUT;
                return;
            }
        }

        if (mv.vector != 0 && mv.height > 500)
        {
            param->vx = RefrectMove[mv.vector][0] * (abs(param->vx) / 4);
            param->vy /= 2;
            param->vz = RefrectMove[mv.vector][1] * (abs(param->vz) / 4);
            param->status = KORO_WALL;
            return;
        }

        if (mv.attrib & MAP_WATER)
        {
            param->vx = rand() % 20 - 10;
            param->vz = rand() % 20 - 10;
            if (param->vy > 20)
            {
                /* Retail retains this dead copy; the demo symbols suggest an older
                 * SetSplash accepted the direction. */
                vec = (SVECTOR){
                    .vx = 0,
                    .vy = -20,
                    .vz = 0
                };
                SetSplash(MODEL_POSITION(item->locate),
                          2 * FIXED_ONE, 2 * FIXED_ONE, 4);
                param->status = KORO_WATER;
            }
            param->vy = -param->vy / 8;
            return;
        }
        else
        {
            param->vx /= 2;
            param->vz /= 2;
            if (mv.level != LEVEL_NONE && mv.height <= 1500)
            {
                item->locate->locate.coord.t[1] = mv.level;
                if (param->vy < 46)
                {
                    param->status = KORO_STAY;
                    return;
                }

                param->vx += RefrectMove[mv.vector][0] * (rand() % 25 + 25);
                param->vz += RefrectMove[mv.vector][1] * (rand() % 25 + 25);
                param->status = KORO_GRAND;
                param->vy = -abs(param->vy) / 2;
                return;
            }
        }
        param->vy = abs(param->vy) / 2 + (rand() % 25 + 25);
        param->status = KORO_WALL;
        return;
    }
    param->status = KORO_NORMAL;
    param->vy += 15;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void MoveFly(struct tag_TItem *item, struct param_fly *param);
 *     ITEM.C:742, 43 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $t4       struct tag_TItem * item
 *     param $a2       struct param_fly * param
 *     reg   $t3       long x
 *     reg   $t0       long y
 *     reg   $a3       long z
 *     reg   $v0       long t
 *     reg   $a0       long Q
 *     reg   $a1       long R
 *     reg   $a2       struct param_korogari * param
 * END PSX.SYM */

static void MoveFly(TItem *item, param_fly *param)
{
    s32 x, y, z, q, q2, w9, w8, d2, k, nv;
    s32 xs, ys, zs;
    s32 t, ax, ay, az;

    switch (param->mode)
    {
    case FLY_MODE_ARC:
    {
        t = param->p.fly.count;
        k = FIXED_ONE;
        q = k - (t << FIXED_SHIFT) / param->p.fly.count2;
        q2 = q * q;
        d2 = q * 2;
        if (q2 < 0)
            q2 += FIXED_TRUNC_BIAS;
        q2 = q2 >> FIXED_SHIFT;
        nv = q2;
        w9 = k - d2 + nv;
        w8 = d2 + nv * -2;
        x = w9 * param->p.fly.sx + w8 * param->p.fly.rx + nv * param->p.fly.vx;
        xs = x / FIXED_ONE;
        y = w9 * param->p.fly.sy + w8 * param->p.fly.ry + q2 * param->p.fly.vy;
        ys = y / FIXED_ONE;
        z = w9 * param->p.fly.sz + w8 * param->p.fly.rz + nv * param->p.fly.vz;
        zs = z / FIXED_ONE;
        if (t == 0)
        {
            ax = item->locate->locate.coord.t[0];
            ay = item->locate->locate.coord.t[1];
            az = item->locate->locate.coord.t[2];
            param->p.koro.hint = 0;
            param->p.koro.status = KORO_NORMAL;
            param->mode = FLY_MODE_ROLL;
            setVector(&param->p.koro, xs - ax, ys - ay, zs - az);
        }
        else
        {
            param->p.fly.count--;
        }
        item->locate->locate.coord.t[0] = xs;
        item->locate->locate.coord.t[1] = ys;
        item->locate->locate.coord.t[2] = zs;
        return;
    }
    case FLY_MODE_ROLL:
    {
        MoveKorogari(item, &param->p.koro);
        break;
    }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void SetupFly(struct param_fly *pfly, struct VECTOR *start, struct VECTOR *end, int yw, int yh, int time);
 *     ITEM.C:792, 39 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct param_fly * pfly
 *     param $a1       struct VECTOR * start
 *     param $a2       struct VECTOR * end
 *     param $s5       int yw
 *     param stack+16  int yh
 *     param stack+20  int time
 *     reg   $s6       int yh
 *     reg   $s0       int time
 *     reg   $a0       long len
 *     reg   $s1       struct tag_fly * fly
 * END PSX.SYM */

static inline long SubFlyJitter(long mid, long half, long range)
{
    if (range > 0)
    {
        return mid - (rand() % range + half);
    }
    return mid - half;
}

static void SetupFly(param_fly *pfly, VECTOR *start, VECTOR *end, s32 yw, s32 yh, s32 time)
{
    long len;
    long horizontal_span;
    long mid_x;
    long mid_z;
    long control_z;
    long scaled_horizontal_spread;
    struct tag_fly *fly;

    fly = &pfly->p.fly;
    pfly->mode = FLY_MODE_ARC;
    fly->sx = start->vx;
    fly->sy = start->vy;
    fly->sz = start->vz;
    copyVector(fly, end);
    len = GetVectorDistance(start, end);
    if (time > 0)
    {
        fly->count = len / time;
        if (fly->count == 0)
        {
            fly->count = 1;
        }
    }
    else
    {
        fly->count = 1;
    }
    /* These biased shifts implement signed division with truncation toward zero. */
    scaled_horizontal_spread = len * (yw / 2);
    fly->count2 = fly->count;
    if (scaled_horizontal_spread < 0)
    {
        scaled_horizontal_spread += FIXED_TRUNC_BIAS;
    }
    len = len * (yh / 2);
    yw = scaled_horizontal_spread >> FIXED_SHIFT;
    if (len < 0)
    {
        len += FIXED_TRUNC_BIAS;
    }
    yh = len >> FIXED_SHIFT;
    mid_x = (fly->sx + fly->vx) / 2;
    horizontal_span = yw << 1;
    if (horizontal_span > 0)
    {
        len = mid_x + (rand() % horizontal_span - yw);
    }
    else
    {
        len = mid_x - yw;
    }
    fly->rx = len;
    len = SubFlyJitter((fly->sy + fly->vy) / 2, yh / 2,
                       yh - yh / 2);
    mid_z = (fly->sz + fly->vz) / 2;
    horizontal_span = yw << 1;
    fly->ry = len;
    if (horizontal_span > 0)
    {
        control_z = mid_z + (rand() % horizontal_span - yw);
    }
    else
    {
        control_z = mid_z - yw;
    }
    fly->rz = control_z;
    fly->count--;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemDrop(struct tag_TItem *item);
 *     ITEM.C:835, 68 src lines, frame 40 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct tag_TItem * item
 *     reg   $s4       struct param_drop * param
 *     reg   $a0       int cid
 *     reg   $t1       struct Sprite3D * model
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s3       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       struct ModelType * model
 *     reg   $s0       struct Humanoid * human
 *     reg   $s0       struct Humanoid * human
 *     reg   $s0       struct Humanoid * human
 *     reg   $s4       struct param_korogari * param
 *     reg   $s1       int x
 *     reg   $s0       int y
 *     reg   $v0       int z
 *     reg   $s3       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern short ActionHalt;
 * END PSX.SYM */

void ProcItemDrop(TItem *item)
{
    enum
    {
        DROP_MODE_ROLL = 0,
        DROP_MODE_WAIT = 1,
        DROP_MODE_TRANSFER = 2,
        DROP_PICKUP_RADIUS = 180,
        DROP_HORIZONTAL_SPREAD = 200,
        DROP_VERTICAL_SPREAD = 100,
        DROP_VERTICAL_BASE = -200,
        DROP_TRANSFER_DELAY = 10
    };
    Sprite3D *model;
    param_drop *param;
    Humanoid *human;
    MotionDataType *motion_data;
    s32 conflict_result;
    s32 conflict_id;
    ConflictClass collision_mode;
    s32 random_x;
    s32 random_y;
    s32 random_z;
    u8 inventory_count;

    model = (Sprite3D *)item->model;
    param = &item->param.drop;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = DROP_MODE_ROLL;
        return;
    }
    model->locate = item->locate->locate;
    DrawSprite(model);
    switch (item->mode)
    {
    case DROP_MODE_ROLL:
        MoveKorogari(item, &param->koro);
        switch (param->koro.status)
        {
        case KORO_WATER:
            if (item->proc == 0)
                return;
            DISPOSE_ITEM(item);
            return;
        case KORO_GRAND:
        case KORO_STAY:
            DeleteConflict(item->locate);
            conflict_id = InsertConflict(item->locate);
            collision_mode = CONFLICT_SOFT;
            SET_ITEM_COLLISION(conflict_id, DROP_PICKUP_RADIUS,
                               CONFLICT_OWNER_ITEM,
                               collision_mode);
            item->mode++;
            return;
        }
        return;

    case DROP_MODE_WAIT:
        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
            conflict_result = CONFLICT_NONE;
        else
            conflict_result = GetConflictResult(item->locate, CONFLICT_NONE);
        if (conflict_result == CONFLICT_NONE)
            return;
        human = ConflictObject[conflict_result].common;
        if (is_humanoid_on_stage_(human) == 0)
            return;
        if (human->motion->mid == MOT_STATE_PICKUP)
            return;
        if ((human->status != STAT_CHASE) && (human->status != STAT_MOVE))
            return;
        if (ActionHalt == ACTION_HALT_NONE && human->life > 0)
        {
            dispose_weapon_data_of_char_(human, ATTACK_CANCEL_ALL);
            UpdateMotion(human->motion, MOT_STATE_PICKUP);
            human->status = STAT_STATE;
            motion_data = human->motion->motion;
            MoveHumanoid(human, motion_data->orderspd, motion_data->sidespd);
        }
        item->owner = human;
        item->mode++;
        param->count = 0;
        return;

    case DROP_MODE_TRANSFER:
        if (item->owner->motion->mid != MOT_STATE_PICKUP)
        {
            random_x = rand();
            random_x %= DROP_HORIZONTAL_SPREAD;
            random_y = rand();
            random_y %= DROP_VERTICAL_SPREAD;
            random_z = rand();
            random_z %= DROP_HORIZONTAL_SPREAD;
            param->koro.vx = random_x - DROP_HORIZONTAL_SPREAD / 2;
            param->koro.vy = random_y + DROP_VERTICAL_BASE;
            param->koro.hint = 0;
            param->koro.status = KORO_NORMAL;
            param->koro.vz = random_z - DROP_HORIZONTAL_SPREAD / 2;
            item->mode = DROP_MODE_ROLL;
        }
        if (++param->count == DROP_TRANSFER_DELAY)
        {
            SoundEx(item->owner->locate, SE_ITEM_TRANSFER);
            inventory_count = item->owner->item[item->type];
            if (inventory_count != ITEM_INFINITE)
            {
                item->owner->item[item->type] = inventory_count + 1;
            }
            if (item->proc == 0)
                return;
            DISPOSE_ITEM(item);
        }
        return;
    }
}

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

int ReqItemDrop(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    param_drop *param;

    item = TakeItemSlot();
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

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemDrop);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcSightShot(struct tag_TItem *item);
 *     ITEM.C:939, 83 src lines, frame 128 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct tag_TItem * item
 *     reg   $v1       struct param_launch * param
 *     reg   $s1       struct tag_TItem * item
 *     reg   $a1       struct ModelType * model
 *     stack sp+16     struct PARAM_ITEM_LAUNCH param
 *     stack sp+56     struct SVECTOR rot
 *     stack sp+64     int rx
 *     stack sp+68     int ry
 *     stack sp+72     struct PARAM_ITEM_LAUNCH param
 *     reg   $s1       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE TargetSprite[1];
 *     extern struct GsOT *OTablePt;
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */



void ProcSightShot(TItem *item)
{
    param_launch *launch;
    s32 dispose_mode;
    PARAM_ITEM_LAUNCH param;
    SVECTOR rot;
    int rx;
    int ry;
    Humanoid *human;

    launch = &item->param.launch;
    dispose_mode = ITEM_MODE_DISPOSE;
    if (item->mode == dispose_mode)
    {
        item->owner->item[ITEM_N] = 0;
        item->mode = ITEM_MODE_START;
        return;
    }

    human = item->owner;
    if (human->item[ITEM_N] == 0)
    {
        u8 item_count;

        item_count = human->item[item->type];
        if (item_count != ITEM_INFINITE)
        {
            human->item[item->type] = item_count + 1;
        }
        SetCameraMode(CMODE_DIRECTION);
        if (item->proc != 0)
        {
            DISPOSE_ITEM_WITH_MODE(item, dispose_mode);
        }
        return;
    }

    if (human->motion->mid != MOT_SYURI)
    {
        VECTOR *pos;
        Humanoid *drop_owner;
        s32 itemID;

        pos = GetAbsolutePosition(item->locate, 0, 0, 0);
        drop_owner = item->owner;
        itemID = item->type;
        param = (PARAM_ITEM_LAUNCH){0};
        param.type = itemID;
        param.user = drop_owner;
        param.start.vx = pos->vx;
        param.start.vy = pos->vy;
        param.start.vz = pos->vz;
        param.end.vx = rand() % 200 - 100;
        param.end.vy = rand() % 100 - 200;
        param.end.vz = rand() % 200 - 100;
        ReqItemDrop(&param);

        if (item->proc != 0)
        {
            DISPOSE_ITEM_WITH_MODE(item, dispose_mode);
        }
        return;
    }

{
    u8 count;
    ModelArchiveType *model;

    count = launch->count;
    if (count != 0)
    {
        launch->count--;
    }
    if ((item->owner->pad.data & PADRup) != 0)
    {
        if (launch->count != 0)
        {
            return;
        }
        SetCameraMode(CMODE_SIGHT);
        GsSortSprite(TargetSprite, OTablePt, 0);
        return;
    }

    count = launch->count;
    model = item->owner->model;
    if (count == 0)
    {
        GsRVIEW2 *view;

        param.type = item->type;
        param.user = item->owner;
        param.start.vx = item->locate->locate.coord.t[0];
        param.start.vy = item->locate->locate.coord.t[1];
        param.start.vz = item->locate->locate.coord.t[2];
        view = &ViewInfo;
        GetVectorRotation(CAMERA_VIEWPOINT(view), CAMERA_REFERENCE(view),
                          &rx, &ry);
        rot.vz = 0;
        rot.vx = rx;
        rot.vy = ry;
        SearchItemTarget2(param.user, &rot, CAMERA_VIEWPOINT(view),
                          &param.end);
        if (item->proc != 0)
        {
            DISPOSE_ITEM_WITH_MODE(item, dispose_mode);
        }
        SetCameraMode(CMODE_LOCK);
    }
    else
    {
        param.type = item->type;
        param.user = item->owner;
        param.start.vx = item->locate->locate.coord.t[0];
        param.start.vy = item->locate->locate.coord.t[1];
        param.start.vz = item->locate->locate.coord.t[2];
        SearchItemTarget2(param.user, &model->rotate, &param.start,
                          &param.end);
        if (item->proc != 0)
        {
            DISPOSE_ITEM_WITH_MODE(item, dispose_mode);
        }
    }
    ReqItemLaunch(&param);
}
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcKaginawa(struct tag_TItem *item);
 *     ITEM.C:1026, 67 src lines, frame 40 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct tag_TItem * item
 *     stack sp+16     int rx
 *     stack sp+20     int ry
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v0       int dist
 *     reg   $s0       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct GsSPRITE TargetSprite[1];
 *     extern struct GsOT *OTablePt;
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */



extern VECTOR vec_z_n20000; /* {0,0,-20000} */

void ProcKaginawa(TItem *item)
{
    enum
    {
        KAGINAWA_TARGET_OFFSET_DIVISOR = 16,
        KAGINAWA_MAX_LOCK_DISTANCE = 15000
    };
    Humanoid *owner;
    VECTOR v;
    VECTOR w;
    s32 rx, ry;
    s32 dist;
    item_mode dispose_mode;

    dispose_mode = ITEM_MODE_DISPOSE;
    if (item->mode == dispose_mode)
    {
        item->mode = ITEM_MODE_START;
        return;
    }
    owner = item->owner;
    if (owner->item[ITEM_N] == 0)
    {
        SetCameraMode(CMODE_DIRECTION);
        if (item->proc == 0)
            return;
        DISPOSE_ITEM_WITH_MODE(item, dispose_mode);
    }
    else if (owner->motion->mid != MOT_KAGI)
    {
        if (item->proc == 0)
            return;
        DISPOSE_ITEM_WITH_MODE(item, dispose_mode);
    }
    else
    {
        GetVectorRotation(CAMERA_VIEWPOINT(&ViewInfo),
                          CAMERA_REFERENCE(&ViewInfo), &rx, &ry);
        if (item->owner->pad.data & PADRup)
        {
            if (rx < 0)
                GsSortSprite(TargetSprite, OTablePt, 0);
            return;
        }
        v = vec_z_n20000;
        RotateVector(&v, rx, ry, 0);
        copyVector(&w, &v);
        w.vx += ViewInfo.vpx;
        w.vy += ViewInfo.vpy;
        w.vz += ViewInfo.vpz;
        trace_ground_(CAMERA_VIEWPOINT(&ViewInfo), &w,
                      &CamState.TargetVector, 0);
        v.vx /= KAGINAWA_TARGET_OFFSET_DIVISOR;
        v.vy /= KAGINAWA_TARGET_OFFSET_DIVISOR;
        v.vz /= KAGINAWA_TARGET_OFFSET_DIVISOR;
        CamState.TargetVector.vx += v.vx;
        CamState.TargetVector.vy += v.vy;
        CamState.TargetVector.vz += v.vz;
        dist = GetVectorDistance(MODEL_POSITION(CamState.Owner->model),
                                 &CamState.TargetVector);
        if (rx > 0 || dist > KAGINAWA_MAX_LOCK_DISTANCE)
        {
            CamState.TargetVector = *MODEL_POSITION(CamState.Owner->model);
        }
        SetCameraMode(CMODE_LOCK);
        item->owner->item[ITEM_N] = 0;
        if (item->proc == 0)
            return;
        item->mode = dispose_mode;
        item->proc(item);
        DeleteConflict(item->locate);
        if (item->mode != ITEM_MODE_START)
            AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
        item->owner = 0;
        item->proc = 0;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemTeleport(struct tag_TItem *item);
 *     ITEM.C:1097, 39 src lines, frame 48 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE TargetSprite[1];
 *     extern struct GsOT *OTablePt;
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

void ProcItemTeleport(TItem *item)
{
    enum
    {
        TELEPORT_MAX_DISTANCE = 20000,
        TELEPORT_EFFECT_GROUND_RANGE = 1000,
        TELEPORT_EFFECT_SPREAD = 20,
        TELEPORT_EFFECT_PARTICLES = 50,
        TELEPORT_EFFECT_LIFETIME = 60
    };

    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = ITEM_MODE_START;
        return;
    }
    if ((item->owner->pad.data & PADRup) != 0)
    {
        SetCameraMode(CMODE_SIGHT);
        GsSortSprite(TargetSprite, OTablePt, 0);
        return;
    }
    SnapCameraTargetVector();
    if (GetVectorDistance(MODEL_POSITION(CamState.Owner->model),
                          &CamState.TargetVector) < TELEPORT_MAX_DISTANCE)
    {
        copyVector(MODEL_POSITION(CamState.Owner->model),
                   &CamState.TargetVector);
        SetBleeds(&CamState.TargetVector, TELEPORT_EFFECT_GROUND_RANGE,
                  TELEPORT_EFFECT_SPREAD, TELEPORT_EFFECT_PARTICLES,
                  TELEPORT_EFFECT_LIFETIME, COLOR_WHITE);
    }
    SetCameraMode(CMODE_NORMAL);
    if (item->proc == 0)
        return;
    DISPOSE_ITEM(item);
}

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

void ProcItemMakibishi(TItem *item)
{
    enum
    {
        MAKIBISHI_MODE_ROLL = 0,
        MAKIBISHI_MODE_ARMED = 1,
        MAKIBISHI_TRIGGER_RADIUS = 100,
        MAKIBISHI_BLOOD_SPREAD = 20,
        MAKIBISHI_BLOOD_PARTICLES = 10,
        MAKIBISHI_BLOOD_LIFETIME = 15,
        MAKIBISHI_BLOOD_COLOR = RGB24(127, 0, 0)
    };
    Sprite3D *model;
    param_drop *param;
    s32 i;
    s32 conflict_id;

    model = (Sprite3D *)item->model;
    param = &item->param.drop;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = MAKIBISHI_MODE_ROLL;
        return;
    }
    switch (item->mode)
    {
    case MAKIBISHI_MODE_ROLL:
        MoveKorogari(item, &param->koro);
        switch (param->koro.status)
        {
        case KORO_STAY:
            item->mode++;
            DeleteConflict(item->locate);
            conflict_id = InsertConflict(item->locate);
            SET_ITEM_COLLISION(conflict_id, MAKIBISHI_TRIGGER_RADIUS,
                               CONFLICT_OWNER_ITEM,
                               CONFLICT_HIT);
            break;

        case KORO_WATER:
            if (item->proc == 0)
                return;
            DISPOSE_ITEM(item);
            return;
        }
        break;

    case MAKIBISHI_MODE_ARMED:
        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
            i = CONFLICT_NONE;
        else
            i = GetConflictResult(item->locate, CONFLICT_NONE);
        if (i != CONFLICT_NONE &&
            is_humanoid_on_stage_(ConflictObject[i].common) != 0)
        {
            SetBleeds(MODEL_POSITION(item->locate), 0,
                      MAKIBISHI_BLOOD_SPREAD, MAKIBISHI_BLOOD_PARTICLES,
                      MAKIBISHI_BLOOD_LIFETIME, MAKIBISHI_BLOOD_COLOR);
            SoundEx(MODEL_POSITION(item->locate), SE_PROJECTILE_HIT);
            if (item->proc == 0)
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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemMakibishi(struct PARAM_ITEM_DROP *p);
 *     ITEM.C:1235, 17 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct PARAM_ITEM_DROP * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_drop * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       int atype
 *     reg   $a0       struct VECTOR * pos
 *     reg   $s1       struct param_korogari * param
 *     reg   $v0       int x
 *     reg   $v1       int y
 *     reg   $a0       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

int ReqItemMakibishi(PARAM_ITEM_DROP *p)
{
    TItem *item;
    TItem *ret;
    param_drop *param;
    VECTOR *pos;
    s32 x;
    s32 y;
    s32 z;
    s32 i;

    TAKE_ITEM_SLOT_VIA_CURSOR(found);
found:
    param = &item->param.drop;
    if (item == 0)
        return 0;
    {
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemMakibishi);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
    }
    x = p->vec.vx;
    y = p->vec.vy;
    z = p->vec.vz;
    param->koro.vx = x;
    param->koro.vy = y;
    param->koro.vz = z;
    item->param.drop.koro.hint = 0;
    param->koro.status = KORO_NORMAL;
    SoundEx(pos, SE_CALTROP_THROW);
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemManebue(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:1290, 12 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

static void ProcItemManebue(TItem *item);

static int ReqItemManebue(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;

    item = TakeItemSlot();
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemManebue);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
    }
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemShinsoku(struct tag_TItem *item);
 *     ITEM.C:1324, 55 src lines, frame 104 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct tag_TItem * item
 *     reg   $s1       struct param_shinsoku * param
 *     stack sp+24     struct VECTOR pos
 *     reg   $s2       struct Humanoid * human
 *     reg   $s0       int itemID
 *     stack sp+40     struct PARAM_ITEM_LAUNCH p
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s0       struct VECTOR * apos
 *     stack sp+56     struct MapVector map
 *     stack sp+40     struct VECTOR pos
 *     reg   $s3       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */


void ProcItemShinsoku(TItem *item)
{
    enum
    {
        SHINSOKU_MODE_START = 0,
        SHINSOKU_MODE_WAIT = 1,
        SHINSOKU_MODE_ACTIVE = 2,
        INTERRUPTED_DROP_HORIZONTAL_RADIUS = 100,
        INTERRUPTED_DROP_VERTICAL_RANGE = 100,
        INTERRUPTED_DROP_VERTICAL_BASE = -200,
        SHINSOKU_SMOKE_RADIUS = 150,
        SHINSOKU_SMOKE_COUNT = 8,
        SHINSOKU_GROUND_PROBE_DEPTH = 2000,
        SHINSOKU_GROUND_PROBE_RADIUS = 500,
        SHINSOKU_MAX_STEP_DOWN = 500,
        SHINSOKU_TRAIL_PERIOD = 4,
        SHINSOKU_TRAIL_HEIGHT = 300,
        SHINSOKU_TRAIL_START_SIZE = 2 * FIXED_ONE,
        SHINSOKU_TRAIL_END_SIZE = 5 * FIXED_ONE,
        SHINSOKU_TRAIL_ROTATE_SPEED = -30,
        SHINSOKU_TRAIL_FRAMES = 16,
        SHINSOKU_TURN_STEP = ANGLE_FULL / 64
    };
    param_shinsoku *param;
    VECTOR pos;

    param = &item->param.shinsoku;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = SHINSOKU_MODE_START;
        return;
    }

    switch (item->mode)
    {
    case SHINSOKU_MODE_START:
        SetNowMotion(item->owner, MOT_ITEM_SHINSOKU, MOTION_MOVE_APPLY);
        Sound(item->owner, SE_ITEM_USE);
        item->mode++;
        return;

    case SHINSOKU_MODE_WAIT:
    {
        MotionManager *motion;

        motion = item->owner->motion;
        if (motion->mid != MOT_ITEM_SHINSOKU)
        {
            VECTOR *pos;
            Humanoid *human;
            s32 itemID;
            s32 rand_x;
            s32 rand_y;
            s32 rand_z;

            pos = GetAbsolutePosition(item->locate, 0, 0, 0);
            human = item->owner;
            itemID = item->type;
            {
                PARAM_ITEM_LAUNCH drop_request;

                ClearItemLaunchRequest(&drop_request);
                drop_request.type = itemID;
                drop_request.user = human;
                drop_request.start.vx = pos->vx;
                drop_request.start.vy = pos->vy;
                drop_request.start.vz = pos->vz;
                rand_x = rand();
                drop_request.end.vx =
                    rand_x % (INTERRUPTED_DROP_HORIZONTAL_RADIUS * 2) -
                    INTERRUPTED_DROP_HORIZONTAL_RADIUS;
                rand_y = rand();
                drop_request.end.vy =
                    rand_y % INTERRUPTED_DROP_VERTICAL_RANGE +
                    INTERRUPTED_DROP_VERTICAL_BASE;
                rand_z = rand();
                drop_request.end.vz =
                    rand_z % (INTERRUPTED_DROP_HORIZONTAL_RADIUS * 2) -
                    INTERRUPTED_DROP_HORIZONTAL_RADIUS;
                ReqItemDrop(&drop_request);
            }
            if (item->proc == 0)
            {
                return;
            }
            DISPOSE_ITEM(item);
            return;
        }
        if (motion->count != 0)
        {
            return;
        }
        if (motion->loop == 0)
        {
            return;
        }
        spawn_smoke_burst_(item->owner->locate, SHINSOKU_SMOKE_RADIUS,
                           SMOKE_DRIFT_DIVISOR_DEFAULT,
                           SHINSOKU_SMOKE_COUNT);
        param->count = SHINSOKU_DURATION;
        item->mode++;
        return;
    }

    case SHINSOKU_MODE_ACTIVE:
    {
        Humanoid *human;
        ModelArchiveType *model;
        s32 valid;
        u16 buttons;
        s32 turn_step;
        VECTOR *apos;

        if (item->owner->motion->mid != MOT_ITEM_SHINSOKU)
        {
            if (item->proc == 0)
            {
                return;
            }
            DISPOSE_ITEM(item);
            return;
        }

        pos.vx = item->owner->model->locate.coord.t[0];
        pos.vy = item->owner->model->locate.coord.t[1];
        pos.vz = item->owner->model->locate.coord.t[2];
        pos.vx += param->vec.vx;
        pos.vy += param->vec.vy;
        pos.vz += param->vec.vz;
        apos = &pos;
        {
            VECTOR query_position;
            MapVector map;

            query_position.vx = apos->vx;
            query_position.vy = apos->vy;
            query_position.vz = apos->vz;
            query_position.vy -= SHINSOKU_GROUND_PROBE_DEPTH;
            GetAreaMapVector(GlobalAreaMap,
                             &map,
                             &query_position, SHINSOKU_GROUND_PROBE_RADIUS,
                             AREA_LEVEL_DEFAULT);
            if (map.level >= apos->vy - SHINSOKU_MAX_STEP_DOWN)
            {
                if (map.level < apos->vy)
                {
                    apos->vy = map.level;
                }
                valid = 1;
            }
            else
            {
                valid = 0;
            }
            if (valid != 0)
            {
                item->owner->model->locate.coord.t[0] = pos.vx;
                item->owner->model->locate.coord.t[1] = pos.vy;
                item->owner->model->locate.coord.t[2] = pos.vz;
            }

            if ((param->count & (SHINSOKU_TRAIL_PERIOD - 1)) == 0)
            {
                query_position = *MODEL_POSITION(item->owner->model);
                query_position.vy -= SHINSOKU_TRAIL_HEIGHT;
                set_impact_ex_(&query_position, 0,
                               SHINSOKU_TRAIL_START_SIZE,
                               SHINSOKU_TRAIL_END_SIZE,
                               COLOR_GRAY, 0, 0,
                               SHINSOKU_TRAIL_ROTATE_SPEED,
                               SHINSOKU_TRAIL_FRAMES,
                               IMPACT_SPRITE_SHINSOKU);
            }
        }
        if (CamState.Owner == item->owner)
        {
            SetCameraMode(CMODE_CROUCH);
        }

        human = item->owner;
        buttons = human->pad.data;
        if ((buttons & PADLright) != 0)
        {
            model = human->model;
            turn_step = SHINSOKU_TURN_STEP;
            model->rotate.vy += turn_step;
            RotateVectorS(&param->vec, 0, turn_step, 0);
        }
        else if ((buttons & PADLleft) != 0)
        {
            model = human->model;
            turn_step = -SHINSOKU_TURN_STEP;
            model->rotate.vy += turn_step;
            RotateVectorS(&param->vec, 0, turn_step, 0);
        }

        param->count--;
        if (param->count != 0 && (item->owner->pad.trig & (PADRleft | PADRdown | PADRright | PADRup)) == 0)
        {
            return;
        }
        NowReturnNormal(item->owner);
        SetCameraMode(CMODE_NORMAL);
        if (item->proc == 0)
        {
            return;
        }
        DISPOSE_ITEM(item);
        return;
    }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemShinsoku(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:1383, 13 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s2       struct param_shinsoku * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

static int ReqItemShinsoku(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    param_shinsoku *param;

    item = TakeItemSlot();
    param = &item->param.shinsoku;
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemShinsoku);
        item->collision.size = 0;
        item->model = 0;
    }
    param->vec.vx = p->end.vx;
    param->vec.vy = p->end.vy;
    param->vec.vz = p->end.vz;
    return 0;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemSmoke(struct tag_TItem *item);
 *     ITEM.C:1400, 59 src lines, frame 112 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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


void ProcItemSmoke(TItem *item)
{
    enum
    {
        SMOKE_MODE_FUSE = 0,
        SMOKE_MODE_ACTIVE = 1,
        SMOKE_TRAIL_INTERVAL = 2,
        SMOKE_TRAIL_RISE_SPEED = 250,
        SMOKE_TRAIL_PARTICLES = 1,
        SMOKE_TRAIL_LIFETIME = 3,
        SMOKE_CHOKE_CHECK_INTERVAL = 16,
        SMOKE_CHOKE_RADIUS = 2000
    };
    Sprite3D *model;
    param_smoke *param;

    model = (Sprite3D *)item->model;
    param = &item->param.smoke;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = SMOKE_MODE_FUSE;
        return;
    }
    MoveKorogari(item, &param->koro);
    if (param->koro.status == KORO_WATER)
    {
        if (item->proc == 0)
            return;
        item->mode = ITEM_MODE_DISPOSE;
        item->proc(item);
        DeleteConflict(item->locate);
        if (item->mode != SMOKE_MODE_FUSE)
        {
            AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
        }
        item->owner = 0;
        item->proc = 0;
        return;
    }
    UpdateCoordinate(item->locate);
    model->locate = item->locate->locate;
    DrawSprite(model);
    param->count--;
    switch (item->mode)
    {
    case SMOKE_MODE_FUSE:
        if (param->count != 0)
            return;
        SoundEx(MODEL_POSITION(item->locate), SE_SMOKE_PUFF);
        param->count = SMOKE_DURATION;
        item->mode++;
        return;

    case SMOKE_MODE_ACTIVE:
        if (param->count == 0)
        {
            if (item->proc == 0)
                return;
            DISPOSE_ITEM(item);
            return;
        }
        if ((param->count & (SMOKE_TRAIL_INTERVAL - 1)) == 0)
        {
            {
                SVECTOR vec = {
                    .vx = 0,
                    .vy = -SMOKE_TRAIL_RISE_SPEED,
                    .vz = 0
                };
                VECTOR pos = {
                    .vx = item->locate->locate.coord.t[0],
                    .vy = item->locate->locate.coord.t[1],
                    .vz = item->locate->locate.coord.t[2]
                };

                SetSmoke(&pos, &vec, SMOKE_TRAIL_PARTICLES,
                         SMOKE_TRAIL_LIFETIME);
            }
        }
        if ((GameClock & (SMOKE_CHOKE_CHECK_INTERVAL - 1)) != 0)
            return;
        {
            TFindItemTarget search_state;
            TFindItemTarget *q;
            TFindItemTarget *find;
            VECTOR *pos;
            character_status status;
            Humanoid *human;

            q = &search_state;
            pos = MODEL_POSITION(item->locate);
            q->i = 0;
            find = &search_state;
            find->pos.vx = pos->vx;
            find->pos.vy = pos->vy;
            find->pos.vz = pos->vz;
            find->find_dist = SMOKE_CHOKE_RADIUS;
            while (FindItemTarget(find) != 0)
            {
                human = search_state.find;
                if (human != item->owner &&
                    human->life != HUMANOID_LIFE_INACTIVE &&
                    human->motion->mid != MOT_DAMAGE_CHOKE)
                {
                    status = STAT_DAMAGE;
                    if (ActionHalt == ACTION_HALT_NONE && human->life > 0)
                    {
                        dispose_weapon_data_of_char_(human,
                                                     ATTACK_CANCEL_ALL);
                        UpdateMotion(human->motion, MOT_DAMAGE_CHOKE);
                        human->status = status;
                        MoveHumanoid(human,
                                     human->motion->motion->orderspd,
                                     human->motion->motion->sidespd);
                    }
                    Sound(search_state.find, CHAR_VOICE_HURT);
                }
            }
            return;
        }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemSmoke(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:1463, 18 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_smoke * param
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
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

int ReqItemSmoke(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    param_smoke *param;
    s32 x;
    s32 y;
    s32 z;

    item = TakeItemSlot();
    param = &item->param.smoke;
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemSmoke);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
    }
    {
        param_korogari *param; /* Shadows the outer launch parameter in retail. */

        param = &item->param.smoke.koro;
        x = p->end.vx;
        y = p->end.vy;
        z = p->end.vz;
        setVector(param, x, y, z);
        item->param.smoke.koro.hint = 0;
        param->status = KORO_NORMAL;
    }
    param->count = 10;
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemKusuri(struct tag_TItem *item);
 *     ITEM.C:1485, 60 src lines, frame 80 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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

void ProcItemKusuri(TItem *item)
{
    enum
    {
        KUSURI_MODE_START = 0,
        KUSURI_MODE_DRINK = 1,
        KUSURI_MODE_HEAL = 2,
        KUSURI_MODEL_HAND_OFFSET = 50,
        KUSURI_MODEL_SCALE = 2 * FIXED_ONE,
        KUSURI_VISIBLE_START_FRAME = 4,
        KUSURI_HEAL_FRAME = 55,
        KUSURI_DROP_HORIZONTAL_SPREAD = 200,
        KUSURI_DROP_VERTICAL_SPREAD = 100,
        KUSURI_DROP_VERTICAL_BASE = -200,
        N_KUSURI_HEAL_PARTICLES = 20,
        KUSURI_PARTICLE_SCATTER = 1000,
        KUSURI_PARTICLE_RADIUS = KUSURI_PARTICLE_SCATTER / 2,
        KUSURI_PARTICLE_Y_OFFSET = 1200,
        KUSURI_PARTICLE_SPEED_RANGE = 10,
        KUSURI_PARTICLE_SPEED_BASE = -30,
        KUSURI_PARTICLE_LIFETIME_RANGE = 16,
        KUSURI_PARTICLE_LIFETIME_MIN = 15,
        KUSURI_PARTICLE_COLOR = RGB24(255, 255, 126)
    };
    Sprite3D *model;
    s32 i;

    model = (Sprite3D *)item->model;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = KUSURI_MODE_START;
        return;
    }
    switch (item->mode)
    {
    case KUSURI_MODE_START:
    {
        Humanoid *human;

        human = item->owner;
        if (ActionHalt == ACTION_HALT_NONE && human->life > 0)
        {
            MotionDataType *md;

            dispose_weapon_data_of_char_(human, ATTACK_CANCEL_ALL);
            UpdateMotion(human->motion, MOT_ITEM_DRINK);
            human->status = STAT_ITEM;
            md = human->motion->motion;
            MoveHumanoid(human, md->orderspd, md->sidespd);
        }
    }
        {
            ModelArchiveType *arc;

            arc = item->owner->model;
            if (arc->n > MODEL_PART_WEAPON_HAND_1)
                item->locate->locate.super =
                    &arc->object[MODEL_PART_WEAPON_HAND_1]->locate;
            else
                item->locate->locate.super =
                    &arc->object[MODEL_PART_HEAD]->locate;
        }
        item->locate->locate.coord.t[0] = 0;
        item->locate->locate.coord.t[1] = KUSURI_MODEL_HAND_OFFSET;
        item->locate->locate.coord.t[2] = 0;
        item->mode++;
        return;

    case KUSURI_MODE_DRINK:
    {
        MotionManager *mot;

        mot = item->owner->motion;
        if (mot->mid != MOT_ITEM_DRINK)
        {
            /* animation interrupted: toss the item back out */
            VECTOR *pos;
            Humanoid *human;
            s32 itemID;

            pos = GetAbsolutePosition(item->locate, 0, 0, 0);
            human = item->owner;
            itemID = item->type;
            {
                PARAM_ITEM_LAUNCH p = {
                    .type = itemID,
                    .user = human
                };

                copyVector(&p.start, pos);
                p.end.vx = rand() % KUSURI_DROP_HORIZONTAL_SPREAD -
                           KUSURI_DROP_HORIZONTAL_SPREAD / 2;
                p.end.vy = rand() % KUSURI_DROP_VERTICAL_SPREAD +
                           KUSURI_DROP_VERTICAL_BASE;
                p.end.vz = rand() % KUSURI_DROP_HORIZONTAL_SPREAD -
                           KUSURI_DROP_HORIZONTAL_SPREAD / 2;
                ReqItemDrop(&p);
            }
            if (item->proc == 0)
                return;
            DISPOSE_ITEM(item);
            return;
        }
        {
            s16 cnt;

            cnt = mot->count;
            if (cnt == KUSURI_HEAL_FRAME)
            {
                item->mode = KUSURI_MODE_HEAL;
                return;
            }
            if (cnt < KUSURI_VISIBLE_START_FRAME)
                return;
        }
    }
        UpdateCoordinate(item->locate);
        model->locate = item->locate->locate;
        model->scale = KUSURI_MODEL_SCALE;
        DrawSprite(model);
        return;

    case KUSURI_MODE_HEAL:
    {
        i = 0;
        item->owner->life = item->owner->lifemax;
        while (1)
        {
            if (i >= N_KUSURI_HEAL_PARTICLES)
                break;
            {
                VECTOR pos = {
                    .vx = item->owner->model->locate.coord.t[0] +
                        (rand() % KUSURI_PARTICLE_SCATTER -
                         KUSURI_PARTICLE_RADIUS),
                    .vy = item->owner->model->locate.coord.t[1] +
                        (rand() % KUSURI_PARTICLE_SCATTER -
                         KUSURI_PARTICLE_Y_OFFSET),
                    .vz = item->owner->model->locate.coord.t[2] +
                        (rand() % KUSURI_PARTICLE_SCATTER -
                         KUSURI_PARTICLE_RADIUS)
                };
                SVECTOR vec = {
                    .vx = 0,
                    .vy = rand() % KUSURI_PARTICLE_SPEED_RANGE +
                        KUSURI_PARTICLE_SPEED_BASE,
                    .vz = 0
                };

                SetBleed(&pos, &vec,
                         rand() % KUSURI_PARTICLE_LIFETIME_RANGE +
                             KUSURI_PARTICLE_LIFETIME_MIN,
                         KUSURI_PARTICLE_COLOR);
            }
            i++;
        }
        SoundEx(item->owner->locate, SE_MEDICINE);
        if (item->proc == 0)
            return;
        DISPOSE_ITEM(item);
    }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemKusuri(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:1549, 18 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

static int ReqItemKusuri(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;

    item = TakeItemSlot();
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemKusuri);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
    }
    return 1;
}

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


void ProcItemKawarimi(TItem *item)
{
    enum
    {
        KAWARIMI_MODE_START = 0,
        KAWARIMI_MODE_BLEED = 1,
        KAWARIMI_MODE_FINISH = 2,
        KAWARIMI_BLEED_FRAMES = 31,
        KAWARIMI_PARTICLE_COUNT = 20,
        KAWARIMI_PARTICLE_SPAN = 1000,
        KAWARIMI_PARTICLE_RADIUS = KAWARIMI_PARTICLE_SPAN / 2,
        KAWARIMI_PARTICLE_Y_OFFSET = 1200,
        KAWARIMI_PARTICLE_UPWARD_BASE = 30,
        KAWARIMI_PARTICLE_UPWARD_RANGE = 10,
        KAWARIMI_PARTICLE_LIFETIME_MIN = 15,
        KAWARIMI_PARTICLE_LIFETIME_RANGE = 16,
        KAWARIMI_PARTICLE_COLOR = RGB24(100, 200, 220)
    };
    param_drop *param;
    s32 particle_index;

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
            if (particle_index >= KAWARIMI_PARTICLE_COUNT)
                break;
            {
                VECTOR position = {
                    .vx = item->owner->model->locate.coord.t[0] +
                        (rand() % KAWARIMI_PARTICLE_SPAN -
                         KAWARIMI_PARTICLE_RADIUS),
                    .vy = item->owner->model->locate.coord.t[1] +
                        (rand() % KAWARIMI_PARTICLE_SPAN -
                         KAWARIMI_PARTICLE_Y_OFFSET),
                    .vz = item->owner->model->locate.coord.t[2] +
                        (rand() % KAWARIMI_PARTICLE_SPAN -
                         KAWARIMI_PARTICLE_RADIUS)
                };
                SVECTOR velocity = {
                    .vx = 0,
                    .vy = rand() % KAWARIMI_PARTICLE_UPWARD_RANGE -
                          KAWARIMI_PARTICLE_UPWARD_BASE,
                    .vz = 0
                };

                SetBleed(&position, &velocity,
                         rand() % KAWARIMI_PARTICLE_LIFETIME_RANGE +
                             KAWARIMI_PARTICLE_LIFETIME_MIN,
                         KAWARIMI_PARTICLE_COLOR);
            }
            particle_index++;
        }
        if (++param->count < KAWARIMI_BLEED_FRAMES)
            return;
        item->mode++;
        return;

    case KAWARIMI_MODE_FINISH:
        if (item->proc == 0)
            return;
        DISPOSE_ITEM(item);
        return;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemKawarimi(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:1610, 18 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

static int ReqItemKawarimi(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;

    item = TakeItemSlot();
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemKawarimi);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
    }
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemDokudango(struct tag_TItem *item);
 *     ITEM.C:1632, 141 src lines, frame 128 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s4       struct tag_TItem * item
 *     reg   $s0       struct Sprite3D * model
 *     reg   $s6       struct param_dokudango * param
 *     reg   $s4       struct tag_TItem * item
 *     stack sp+16     struct TFindItemTarget find
 *     reg   $s5       struct Humanoid * target
 *     reg   $s7       int targetlen
 *     reg   $v0       struct TFindItemTarget * find
 *     reg   $v1       struct VECTOR * pos
 *     reg   $s7       int dist
 *     reg   $s3       struct TFindItemTarget * find
 *     reg   $s1       int i
 *     reg   $s0       struct Humanoid * target
 *     reg   $v0       int dist
 *     stack sp+48     struct PARAM_ITEM_LAUNCH param
 *     reg   $s4       struct tag_TItem * item
 *     reg   $s0       struct Humanoid * human
 *     reg   $v0       struct VECTOR * tv
 *     reg   $s6       struct param_korogari * param
 *     reg   $s1       int x
 *     reg   $s0       int y
 *     reg   $v0       int z
 *     reg   $s1       struct Humanoid * human
 *     reg   $s1       struct Humanoid * human
 *     reg   $s4       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct Humanoid *HumanGroup[32];
 *     extern short Humans;
 *     extern short ActionHalt;
 * END PSX.SYM */


static inline void restore_dokudango_target(TItem *item,
                                             param_dokudango *param)
{
    param_dokudango *restore_param;

    restore_param = param;
    if (is_humanoid_on_stage_(restore_param->eater) != 0 &&
        restore_param->org_think != 0)
    {
        restore_param->eater->think[0] = restore_param->org_think;
        restore_param->eater->target = &item->owner->model->locate;
    }
    restore_param->eater = 0;
}

static inline void apply_dokudango_reaction(Humanoid *human,
                                             motion_id reaction_motion)
{
    if (ActionHalt == ACTION_HALT_NONE && human->life > 0)
    {
        MotionDataType *motion_data;

        dispose_weapon_data_of_char_(human, ATTACK_CANCEL_ALL);
        UpdateMotion(human->motion, reaction_motion);
        human->status = STAT_ITEM;
        motion_data = human->motion->motion;
        MoveHumanoid(human, motion_data->orderspd, motion_data->sidespd);
    }
}

void ProcItemDokudango(TItem *item)
{
    enum
    {
        DOKUDANGO_MODE_ROLL = 0,
        DOKUDANGO_MODE_SEARCH = 1,
        DOKUDANGO_MODE_EAT = 2,
        DOKUDANGO_MODE_POISON = 3,
        DOKUDANGO_SEARCH_INTERVAL = 2,
        DOKUDANGO_PICKUP_RANGE = 500,
        DOKUDANGO_EAT_RANGE = 1000,
        DOKUDANGO_HUMAN_HAND_OFFSET = 50,
        DOKUDANGO_BEAST_HAND_OFFSET = -150,
        DOKUDANGO_ROLL_DELAY = 30,
        DOKUDANGO_DROP_HORIZONTAL_SPREAD = 200,
        DOKUDANGO_DROP_VERTICAL_SPREAD = 100,
        DOKUDANGO_DROP_VERTICAL_BASE = -200,
        DOKUDANGO_EAT_FRAME = 55,
        DOKUDANGO_POISON_DURATION = 600,
        DOKUDANGO_REACTION_PERIOD = 30,
        DOKUDANGO_REACTION_CHANCE = 3
    };
    Sprite3D *model;
    param_dokudango *param;

    model = (Sprite3D *)item->model;
    param = &item->param.dokudango;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        restore_dokudango_target(item, param);
        item->mode = DOKUDANGO_MODE_ROLL;
        return;
    }

    if (item->mode < DOKUDANGO_MODE_EAT)
    {
        MoveKorogari(item, &param->koro);
        if (param->koro.status == KORO_WATER)
        {
            if (item->proc == 0)
            {
                return;
            }
            item->mode = ITEM_MODE_DISPOSE;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != DOKUDANGO_MODE_ROLL)
            {
                AdtMessageBox(msg_item_dispose_fail, item->type,
                              (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
            return;
        }
    }
    if (item->mode < DOKUDANGO_MODE_POISON)
    {
        UpdateCoordinate(item->locate);
        model->locate = item->locate->locate;
        DrawSprite(model);
    }

    switch (item->mode)
    {
    case DOKUDANGO_MODE_ROLL:
    {
        u16 roll_countdown;

        roll_countdown = --param->count;
        if ((s16)roll_countdown > 0)
        {
            return;
        }
        item->mode++;
        return;
    }

    case DOKUDANGO_MODE_SEARCH:
    {
        TFindItemTarget search_state;
        TFindItemTarget *search_setup;
        TFindItemTarget *search;
        VECTOR *item_position;
        Humanoid *nearest_target;
        Humanoid *eater;
        s32 nearest_distance;
        s32 owner_distance;

        if ((GameClock & (DOKUDANGO_SEARCH_INTERVAL - 1)) != 0)
        {
            return;
        }
        nearest_target = 0;
        nearest_distance = DOKUDANGO_RANGE;
        search_setup = &search_state;
        item_position = MODEL_POSITION(item->locate);
        owner_distance = nearest_distance;
        search_setup->i = 0;
        search_setup->pos.vx = item_position->vx;
        search = &search_state;
        search->pos.vy = item_position->vy;
        search->pos.vz = item_position->vz;
        search->find_dist = nearest_distance;

        while (FindItemTarget(search) != 0)
        {
            if ((search_state.find->type & PAGE_MASK) != PAGE_BOSS &&
                search_state.find->life != HUMANOID_LIFE_INACTIVE &&
                search_state.dist < nearest_distance)
            {
                if (search_state.find == item->owner)
                {
                    owner_distance = search_state.dist;
                }
                else
                {
                    nearest_target = search_state.find;
                    nearest_distance = search_state.dist;
                }
            }
        }

        if (owner_distance < DOKUDANGO_PICKUP_RANGE)
        {
            PARAM_ITEM_LAUNCH drop_request;

            drop_request.type = item->type;
            drop_request.user = item->owner;
            copyVector(&drop_request.start, MODEL_POSITION(item->locate));
            setVector(&drop_request.end, 0, 0, 0);
            if (item->proc != 0)
            {
                DISPOSE_ITEM(item);
            }
            ReqItemDrop(&drop_request);
            return;
        }

        if (nearest_target == 0)
        {
            return;
        }
        restore_dokudango_target(item, &item->param.dokudango);
        param->eater = nearest_target;
        if (nearest_target->target ==
            &item->owner->model->locate &&
            (nearest_target->attribute & ATTR_PHASE) == PHASE_CALM)
        {
            param->org_think = nearest_target->think[0];
            param->eater->target = &item->locate->locate;
            param->eater->think[0] = Think1target;
        }
        else
        {
            param->org_think = 0;
        }

        if (nearest_distance >= DOKUDANGO_EAT_RANGE)
        {
            return;
        }
        eater = param->eater;
        if (eater->status == STAT_DAMAGE || eater->status == STAT_STATE ||
            eater->status == STAT_ATTACK || eater->life <= 0)
        {
            return;
        }
        if (ActionHalt == ACTION_HALT_NONE)
        {
            MotionDataType *motion_data;

            dispose_weapon_data_of_char_(eater, ATTACK_CANCEL_ALL);
            UpdateMotion(eater->motion, MOT_ITEM_DRINK);
            eater->status = STAT_ITEM;
            motion_data = eater->motion->motion;
            MoveHumanoid(eater, motion_data->orderspd,
                         motion_data->sidespd);
        }
        if (param->eater->model->n > MODEL_PART_WEAPON_HAND_1)
        {
            item->locate->locate.super =
                &param->eater->model
                     ->object[MODEL_PART_WEAPON_HAND_1]->locate;
            setVector(MODEL_POSITION(item->locate), 0,
                      DOKUDANGO_HUMAN_HAND_OFFSET, 0);
        }
        else
        {
            item->locate->locate.super =
                &param->eater->model
                     ->object[MODEL_PART_BEAST_HAND_0]->locate;
            setVector(MODEL_POSITION(item->locate), 0, 0,
                      DOKUDANGO_BEAST_HAND_OFFSET);
        }
        item->mode++;
        return;
    }

    case DOKUDANGO_MODE_EAT:
    {
        MotionManager *eating_motion;
        Humanoid *eater;

        if (is_humanoid_on_stage_(param->eater) == 0)
        {
            if (item->proc == 0)
            {
                return;
            }
            DISPOSE_ITEM(item);
            return;
        }
        eater = param->eater;
        eating_motion = eater->motion;
        if (eating_motion->mid != MOT_ITEM_DRINK)
        {
            VECTOR *world_position;
            s32 random_x;
            s32 random_y;
            s32 random_z;

            world_position = GetAbsolutePosition(item->locate, 0, 0, 0);
            item->locate->locate.super = 0;
            copyVector(MODEL_POSITION(item->locate), world_position);
            random_x = rand();
            random_x %= DOKUDANGO_DROP_HORIZONTAL_SPREAD;
            random_y = rand();
            random_y %= DOKUDANGO_DROP_VERTICAL_SPREAD;
            random_z = rand();
            random_z %= DOKUDANGO_DROP_HORIZONTAL_SPREAD;
            param->koro.vx =
                random_x - DOKUDANGO_DROP_HORIZONTAL_SPREAD / 2;
            param->koro.vy = random_y + DOKUDANGO_DROP_VERTICAL_BASE;
            param->count = DOKUDANGO_ROLL_DELAY;
            param->koro.hint = 0;
            param->koro.status = KORO_NORMAL;
            param->koro.vz =
                random_z - DOKUDANGO_DROP_HORIZONTAL_SPREAD / 2;
            item->mode = DOKUDANGO_MODE_ROLL;
            return;
        }
        if (eating_motion->count == DOKUDANGO_EAT_FRAME)
        {
            Humanoid *saved_eater;

            saved_eater = eater;
            restore_dokudango_target(item, &item->param.dokudango);
            param->eater = saved_eater;
            NowReturnNormal(saved_eater);
            param->count = DOKUDANGO_POISON_DURATION;
            item->mode++;
            return;
        }
        if ((eater->attribute & ATTR_WEAPON_DRAWN) != 0 &&
            (eater->type & PAGE_MASK) != PAGE_BEAST)
        {
            NowReturnNormal(eater);
        }
        return;
    }

    case DOKUDANGO_MODE_POISON:
    {
        Humanoid *poisoned_eater;
        Humanoid *reaction_target;
        s32 poison_countdown;

        if (is_humanoid_on_stage_(param->eater) == 0)
        {
            if (item->proc == 0)
            {
                return;
            }
            DISPOSE_ITEM(item);
            return;
        }
        poison_countdown = --param->count;
        if ((s16)poison_countdown == 0)
        {
            if (item->proc == 0)
            {
                return;
            }
            DISPOSE_ITEM(item);
            return;
        }
        poisoned_eater = param->eater;
        if (poisoned_eater->life <= 0)
        {
            if (item->proc == 0)
            {
                return;
            }
            DISPOSE_ITEM(item);
            return;
        }

        if (poisoned_eater->status == STAT_DAMAGE ||
            poisoned_eater->status == STAT_STATE ||
            poisoned_eater->status == STAT_ATTACK ||
            poisoned_eater->status == STAT_ITEM)
        {
            return;
        }
        if (rand() % DOKUDANGO_REACTION_PERIOD >=
            DOKUDANGO_REACTION_CHANCE)
        {
            return;
        }
        reaction_target = param->eater;
        if ((reaction_target->type & PAGE_MASK) == PAGE_BEAST)
        {
            apply_dokudango_reaction(reaction_target, MOT_DAMAGE);
        }
        else
        {
            apply_dokudango_reaction(reaction_target, MOT_DAMAGE_CHOKE);
        }
        Sound(param->eater, CHAR_VOICE_HURT);
        return;
    }

    default:
        return;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemDokudango(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:1777, 23 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_dokudango * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *     reg   $s1       struct param_korogari * param
 *     reg   $v0       int x
 *     reg   $v1       int y
 *     reg   $a0       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

int ReqItemDokudango(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    param_dokudango *param;
    s32 x;
    s32 y;
    s32 z;

    item = TakeItemSlot();
    param = &item->param.dokudango;
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemDokudango);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
    }
    {
        param_korogari *param; /* Shadows the outer launch parameter in retail. */

        param = &item->param.dokudango.koro;
        x = p->end.vx;
        y = p->end.vy;
        z = p->end.vz;
        setVector(param, x, y, z);
        item->param.dokudango.koro.hint = 0;
        param->status = KORO_NORMAL;
    }
    param->count = 10;
    param->eater = 0;
    SetNowMotion(item->owner, MOT_ITEM_THROW, MOTION_MOVE_APPLY);
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemGosin(struct tag_TItem *item);
 *     ITEM.C:1804, 52 src lines, frame 88 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct tag_TItem * item
 *     reg   $s2       struct Humanoid * human
 *     reg   $s0       int itemID
 *     stack sp+24     struct PARAM_ITEM_LAUNCH p
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s3       struct tag_TItem * item
 *     stack sp+24     struct VECTOR v
 * END PSX.SYM */


extern VECTOR vec_y_n1200_z_400; /* {0,-1200,400} */

void ProcItemGosin(TItem *item)
{
    enum
    {
        GOSIN_MODE_START = 0,
        GOSIN_MODE_WAIT = 1,
        GOSIN_MODE_ACTIVE = 2
    };
    PARAM_ITEM_LAUNCH drop_request;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->owner->active_item = ACTIVE_ITEM_NONE;
        item->mode = GOSIN_MODE_START;
        return;
    }
    switch (item->mode)
    {
    case GOSIN_MODE_START:
        SetNowMotion(item->owner, MOT_ITEM_KAENGEKI, MOTION_MOVE_APPLY);
        Sound(item->owner, SE_ITEM_USE);
        item->mode++;
        return;

    case GOSIN_MODE_WAIT:
    {
        MotionManager *mot;

        mot = item->owner->motion;
        if (mot->mid != MOT_ITEM_KAENGEKI)
        {
            VECTOR *pos;
            Humanoid *human;
            s32 itemID;

            pos = GetAbsolutePosition(item->locate, 0, 0, 0);
            human = item->owner;
            itemID = item->type;
            drop_request = (PARAM_ITEM_LAUNCH){0};
            drop_request.type = itemID;
            drop_request.user = human;
            drop_request.start.vx = pos->vx;
            drop_request.start.vy = pos->vy;
            drop_request.start.vz = pos->vz;
            drop_request.end.vx = rand() % 200 - 100;
            drop_request.end.vy = rand() % 100 - 200;
            drop_request.end.vz = rand() % 200 - 100;
            ReqItemDrop(&drop_request);
            if (item->proc == 0)
                return;
            DISPOSE_ITEM(item);
            return;
        }
        if (mot->count != 0)
            return;
        if (mot->loop == 0)
            return;
        NowReturnNormal(item->owner);
        SetBleeds(
            GetAbsolutePosition(
                item->owner->model->object[MODEL_PART_TORSO], 0, 0, 0),
            600, 100, 20, 15, RGB24(180, 140, 30));
        item->owner->active_item = item->type;
        item->param.gosin.count = GOSIN_DURATION;
        item->mode++;
        return;
    }

    case GOSIN_MODE_ACTIVE:
    {
        s16 c;

        c = item->param.gosin.count - 1;
        item->param.gosin.count = c;
        if (c == 0)
        {
            if (item->proc == 0)
                return;
            DISPOSE_ITEM(item);
            return;
        }
        if ((c & 0x3f) != 0)
            return;
        *(VECTOR *)&drop_request = vec_y_n1200_z_400;
        set_impact_ex_((VECTOR *)&drop_request, &item->owner->model->locate,
                       FIXED_ONE, 6 * FIXED_ONE, COLOR_GRAY, 0,
                       (s16)(rand() % 360), 2, 120, IMPACT_SPRITE_GOSIN);
        return;
    }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemGosin(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:1860, 18 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

static int ReqItemGosin(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;

    item = TakeItemSlot();
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemGosin);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
    }
    return 1;
}

static u8 NingyoCount = 0;


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcItemNingyo(struct tag_TItem *item);
 *     ITEM.C:1882, 132 src lines, frame 112 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s5       struct tag_TItem * item
 *     reg   $s4       struct param_ningyo * param
 *     reg   $a0       int cid
 *     reg   $s5       struct tag_TItem * item
 *     stack sp+24     struct SVECTOR sv
 *     stack sp+32     struct VECTOR v
 *     reg   $s5       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       struct ModelType * model
 *     reg   $s2       int i
 *     reg   $s0       struct Humanoid * human
 *     reg   $s1       long len
 *     reg   $v0       struct Humanoid * human
 *     reg   $v0       struct Humanoid * human
 *     reg   $a1       int i
 *     reg   $s5       struct tag_TItem * item
 *     stack sp+48     struct VECTOR pos
 *     reg   $s4       struct param_korogari * param
 *     reg   $s4       struct param_korogari * param
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct TCameraStatus CamState;
 *     extern struct Humanoid *HumanGroup[32];
 *     extern struct ModelType *NingyoModel;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct SVECTOR ConflictDistance;
 * END PSX.SYM */

static void ProcItemNingyo(TItem *item)
{
    enum
    {
        NINGYO_MODE_WAIT = 0,
        NINGYO_MODE_GROW = 1,
        NINGYO_MODE_ACTIVE = 2,
        MAX_ACTIVE_NINGYO = 3,
        ACTIVE_NINGYO_HP = 3,
        APPEAR_SMOKE_COUNT = 10,
        APPEAR_SMOKE_TIME = 6,
        APPEAR_SMOKE_RISE_SPEED = -25,
        DROP_HORIZONTAL_SPREAD = 200,
        DROP_VERTICAL_SPREAD = 100,
        DROP_UPWARD_SPEED = 200,
        GROWTH_SCALE_SHIFT = 8,
        GROWTH_FRAMES = FIXED_ONE >> GROWTH_SCALE_SHIFT,
        NINGYO_COLLISION_SIZE = 500,
        FIRST_RETARGET_DELAY = 3,
        RETARGET_INTERVAL = 30,
        NINGYO_LURE_RANGE = 10000,
        NINGYO_BREAK_BLEED_GROUND_RANGE = 0,
        NINGYO_BREAK_BLEED_SPREAD = 30,
        NINGYO_BREAK_BLEED_COUNT = 30,
        NINGYO_BREAK_BLEED_TIME = 30,
        KNOCKBACK_Y_SPEED = 100,
        KNOCKBACK_SPREAD = 20,
        HIT_KNOCKBACK_DIVISOR = 16,
        BUMP_KNOCKBACK_DIVISOR = 8
    };
    param_ningyo *param;
    s32 collision_id;
    item_mode dispose_mode;

    param = &item->param.ningyo;
    dispose_mode = ITEM_MODE_DISPOSE;
    if (item->mode == dispose_mode)
    {
        if (param->hp != NINGYO_HP)
        {
            s32 human_index;

            for (human_index = 0; human_index < Humans; human_index++)
            {
                Humanoid *human;

                human = HumanGroup[human_index];
                if (human->target == &item->locate->locate)
                {
                    human->target = &CamState.Owner->model->locate;
                }
            }
            NingyoCount--;
        }
        item->mode = NINGYO_MODE_WAIT;
        return;
    }

    MoveKorogari(item, &param->koro);
    if (param->koro.status == KORO_WATER)
    {
        if (item->proc == 0)
        {
            return;
        }
        DISPOSE_ITEM_WITH_MODE(item, dispose_mode);
        return;
    }

    switch (item->mode)
    {
    case NINGYO_MODE_WAIT:
    {
        {
            if (--param->count == 0)
            {
                SVECTOR smoke_velocity;

                param->count = 0;
                item->mode++;
                smoke_velocity = (SVECTOR){
                    .vx = 0,
                    .vy = APPEAR_SMOKE_RISE_SPEED,
                    .vz = 0
                };
                SetSmoke(MODEL_POSITION(item->locate),
                         &smoke_velocity,
                         APPEAR_SMOKE_COUNT, APPEAR_SMOKE_TIME);
                SoundEx(MODEL_POSITION(item->locate), SE_SMOKE_PUFF);
                if (NingyoCount < MAX_ACTIVE_NINGYO)
                {
                    param->hp = ACTIVE_NINGYO_HP;
                    NingyoCount++;
                }
                else
                {
                    Humanoid *owner;
                    s32 item_type;
                    ModelType *model;
                    PARAM_ITEM_LAUNCH launch_request;

                    owner = item->owner;
                    item_type = item->type;
                    model = item->locate;
                    ClearItemLaunchRequest(&launch_request);
                    launch_request.type = item_type;
                    launch_request.user = owner;
                    {
                        VECTOR *position;

                        position = MODEL_POSITION(model);
                        launch_request.start.vx = position->vx;
                        launch_request.start.vy = position->vy;
                        launch_request.start.vz = position->vz;
                    }
                    launch_request.end.vx =
                        rand() % DROP_HORIZONTAL_SPREAD -
                        DROP_HORIZONTAL_SPREAD / 2;
                    launch_request.end.vy =
                        rand() % DROP_VERTICAL_SPREAD - DROP_UPWARD_SPEED;
                    launch_request.end.vz =
                        rand() % DROP_HORIZONTAL_SPREAD -
                        DROP_HORIZONTAL_SPREAD / 2;
                    ReqItemDrop(&launch_request);

                    if (item->proc == 0)
                    {
                        return;
                    }
                    DISPOSE_ITEM_WITH_MODE(item, dispose_mode);
                    return;
                }
            }

            UpdateCoordinate(item->locate);
            item->model->locate = item->locate->locate;
            DrawSprite((Sprite3D *)item->model);
            return;
        }
    }

    case NINGYO_MODE_GROW:
    {
        {
            s32 new_conflict_id;
            ConflictObjectType *conflict_pool;
            ConflictObjectType *conflict;

            param->count++;
            {
                VECTOR scale = {
                    .vx = param->count << GROWTH_SCALE_SHIFT,
                    .vy = param->count << GROWTH_SCALE_SHIFT,
                    .vz = param->count << GROWTH_SCALE_SHIFT
                };

                RotMatrixYXZ(&item->locate->rotate, &item->locate->locate.coord);
                ScaleMatrix(&item->locate->locate.coord, &scale);
            }
            item->locate->locate.flg = 0;
            NingyoModel->locate = item->locate->locate;
            DrawModel(NingyoModel);
            if (param->count < GROWTH_FRAMES)
            {
                return;
            }

            DeleteConflict(item->locate);
            new_conflict_id = InsertConflict(item->locate);
            conflict_pool = ConflictObject;
            conflict = conflict_pool + new_conflict_id;
            {
                s32 collision_offset_y = -NINGYO_COLLISION_SIZE / 2;
                s32 collision_size = NINGYO_COLLISION_SIZE;
                INITIALIZE_CONFLICT_OBJECT(
                    conflict, collision_size, collision_offset_y,
                    CONFLICT_OWNER_ITEM, CONFLICT_STAND | CONFLICT_SOFT);
                item->collision.mode = conflict->size.pad;
                item->collision.size = collision_size;
                item->collision.ofsY = collision_offset_y;
                item->collision.pause = ITEM_COLLISION_ACTIVE;
            }
            param->count = FIRST_RETARGET_DELAY;
            item->mode++;
            return;
        }
    }

    case NINGYO_MODE_ACTIVE:
    {
        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        {
            collision_id = CONFLICT_NONE;
        }
        else
        {
            collision_id = GetConflictResult(item->locate, CONFLICT_NONE);
        }

        if (--param->count == 0)
        {
            s32 human_index;

            human_index = 0;
            while (1)
            {
                Humanoid *human;
                s32 distance_to_decoy;

                if (human_index >= Humans)
                {
                    break;
                }
                human = HumanGroup[human_index];
                distance_to_decoy = GetVectorDistance(
                    MODEL_POSITION(item->locate),
                    human->locate);
                if (distance_to_decoy < NINGYO_LURE_RANGE &&
                    human->target != 0 &&
                    distance_to_decoy <
                        GetVectorDistance(
                            COORDINATE_POSITION(human->target),
                            human->locate) &&
                    ((u16)human->type & PAGE_MASK) != PAGE_BOSS)
                {
                    human->target = &item->locate->locate;
                }
                human_index++;
            }
            param->count = RETARGET_INTERVAL;
        }
        else if (collision_id != CONFLICT_NONE)
        {
            ConflictObjectType *conflict_pool = ConflictObject;
            ConflictObjectType *conflict = &conflict_pool[collision_id];
            ConflictClass conflict_class = conflict->size.pad;
            if (conflict_class == CONFLICT_HIT)
            {
                if (param->hp == 0)
                {
                    SetBleeds(MODEL_POSITION(item->locate),
                              NINGYO_BREAK_BLEED_GROUND_RANGE,
                              NINGYO_BREAK_BLEED_SPREAD,
                              NINGYO_BREAK_BLEED_COUNT,
                              NINGYO_BREAK_BLEED_TIME, COLOR_YELLOW);
                    SoundEx(MODEL_POSITION(item->locate),
                            SE_SMOKE_PUFF);
                    if (item->proc != 0)
                    {
                        DISPOSE_ITEM(item);
                    }
                    item->mode++;
                }
                else
                {
                    s32 knockback_x;
                    s32 knockback_z;
                    /* Retail snapshots this contact point but never reads it. */
                    VECTOR pos = {
                        .vx = conflict->position.vx,
                        .vy = conflict->position.vy,
                        .vz = conflict->position.vz
                    };

                    knockback_x = -ConflictDistance.vx /
                                  HIT_KNOCKBACK_DIVISOR;
                    knockback_z = -ConflictDistance.vz /
                                  HIT_KNOCKBACK_DIVISOR;
                    param->koro.vx = knockback_x;
                    param->koro.vy = -KNOCKBACK_Y_SPEED;
                    param->koro.vz = knockback_z;
                    param->koro.hint = 0;
                    param->koro.status = KORO_NORMAL;
                    param->hp--;
                    SoundEx(MODEL_POSITION(item->locate),
                            SE_PROJECTILE_HIT);
                }
            }
            else if (conflict_class != CONFLICT_SOFT)
            {
                s32 x_random = rand();
                s32 z_random;
                s32 knockback_y;
                s32 knockback_z;
                s16 knockback_x;
                s16 x_jitter;

                knockback_x = -ConflictDistance.vx / BUMP_KNOCKBACK_DIVISOR;
                x_jitter = x_random % KNOCKBACK_SPREAD;
                knockback_y = 0;
                if (ConflictDistance.vy >= -NINGYO_COLLISION_SIZE)
                {
                    knockback_y = -KNOCKBACK_Y_SPEED;
                }
                z_random = rand();
                knockback_z = -ConflictDistance.vz / BUMP_KNOCKBACK_DIVISOR;
                param->koro.vx = knockback_x + x_jitter -
                                 KNOCKBACK_SPREAD / 2;
                param->koro.vy = knockback_y;
                param->koro.hint = 0;
                param->koro.status = KORO_NORMAL;
                param->koro.vz = knockback_z +
                                 z_random % KNOCKBACK_SPREAD -
                                 KNOCKBACK_SPREAD / 2;
            }
        }

        UpdateCoordinate(item->locate);
        NingyoModel->locate = item->locate->locate;
        DrawModel(NingyoModel);
        return;
    }
    }
    return;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemNingyo(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2018, 24 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_ningyo * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *     reg   $s1       struct param_korogari * param
 *     reg   $v0       int x
 *     reg   $v1       int y
 *     reg   $a0       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

static int ReqItemNingyo(PARAM_ITEM_LAUNCH *p)
{
    enum { NINGYO_ROLL_VARIATION = 68 };
    TItem *item;
    param_ningyo *param;
    s32 x;
    s32 y;
    s32 z;

    item = TakeItemSlot();
    param = &item->param.ningyo;
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemNingyo);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
    }
    {
        param_korogari *rolling_motion;

        rolling_motion = &param->koro;
        x = p->end.vx;
        y = p->end.vy;
        z = p->end.vz;
        setVector(rolling_motion, x, y, z);
        rolling_motion->hint = 0;
        rolling_motion->status = KORO_NORMAL;
    }
    param->count = NINGYO_DURATION;
    item->locate->rotate.vx = 0;
    item->locate->rotate.vy = rand() % ANGLE_FULL;
    item->locate->rotate.vz = rand() % NINGYO_ROLL_VARIATION;
    param->hp = NINGYO_HP;
    SetNowMotion(item->owner, MOT_ITEM_THROW, MOTION_MOVE_APPLY);
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcItemHenshin(struct tag_TItem *item);
 *     ITEM.C:2060, 140 src lines, frame 208 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct tag_TItem * item
 *     reg   $s4       struct param_henshin * param
 *     reg   $a1       int i
 *     reg   $s1       struct ModelArchiveType * mad
 *     reg   $a2       struct ModelArchiveType * hen
 *     stack sp+16     struct ModelArchiveType *[30] target
 *     reg   $s2       int targets
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s2       struct Humanoid * human
 *     reg   $s0       int itemID
 *     stack sp+136    struct PARAM_ITEM_LAUNCH p
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s3       struct tag_TItem * item
 *     stack sp+136    struct SVECTOR sv
 *     stack sp+144    struct SVECTOR sv
 *     reg   $s3       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern long EmergencyNotice;
 * END PSX.SYM */


static TItem *HenshinItem = 0;
static u16 HenshinCount = 0;

enum henshin_mode
{
    HENSHIN_MODE_START = 0,
    HENSHIN_MODE_WAIT = 1,
    HENSHIN_MODE_TRANSFORM = 2,
    HENSHIN_MODE_ACTIVE = 3
};

enum
{
    HENSHIN_DURATION = 600,
    HENSHIN_DROP_HORIZONTAL_SPREAD = 200,
    HENSHIN_DROP_VERTICAL_SPREAD = 100,
    HENSHIN_DROP_UPWARD_SPEED = 200,
    HENSHIN_SMOKE_RISE_SPEED = -50,
    HENSHIN_SMOKE_COUNT = 10,
    HENSHIN_SMOKE_TIME = 6
};

static void ProcItemHenshin(TItem *item)
{
    ModelArchiveType *archive = item->owner->model;
    PARAM_ITEM_LAUNCH drop_request;

    if (item->mode == ITEM_MODE_DISPOSE)
    {
        if (item == HenshinItem)
        {
            HenshinModelSnapshot *snapshot = &Item_save;
            ApplyHenshinModel(snapshot, archive);
            if (item->owner->status == STAT_SQUAT)
            {
                NowReturnNormal(item->owner);
            }
            HenshinItem = 0;
            (*(Humanoid *volatile *)&item->owner)->active_item =
                ACTIVE_ITEM_NONE;
        }
        item->mode = HENSHIN_MODE_START;
        return;
    }

    switch (item->mode)
    {
    case HENSHIN_MODE_START:
        SetNowMotion(item->owner, MOT_ITEM_KAENGEKI, MOTION_MOVE_APPLY);
        Sound(item->owner, SE_ITEM_USE);
        item->mode++;
        return;

    case HENSHIN_MODE_WAIT:
    {
        Humanoid *human = item->owner;
        MotionManager *motion = human->motion;
        if (motion->mid != MOT_ITEM_KAENGEKI)
        {
            VECTOR *drop_position =
                GetAbsolutePosition(item->locate, 0, 0, 0);
            Humanoid *drop_owner = item->owner;
            TItemType item_type = item->type;

            drop_request = (PARAM_ITEM_LAUNCH){0};
            drop_request.type = item_type;
            drop_request.user = drop_owner;
            copyVector(&drop_request.start, drop_position);
            drop_request.end.vx =
                rand() % HENSHIN_DROP_HORIZONTAL_SPREAD -
                HENSHIN_DROP_HORIZONTAL_SPREAD / 2;
            drop_request.end.vy =
                rand() % HENSHIN_DROP_VERTICAL_SPREAD -
                HENSHIN_DROP_UPWARD_SPEED;
            drop_request.end.vz =
                rand() % HENSHIN_DROP_HORIZONTAL_SPREAD -
                HENSHIN_DROP_HORIZONTAL_SPREAD / 2;
            ReqItemDrop(&drop_request);
            if (item->proc == 0)
            {
                return;
            }
            DISPOSE_ITEM(item);
            return;
        }
        if (motion->count != 0 || motion->loop == 0)
        {
            return;
        }

        NowReturnNormal(human);
        {
            SVECTOR *smoke_velocity = (SVECTOR *)&drop_request;

            *smoke_velocity = (SVECTOR){
                .vx = 0,
                .vy = HENSHIN_SMOKE_RISE_SPEED,
                .vz = 0
            };
            SetSmoke(MODEL_POSITION(archive), smoke_velocity,
                     HENSHIN_SMOKE_COUNT, HENSHIN_SMOKE_TIME);
        }
        {
            TItem *previous_disguise = HenshinItem;
            if (previous_disguise != 0 && previous_disguise->proc != 0)
            {
                DISPOSE_ITEM(previous_disguise);
            }
        }
        HenshinItem = item;
        item->mode++;
        return;
    }

    case HENSHIN_MODE_TRANSFORM:
    {
        HenshinModelSnapshot *snapshot = &HenshinSnapshot;
        Humanoid *disguise_owner;
        u16 item_type;

        ApplyHenshinModel(snapshot, archive);
        item->mode++;
        HenshinCount = HENSHIN_DURATION;
        disguise_owner = *(Humanoid *volatile *)&item->owner;
        /* TItemType is a 32-bit enum; retail deliberately reads its low
         * half. */
        item_type = *(u16 *)&item->type;
        EmergencyNotice = -HENSHIN_DURATION;
        disguise_owner->active_item = item_type;
        return;
    }

    case HENSHIN_MODE_ACTIVE:
    {
        u16 remaining_count;

        remaining_count = --HenshinCount;
        if ((s16)remaining_count > 0 &&
            item->owner->active_item == item->type &&
            item->owner->status != STAT_DAMAGE &&
            item->owner->status != STAT_DEAD)
        {
            if (item->owner->status != STAT_ATTACK)
            {
                return;
            }
            if (item->owner->motion->loop >= 0 &&
                item->owner->motion->mid < MOT_ATTACK_STEALTH_BACK)
            {
                return;
            }
        }
        {
            SVECTOR *smoke_velocity = (SVECTOR *)&drop_request;

            *smoke_velocity = (SVECTOR){
                .vx = 0,
                .vy = HENSHIN_SMOKE_RISE_SPEED,
                .vz = 0
            };
            SetSmoke(MODEL_POSITION(archive), smoke_velocity,
                     HENSHIN_SMOKE_COUNT, HENSHIN_SMOKE_TIME);
        }
        if (item->proc == 0)
        {
            return;
        }
        DISPOSE_ITEM(item);
        return;
    }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemHenshin(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2204, 15 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s2       struct param_henshin * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

static int ReqItemHenshin(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;

    item = TakeItemSlot();
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemHenshin);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
    }
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcItemGoshikimai(struct tag_TItem *item);
 *     ITEM.C:2223, 35 src lines, frame 72 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct tag_TItem * item
 *     reg   $s0       struct param_goshikimai * param
 *     reg   $s0       struct Humanoid * human
 *     stack sp+16     struct PARAM_ITEM_LAUNCH p
 *     reg   $s1       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern short ActionHalt;
 * END PSX.SYM */

static void ProcItemGoshikimai(TItem *item)
{
    enum
    {
        GOSHIKIMAI_MODE_START = 0,
        GOSHIKIMAI_MODE_THROW = 1,
        GOSHIKIMAI_RELEASE_FRAME = 15
    };
    param_goshikimai *param = &item->param.goshikimai;

    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = GOSHIKIMAI_MODE_START;
        return;
    }
    switch (item->mode)
    {
    case GOSHIKIMAI_MODE_START:
    {
        Humanoid *human = item->owner;

        if (ActionHalt == ACTION_HALT_NONE && human->life > 0)
        {
            dispose_weapon_data_of_char_(human, ATTACK_CANCEL_ALL);
            UpdateMotion(human->motion, MOT_ITEM_PLANT);
            human->status = STAT_STATE;
            MoveHumanoid(human, human->motion->motion->orderspd,
                         human->motion->motion->sidespd);
        }
        item->mode++;
        return;
    }

    case GOSHIKIMAI_MODE_THROW:
    {
        PARAM_ITEM_LAUNCH drop_request;

        if (item->owner->motion->mid != MOT_ITEM_PLANT)
        {
            item->mode = GOSHIKIMAI_MODE_START;
            return;
        }
        if (item->owner->motion->count != GOSHIKIMAI_RELEASE_FRAME)
            return;
        drop_request.type = ITEM_GOSHIKIMAI;
        drop_request.user = item->owner;
        /* Retail recomputes the shared result for each component. */
        drop_request.start.vx =
            GetAbsolutePosition(
                item->owner->model->object[MODEL_PART_WEAPON_HAND_0],
                0, 0, 0)->vx;
        drop_request.start.vy =
            GetAbsolutePosition(
                item->owner->model->object[MODEL_PART_WEAPON_HAND_0],
                0, 0, 0)->vy;
        drop_request.start.vz =
            GetAbsolutePosition(
                item->owner->model->object[MODEL_PART_WEAPON_HAND_0],
                0, 0, 0)->vz;
        setVector(&drop_request.end,
                  param->vec.vx, param->vec.vy, param->vec.vz);
        NowReturnNormal(item->owner);
        if (item->proc != 0)
        {
            item->mode = ITEM_MODE_DISPOSE;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != GOSHIKIMAI_MODE_START)
            {
                AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
        }
        ReqItemDrop(&drop_request);
        return;
    }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemGoshikimai(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2262, 21 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s2       struct param_goshikimai * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

static int ReqItemGoshikimai(PARAM_ITEM_LAUNCH *p)
{
    TItem *item = TakeItemSlot();
    param_goshikimai *param = &item->param.goshikimai;
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemGoshikimai);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
    }
    setVector(&param->vec, p->end.vx, p->end.vy, p->end.vz);
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcItemKaengeki(struct tag_TItem *item);
 *     ITEM.C:2287, 54 src lines, frame 80 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct tag_TItem * item
 *     reg   $s1       struct param_kaengeki * param
 *     reg   $s0       struct Humanoid * human
 *     reg   $s2       struct Humanoid * human
 *     reg   $s0       int itemID
 *     stack sp+16     struct PARAM_ITEM_LAUNCH p
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s3       struct tag_TItem * item
 *     stack sp+16     struct PARAM_ITEM_LAUNCH rp
 *
 * Globals it touches, as the original declared them:
 *     extern short ActionHalt;
 *     extern struct TCameraStatus CamState;
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */



static void ProcItemKaengeki(TItem *item)
{
    enum
    {
        KAENGEKI_MODE_START = 0,
        KAENGEKI_MODE_WAIT = 1,
        KAENGEKI_MODE_FIRE = 2,
        KAENGEKI_AIM_TURN_STEP = ANGLE_FULL / 128,
        KAENGEKI_FLAME_OFFSET_SCALE = 12,
        KAENGEKI_FLAME_REACH_SCALE = 2
    };
    param_kaengeki *param;
    PARAM_ITEM_LAUNCH request;
    s32 rx;
    s32 ry;
    s32 dispose_mode;
    item_mode current_mode;

    param = &item->param.kaengeki;
    dispose_mode = ITEM_MODE_DISPOSE;
    current_mode = item->mode;
    if (current_mode == dispose_mode)
    {
        if (item->owner->motion->mid == MOT_ITEM_KAENGEKI)
        {
            NowReturnNormal(item->owner);
        }
        item->mode = KAENGEKI_MODE_START;
        return;
    }

    switch (item->mode)
    {
    case KAENGEKI_MODE_START:
    {
        Humanoid *human;

        human = item->owner;
        if (ActionHalt == ACTION_HALT_NONE && human->life > 0)
        {
            dispose_weapon_data_of_char_(human, ATTACK_CANCEL_ALL);
            UpdateMotion(human->motion, MOT_ITEM_KAENGEKI);
            human->status = STAT_ITEM;
            MoveHumanoid(human, human->motion->motion->orderspd,
                         human->motion->motion->sidespd);
        }
        Sound(item->owner, SE_ITEM_USE);
        item->mode++;
        return;
    }

    case KAENGEKI_MODE_WAIT:
    {
        if (item->owner->motion->count == 0 &&
            item->owner->motion->loop != 0)
        {
            SoundEx(MODEL_POSITION(item->owner->model), SE_FIRE);
            item->mode++;
            param->count = KAENGEKI_DELAY;
        }
        if (item->owner->motion->mid == MOT_ITEM_KAENGEKI)
        {
            return;
        }
        {
            VECTOR *pos;
            Humanoid *human;
            s32 itemID;

            pos = GetAbsolutePosition(item->locate, 0, 0, 0);
            human = item->owner;
            itemID = item->type;
            request = (PARAM_ITEM_LAUNCH){0};
            request.type = itemID;
            request.user = human;
            request.start.vx = pos->vx;
            request.start.vy = pos->vy;
            request.start.vz = pos->vz;
            request.end.vx = rand() % 200 - 100;
            request.end.vy = rand() % 100 - 200;
            request.end.vz = rand() % 200 - 100;
            ReqItemDrop(&request);
            if (item->proc == 0)
            {
                return;
            }
            DISPOSE_ITEM(item);
            return;
        }
    }

    case KAENGEKI_MODE_FIRE:
    {
        ModelArchiveType *model;
        s32 rz;

        if (item->owner->motion->mid == MOT_ITEM_KAENGEKI &&
            --param->count != 0)
        {
            if ((item->owner->pad.data & PADLright) != 0)
            {
                item->owner->model->rotate.vy += KAENGEKI_AIM_TURN_STEP;
            }
            else if ((item->owner->pad.data & PADLleft) != 0)
            {
                item->owner->model->rotate.vy -= KAENGEKI_AIM_TURN_STEP;
            }

            request.user = item->owner;
            request.type = ITEM_NAPALM;
            request.end.vx = param->end.vx;
            request.end.vy = param->end.vy;
            request.end.vz = param->end.vz;
            model = item->owner->model;
            if (CamState.Owner->model == model &&
                CamState.Mode == CMODE_DIRECTION)
            {
                GetVectorRotation(CAMERA_VIEWPOINT(&ViewInfo),
                                  CAMERA_REFERENCE(&ViewInfo),
                                  &rx, &ry);
                rz = 0;
            }
            else
            {
                rx = model->rotate.vx;
                rz = model->rotate.vz;
                ry = model->rotate.vy;
            }
            RotateVector(&request.end, rx, ry, rz);

            copyVector(&request.start, &request.end);
            request.start.vx *= KAENGEKI_FLAME_OFFSET_SCALE;
            request.start.vy *= KAENGEKI_FLAME_OFFSET_SCALE;
            request.start.vz *= KAENGEKI_FLAME_OFFSET_SCALE;
            request.start.vx += param->start.vx;
            request.start.vy += param->start.vy;
            request.start.vz += param->start.vz;
            request.end.vx *= KAENGEKI_FLAME_REACH_SCALE;
            request.end.vy *= KAENGEKI_FLAME_REACH_SCALE;
            request.end.vz *= KAENGEKI_FLAME_REACH_SCALE;
            request.end.vx += request.start.vx;
            request.end.vy += request.start.vy;
            request.end.vz += request.start.vz;
            ReqItemUse(&request);
            return;
        }

        if (item->proc == 0)
        {
            return;
        }
        DISPOSE_ITEM_WITH_MODE(item, dispose_mode);
        return;
    }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemKaengeki(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2344, 20 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_kaengeki * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

static int ReqItemKaengeki(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    param_kaengeki *param;

    item = TakeItemSlot();
    param = &item->param.kaengeki;
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemKaengeki);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
    }
    item->param.kaengeki.start.vx = p->start.vx;
    param->start.vy = p->start.vy;
    param->start.vz = p->start.vz;
    param->end.vx = p->end.vx;
    param->end.vy = p->end.vy;
    param->end.vz = p->end.vz;
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcItemNinken(struct tag_TItem *item);
 *     ITEM.C:2368, 89 src lines, frame 64 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct tag_TItem * item
 *     reg   $s1       struct param_ninken * param
 *     reg   $s2       struct tag_TItem * item
 *     stack sp+24     struct SVECTOR vec
 *     stack sp+32     struct SVECTOR vec
 *     reg   $s2       struct tag_TItem * item
 *     reg   $s0       unsigned long at
 *     reg   $s0       struct Humanoid * target
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern long GameClock;
 * END PSX.SYM */

static void ProcItemNinken(TItem *item)
{
    enum
    {
        NINKEN_MODE_ROLL = 0,
        NINKEN_MODE_SPAWN = 1,
        NINKEN_MODE_ACTIVE = 2,
        NINKEN_SPAWN_PROBE_DEPTH = 2000,
        NINKEN_SPAWN_PROBE_RADIUS = 500,
        NINKEN_SPAWN_MAX_DROP = 500,
        NINKEN_SPAWN_RETRY_DELAY = 15,
        NINKEN_SMOKE_RISE_SPEED = 50,
        NINKEN_SMOKE_PARTICLES = 10,
        NINKEN_SMOKE_LIFETIME = 6,
        NINKEN_TARGET_SCAN_INTERVAL = 15,
        NINKEN_TARGET_SEARCH_RADIUS = 10000
    };
    param_ninken *param;

    param = &item->param.ninken;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        Humanoid *slave;

        slave = param->slave;
        if (slave != 0)
        {
            NowReturnNormal(slave);
            param->slave->attribute |= ATTR_SUSPEND;
            setVector(MODEL_POSITION(param->slave->model),
                      NINKEN_PARK_POS, NINKEN_PARK_POS, NINKEN_PARK_POS);
            UpdateCoordinate((ModelType *)param->slave->model);
        }
        item->mode = NINKEN_MODE_ROLL;
        return;
    }

    switch (item->mode)
    {
    case NINKEN_MODE_ROLL:
    {
        u8 status;

        MoveKorogari(item, &param->koro);
        status = param->koro.status;
        if (status == KORO_WATER)
        {
            if (item->proc == 0)
            {
                return;
            }
            DISPOSE_ITEM(item);
            return;
        }
        if (status == KORO_STAY)
        {
            PARAM_ITEM_STAY saved_record;
            PARAM_ITEM_STAY stay_request;

            stay_request = (PARAM_ITEM_STAY){0};
            stay_request.type = item->type;
            copyVector(&stay_request.locate, MODEL_POSITION(item->locate));
            saved_record = stay_request;

            if (item->proc != 0)
            {
                DISPOSE_ITEM(item);
            }

            DropStayedItem(&saved_record);
            return;
        }
        {
            u16 count;

            count = --param->count;
            if ((s16)count <= 0)
            {
                item->mode++;
            }
            UpdateCoordinate(item->locate);
            item->model->locate = item->locate->locate;
            DrawSprite((Sprite3D *)item->model);
            return;
        }
    }

    case NINKEN_MODE_SPAWN:
    {
        s32 create;

        create = 0;
        if (is_humanoid_on_stage_(NINKEN_CHARACTER_PTR) == 0 ||
            GetHumanoid(NINKEN) == 0)
        {
            create = 1;
        }
        if (create != 0)
        {
            NINKEN_CHARACTER_PTR =
                BreedLife(NINKEN, NINKEN_PARK_POS, NINKEN_PARK_POS,
                          NINKEN_PARK_POS, 0);
            NINKEN_CHARACTER_PTR->attribute |= ATTR_SUSPEND;
        }

        {
            s32 valid;
            VECTOR *position;
            VECTOR *query;
            MapVector *map;
            VECTOR pos;
            VECTOR work;
            MapVector map_result;

            position = &pos;
            query = &work;
            map = &map_result;
            copyVector(&pos, MODEL_POSITION(item->locate));
            work.vx = position->vx;
            work.vy = position->vy;
            work.vz = position->vz;
            work.vy -= NINKEN_SPAWN_PROBE_DEPTH;
            GetAreaMapVector(GlobalAreaMap, map, query,
                             NINKEN_SPAWN_PROBE_RADIUS,
                             AREA_LEVEL_DEFAULT);

            if (map_result.level >= position->vy - NINKEN_SPAWN_MAX_DROP)
            {
                if (map_result.level < position->vy)
                {
                    position->vy = map_result.level;
                }
                valid = 1;
            }
            else
            {
                valid = 0;
            }
            if (valid == 0 ||
                (NINKEN_CHARACTER_PTR->attribute & ATTR_SUSPEND) == 0)
            {
                item->mode--;
                param->count = NINKEN_SPAWN_RETRY_DELAY;
                return;
            }

            *(SVECTOR *)&work = (SVECTOR){
                .vx = 0,
                .vy = -NINKEN_SMOKE_RISE_SPEED,
                .vz = 0
            };
            SetSmoke(&pos, (SVECTOR *)&work, NINKEN_SMOKE_PARTICLES,
                     NINKEN_SMOKE_LIFETIME);
            SoundEx(&pos, SE_SMOKE_PUFF);
            param->slave = NINKEN_CHARACTER_PTR;
            NINKEN_CHARACTER_PTR->status = STAT_NORMAL;
            param->slave->life = param->slave->lifemax;
            copyVector(MODEL_POSITION(param->slave->model), &pos);
            copyVector(&param->slave->model->rotate,
                       &item->owner->model->rotate);
            EquipWeapon(param->slave, WEAPON_SHEATHED);
            SetNowMotion(param->slave, MOT_STATE_SHEATHE, MOTION_MOVE_APPLY);
            param->slave->attribute &= ~ATTR_PHASE;
            param->slave->attribute = 0;
            param->slave->target = &item->owner->model->locate;
            param->slave->motion->count = 0;
            PlayMotion(param->slave->motion, 1);
            param->slave->attribute &= ~ATTR_SUSPEND;
            param->slave->model->object[MODEL_PART_WAIST]->attribute |= MODEL_ATTR_COLLIDE;
            set_model_hide_(param->slave, 0);
            param->slave->vector.vy = 0;
            item->mode++;
            param->count = NINKEN_DURATION;
            return;
        }
    }

    case NINKEN_MODE_ACTIVE:
    {
        u16 count;
        Humanoid *slave;

        if (is_humanoid_on_stage_(param->slave) == 0)
        {
            if (item->proc != 0)
            {
                item->mode = ITEM_MODE_DISPOSE;
                item->proc(item);
                DeleteConflict(item->locate);
                if (item->mode != NINKEN_MODE_ROLL)
                {
                    AdtMessageBox(msg_item_dispose_fail, item->type,
                                  (u32)item->mode);
                }
                item->owner = 0;
                item->proc = 0;
            }
        }

        count = --param->count;
        if ((s16)count <= 0 ||
            (slave = param->slave)->life <= 0 ||
            (slave->attribute & ATTR_SUSPEND) != 0)
        {
            SVECTOR vec = {
                .vx = 0,
                .vy = -NINKEN_SMOKE_RISE_SPEED,
                .vz = 0
            };
            SetSmoke(MODEL_POSITION(param->slave->model),
                     &vec, NINKEN_SMOKE_PARTICLES,
                     NINKEN_SMOKE_LIFETIME);
            SoundEx(MODEL_POSITION(param->slave->model), SE_SMOKE_PUFF);
            TurnAroundAllItems(param->slave);
            if (item->proc == 0)
            {
                return;
            }
            DISPOSE_ITEM(item);
            return;
        }

    {
        s32 owner_attribute;
        Humanoid *target;
        character_status status;

        if (GameClock % NINKEN_TARGET_SCAN_INTERVAL != 0)
        {
            return;
        }
        status = slave->status;
        if (status == STAT_DAMAGE || status == STAT_STATE || status == STAT_ATTACK)
        {
            return;
        }

        owner_attribute = item->owner->attribute;
        item->owner->attribute = ATTR_SUSPEND;
        target = GetNearestHumanoid(param->slave,
                                    NINKEN_TARGET_SEARCH_RADIUS);
        item->owner->attribute = owner_attribute;
        if (target != 0)
        {
            if (&target->model->locate == param->slave->target)
            {
                return;
            }
            SetupThinkFunction(param->slave, THINK_MIX_NINKEN);
            param->slave->target = &target->model->locate;
            param->slave->attribute |= PHASE_ALERT;
            EquipWeapon(param->slave, WEAPON_DRAWN);
            SetNowMotion(param->slave, MOT_STATE_DRAW, MOTION_MOVE_APPLY);
            return;
        }
        else
        {
            if (param->slave->target == &item->locate->locate)
            {
                return;
            }
            EquipWeapon(param->slave, WEAPON_SHEATHED);
            SetNowMotion(param->slave, MOT_STATE_SHEATHE, MOTION_MOVE_APPLY);
            param->slave->attribute &= ~ATTR_PHASE;
            SetupThinkFunction(param->slave, THINK_MIX_NONE);
            param->slave->target = &item->owner->model->locate;
            return;
        }
    }
    }
    }
    return;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemNinken(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2461, 21 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_ninken * param
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
 *     reg   $a0       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

static int ReqItemNinken(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    param_ninken *param;
    s32 x;
    s32 z;

    item = TakeItemSlot();
    param = &item->param.ninken;
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemNinken);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
    }
    {
        param_korogari *param; /* Shadows the outer launch parameter in retail. */

        param = &item->param.ninken.koro;
        x = p->end.vx;
        z = p->end.vz;
        setVector(param, x, -250, z);
        item->param.ninken.koro.hint = 0;
        param->status = KORO_NORMAL;
    }
    param->slave = 0;
    param->count = 15;
    SetNowMotion(item->owner, MOT_ITEM_THROW, MOTION_MOVE_APPLY);
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemHappou(struct tag_TItem *item);
 *     ITEM.C:2486, 52 src lines, frame 40 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct tag_TItem * item
 *     reg   $s2       struct ModelType * model
 *     reg   $s1       struct param_launch * param
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $s0       struct tag_TItem * item
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s1       struct Humanoid * m
 *     reg   $s1       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType *HappouModel;
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */

void ProcItemHappou(TItem *item)
{
    enum
    {
        HAPPOU_COLLISION_RADIUS = 300,
        HAPPOU_WALL_BLEED_GROUND_RANGE = 0,
        HAPPOU_WALL_BLEED_SPREAD = 25,
        HAPPOU_WALL_BLEED_COUNT = 10,
        HAPPOU_WALL_BLEED_TIME = 10,
        HAPPOU_HIT_IMPACT_SIZE = 4 * FIXED_ONE
    };
    ModelType *model = HappouModel;
    param_launch *param = &item->param.launch;
    s32 collision_result;
    s32 registered_collision;

    if (item->mode == ITEM_MODE_DISPOSE)
    {
        DisposeAfterimage(param->effect);
        item->mode = ITEM_MODE_START;
        return;
    }
    MoveFly(item, &param->fly);
    if (--param->count == 0)
    {
        DeleteConflict(item->locate);
        registered_collision = InsertConflict(item->locate);
        SET_ITEM_COLLISION(registered_collision, HAPPOU_COLLISION_RADIUS,
                           CONFLICT_OWNER_ITEM,
                           CONFLICT_HIT);
    }
    UpdateCoordinate(item->locate);
    model->locate = item->locate->locate;
    DrawModel(model);
    DrawAfterimage(param->effect, 1);
    switch (param->fly.mode)
    {
    case FLY_MODE_ARC:
        break;

    case FLY_MODE_ROLL:
        if (param->fly.p.koro.status != KORO_NORMAL)
        {
            SetBleeds(MODEL_POSITION(item->locate),
                      HAPPOU_WALL_BLEED_GROUND_RANGE,
                      HAPPOU_WALL_BLEED_SPREAD,
                      HAPPOU_WALL_BLEED_COUNT,
                      HAPPOU_WALL_BLEED_TIME, COLOR_YELLOW);
            SoundEx(MODEL_POSITION(item->locate), SE_PROJECTILE_IMPACT);
            if (item->proc != 0)
            {
                DISPOSE_ITEM(item);
            }
        }
        break;
    }
    if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        collision_result = CONFLICT_NONE;
    else
        collision_result = GetConflictResult(item->locate, CONFLICT_NONE);
    if (collision_result != CONFLICT_NONE &&
        is_humanoid_on_stage_(ConflictObject[collision_result].common) != 0)
    {
        SetImpact(MODEL_POSITION(item->locate), HAPPOU_HIT_IMPACT_SIZE,
                  IMPACT_SPRITE_HIT);
        SoundEx(MODEL_POSITION(item->locate), SE_PROJECTILE_HIT);
        DeleteConflict(item->locate);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemHappou(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2541, 41 src lines, frame 72 bytes, saved-reg mask 0x80ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s4       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s5       int i
 *     reg   $s1       struct tag_TItem * item
 *     reg   $s3       struct param_launch * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $s2       struct VECTOR * pos
 *     stack sp+24     struct SVECTOR rot
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */


static int ReqItemHappou(PARAM_ITEM_LAUNCH *p)
{
    enum
    {
        HAPPOU_SHOT_COUNT = 8,
        HAPPOU_AIM_HALF_RANGE = 256,
        HAPPOU_HORIZONTAL_ARC_SPREAD = FIXED_ONE,
        HAPPOU_VERTICAL_ARC_SPREAD = FIXED_QUARTER,
        HAPPOU_FLIGHT_SPEED = 400,
        HAPPOU_AFTERIMAGE_LENGTH = 10,
        HAPPOU_AFTERIMAGE_HALF_WIDTH = 30,
        HAPPOU_ARMING_DELAY = 8
    };
    TItem *item;
    TItem *ret;
    param_launch *param;
    VECTOR *pos;
    VECTOR *trajectory_target;
    Humanoid *aowner;
    s32 atype;
    AfterimageType *afterimage;
    SVECTOR rot;
    s32 random_yaw;
    s32 i;

    SetNowMotion(p->user, MOT_ITEM_THROW, MOTION_MOVE_APPLY);
    i = 0;
    while (1)
    {
        if (i >= HAPPOU_SHOT_COUNT)
            break;
        {
            s32 i;

            TAKE_ITEM_SLOT_VIA_CURSOR(found);
        }

    found:
        param = &item->param.launch;
        if (item == 0)
            return 0;
        INITIALIZE_ITEM_FROM_REQUEST(ProcItemHappou);
        item->collision.size = 0;
        item->locate->rotate = p->user->model->rotate;
        rot = p->user->model->rotate;
        random_yaw = rand();
        rot.vy += random_yaw % (HAPPOU_AIM_HALF_RANGE * 2) -
                  HAPPOU_AIM_HALF_RANGE;
        trajectory_target = &p->end;
        SearchItemTarget2(p->user, &rot, pos, trajectory_target);
        SetupFly(&param->fly, pos, trajectory_target,
                 HAPPOU_HORIZONTAL_ARC_SPREAD,
                 HAPPOU_VERTICAL_ARC_SPREAD,
                 HAPPOU_FLIGHT_SPEED);
        afterimage = SetupAfterimage(item->locate,
                                    HAPPOU_AFTERIMAGE_LENGTH);
        param->effect = afterimage;
        setVector(&afterimage->vector1,
                  HAPPOU_AFTERIMAGE_HALF_WIDTH, 0, 0);
        setVector(&afterimage->vector2,
                  -HAPPOU_AFTERIMAGE_HALF_WIDTH, 0, 0);
        param->count = HAPPOU_ARMING_DELAY;
        i++;
    }
    Sound(p->user, SE_ITEM_USE);
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemFire(struct tag_TItem *item);
 *     ITEM.C:2586, 116 src lines, frame 176 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct tag_TItem * item
 *     reg   $s4       struct Sprite3D * model
 *     reg   $s5       struct param_smoke * param
 *     reg   $s2       struct tag_TItem * item
 *     stack sp+24     struct VECTOR pos
 *     stack sp+40     struct SVECTOR vec
 *     stack sp+56     struct PARAM_ITEM_STAY rparam
 *     reg   $s2       struct tag_TItem * item
 *     stack sp+104    struct PARAM_ITEM_LAUNCH param
 *     reg   $a0       int cid
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       struct ModelType * model
 *     reg   $v0       struct Humanoid * human
 *     stack sp+48     struct SVECTOR vec
 *     stack sp+80     struct VECTOR pos
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $s2       struct tag_TItem * item
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s0       struct Humanoid * human
 *     reg   $s0       struct Humanoid * human
 *     reg   $s1       struct ModelType * model
 *     stack sp+96     struct VECTOR pos
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */


void ProcItemFire(TItem *item)
{
    enum
    {
        FIRE_MODE_FUSE = 0,
        FIRE_MODE_EXPLODE = 1,
        FIRE_MODE_BLAST = 2,
        FIRE_EMBER_POSITION_JITTER = 25,
        FIRE_EMBER_RISE_SPEED = -30,
        FIRE_EMBER_SIZE_RANGE = 20,
        FIRE_RECOVERY_ROLL_RANGE = 10,
        FIRE_RECOVERY_ROLL_THRESHOLD = 2,
        FIRE_RECOVERY_SMOKE_RISE_SPEED = -100,
        FIRE_RECOVERY_SMOKE_TIME = 10,
        FIRE_TRIGGER_ARM_FRAME = 140,
        FIRE_TRIGGER_RADIUS = 500,
        FIRE_EXPLOSION_RISE_SPEED = -25,
        FIRE_SPARK_HORIZONTAL_SPEED = 75,
        FIRE_SPARK_UPWARD_SPEED = 120,
        FIRE_SPARK_COUNT = 8,
        FIRE_SMOKE_RISE_SPEED = -200,
        FIRE_SMOKE_COUNT = 20,
        FIRE_SMOKE_TIME = 6,
        FIRE_BLAST_RADIUS = 1500,
        FIRE_BLAST_FRAMES = 3,
        FIRE_HIT_FRAME_POSITION_SPREAD = 200,
        FIRE_HIT_FRAME_SIZE = 3 * FIXED_ONE,
        FIRE_HIT_FRAME_TIME = 120
    };
    Sprite3D *model = (Sprite3D *)item->model;
    param_smoke *param = &item->param.smoke;
    s32 remaining_count;
    s32 collision_result;

    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = FIRE_MODE_FUSE;
        return;
    }

    MoveKorogari(item, &param->koro);
    if (param->koro.status == KORO_WATER)
    {
        if (item->proc != 0)
        {
            DISPOSE_ITEM(item);
        }
        return;
    }

    UpdateCoordinate(item->locate);
    model->locate = item->locate->locate;
    DrawSprite(model);

    {
        {
            VECTOR pos = {
                .vx = item->locate->locate.coord.t[0] +
                    (rand() % (FIRE_EMBER_POSITION_JITTER * 2) -
                     FIRE_EMBER_POSITION_JITTER),
                .vy = item->locate->locate.coord.t[1] +
                    (rand() % (FIRE_EMBER_POSITION_JITTER * 2) -
                     FIRE_EMBER_POSITION_JITTER),
                .vz = item->locate->locate.coord.t[2] +
                    (rand() % (FIRE_EMBER_POSITION_JITTER * 2) -
                     FIRE_EMBER_POSITION_JITTER)
            };
            SVECTOR vec = {
                .vx = 0,
                .vy = FIRE_EMBER_RISE_SPEED,
                .vz = 0
            };

            SetBleed(&pos, &vec,
                     rand() % FIRE_EMBER_SIZE_RANGE, COLOR_YELLOW);
        }
    }

    remaining_count = param->count - 1;
    param->count = remaining_count;
    switch (item->mode)
    {
    case FIRE_MODE_FUSE:
        if ((u8)remaining_count == 0)
        {
            if (rand() % FIRE_RECOVERY_ROLL_RANGE <
                FIRE_RECOVERY_ROLL_THRESHOLD)
            {
                PARAM_ITEM_STAY saved_record;
                PARAM_ITEM_STAY stay_request;

                stay_request = (PARAM_ITEM_STAY){0};
                stay_request.type = item->type;
                copyVector(&stay_request.locate, MODEL_POSITION(model));
                saved_record = stay_request;

                if (item->proc != 0)
                {
                    DISPOSE_ITEM(item);
                }

                DropStayedItem(&saved_record);
                SetSmokeS(&saved_record.locate, 0,
                          FIRE_RECOVERY_SMOKE_RISE_SPEED, 0,
                          FIRE_RECOVERY_SMOKE_TIME);
                return;
            }
        }
        else
        {
            if ((u8)remaining_count == FIRE_TRIGGER_ARM_FRAME)
            {
                s32 conflict_id;
                s32 size;
                ConflictClass collision_mode;

                DeleteConflict(item->locate);
                conflict_id = InsertConflict(item->locate);
                size = FIRE_TRIGGER_RADIUS;
                collision_mode = CONFLICT_SOFT;
                SET_ITEM_COLLISION(conflict_id, size, CONFLICT_OWNER_ITEM,
                                   collision_mode);
            }

            if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
            {
                collision_result = CONFLICT_NONE;
            }
            else
            {
                collision_result =
                    GetConflictResult(item->locate, CONFLICT_NONE);
            }
            if (collision_result == CONFLICT_NONE)
            {
                return;
            }
            if (is_humanoid_on_stage_(
                    ConflictObject[collision_result].common) == 0 &&
                ConflictObject[collision_result].size.pad !=
                    CONFLICT_HIT)
            {
                return;
            }
        }
        item->mode++;
        return;

    case FIRE_MODE_EXPLODE:
    {
        {
            s32 conflict_id;
            SVECTOR vec = {
                .vx = 0,
                .vy = FIRE_EXPLOSION_RISE_SPEED,
                .vz = 0
            };
            VECTOR pos = {
                .vx = item->locate->locate.coord.t[0],
                .vy = item->locate->locate.coord.t[1],
                .vz = item->locate->locate.coord.t[2]
            };

            SetExplosion(&pos, &vec);

            setVector(&vec, FIRE_SPARK_HORIZONTAL_SPEED,
                      FIRE_SPARK_UPWARD_SPEED,
                      FIRE_SPARK_HORIZONTAL_SPEED);
            SetHinoko(&pos, &vec, FIRE_SPARK_COUNT);
            setVector(&vec, 0, FIRE_SMOKE_RISE_SPEED, 0);
            SetSmoke(&pos, &vec, FIRE_SMOKE_COUNT, FIRE_SMOKE_TIME);
            SoundEx(&pos, SE_EXPLOSION);

            DeleteConflict(item->locate);
            conflict_id = InsertConflict(item->locate);
            SET_ITEM_COLLISION(conflict_id, FIRE_BLAST_RADIUS,
                               CONFLICT_OWNER_ITEM, CONFLICT_HIT);
            item->mode++;
            param->count = FIRE_BLAST_FRAMES;
            reset_alert_duration();
            return;
        }
    }

    case FIRE_MODE_BLAST:
        if ((u8)remaining_count == 0 && item->proc != 0)
        {
            DISPOSE_ITEM(item);
        }

        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        {
            collision_result = CONFLICT_NONE;
        }
        else
        {
            collision_result =
                GetConflictResult(item->locate, CONFLICT_NONE);
        }
        if (collision_result != CONFLICT_NONE)
        {
            Humanoid *human = ConflictObject[collision_result].common;

            if (is_humanoid_on_stage_(human) != 0)
            {
                ModelType **objects;
                ModelType *hit_model;

                objects = human->model->object;
                if (human->model->n > 0)
                {
                    objects += rand() % human->model->n;
                }
                hit_model = *objects;
                {
                    VECTOR frame_position = {
                        .vx = rand() % FIRE_HIT_FRAME_POSITION_SPREAD -
                              FIRE_HIT_FRAME_POSITION_SPREAD / 2,
                        .vy = rand() % FIRE_HIT_FRAME_POSITION_SPREAD -
                              FIRE_HIT_FRAME_POSITION_SPREAD / 2,
                        .vz = rand() % FIRE_HIT_FRAME_POSITION_SPREAD -
                              FIRE_HIT_FRAME_POSITION_SPREAD / 2
                    };

                    SetFrame(&frame_position, FIRE_HIT_FRAME_SIZE,
                             FIRE_HIT_FRAME_TIME,
                             &hit_model->locate);
                }
            }
        }
        return;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemFire(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2706, 18 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_smoke * param
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
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

int ReqItemFire(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    param_smoke *param;
    s32 x;
    s32 y;
    s32 z;

    item = TakeItemSlot();
    param = &item->param.smoke;
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemFire);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
    }
    {
        param_korogari *param;

        param = &item->param.smoke.koro;
        x = p->end.vx;
        y = p->end.vy;
        z = p->end.vz;
        setVector(param, x, y, z);
        item->param.smoke.koro.hint = 0;
        param->status = KORO_NORMAL;
    }
    param->count = 150;
    return 1;
}


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemNemuri(struct tag_TItem *item);
 *     ITEM.C:2738, 93 src lines, frame 112 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct tag_TItem * item
 *     reg   $s4       struct Sprite3D * model
 *     reg   $s3       struct param_napalm * param
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       struct VECTOR * pos
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $s1       int env
 *     reg   $v0       int bright
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s3       struct Humanoid * human
 *     reg   $s3       struct Humanoid * human
 *     stack sp+16     struct VECTOR pos
 *     stack sp+32     struct SVECTOR vec
 *     stack sp+48     struct VECTOR pos
 *     reg   $s3       struct Humanoid * human
 *     reg   $s2       struct tag_TItem * item
 *     reg   $s2       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

void ProcItemNemuri(TItem *item)
{
    enum
    {
        NEMURI_MODE_START = 0,
        NEMURI_MODE_THROW = 1,
        NEMURI_MODE_FLY = 2,
        NEMURI_MODE_FINISH = 3,
        NEMURI_RELEASE_FRAME = 3,
        NEMURI_COLLISION_SIZE = 1000,
        NEMURI_PULSE_STEP = 0x88,
        NEMURI_PULSE_SHIFT = 6,
        NEMURI_BASE_BRIGHTNESS = 0x80,
        NEMURI_BASE_SCALE = 4 * FIXED_ONE,
        NEMURI_SCALE_PER_BRIGHTNESS = 2,
        NEMURI_ROTATION_STEP = 45 * FIXED_ONE,
        NEMURI_BLEED_RANGE = 300,
        NEMURI_BLEED_SPREAD = 10,
        NEMURI_BLEED_COUNT = 2,
        NEMURI_BLEED_LIFETIME = 10,
        NEMURI_BLEED_COLOR = RGB24(110, 110, 110),
        NEMURI_HIT_JITTER_SPREAD = 200,
        NEMURI_HIT_JITTER_RADIUS = NEMURI_HIT_JITTER_SPREAD / 2,
        NEMURI_SMOKE_RISE_SPEED = 150,
        NEMURI_SMOKE_PARTICLES = 10,
        NEMURI_SMOKE_LIFETIME = 30,
        NEMURI_MAX_FLIGHT_COUNT = 100
    };
    Sprite3D *model;
    param_napalm *param;
    s32 rotation_count;

    model = (Sprite3D *)item->model;
    param = &item->param.napalm;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = NEMURI_MODE_START;
        return;
    }

    switch (item->mode)
    {
    case NEMURI_MODE_START:
        SetNowMotion(item->owner, MOT_ITEM_THROW, MOTION_MOVE_APPLY);
        SoundEx(MODEL_POSITION(item->owner->model), SE_SLEEP_DART_THROW);
        item->mode++;
        return;

    case NEMURI_MODE_THROW:
        if (item->owner->motion->mid == MOT_ITEM_THROW)
        {
            if (item->owner->motion->count != NEMURI_RELEASE_FRAME)
            {
                return;
            }
            {
                VECTOR *pos;
                s32 new_conflict_id;
                ConflictClass conflict_class;

                pos = GetAbsolutePosition(
                    item->owner->model->object[MODEL_PART_WEAPON_HAND_1],
                    0, 0, 0);
                param->count = 0;
                item->mode++;
                copyVector(MODEL_POSITION(item->locate), pos);
                DeleteConflict(item->locate);
                new_conflict_id = InsertConflict(item->locate);
                conflict_class = CONFLICT_SOFT;
                SET_ITEM_COLLISION(new_conflict_id, NEMURI_COLLISION_SIZE,
                                   CONFLICT_OWNER_ITEM, conflict_class);
                return;
            }
        }
        if (item->proc == 0)
        {
            return;
        }
        DISPOSE_ITEM(item);
        return;

    case NEMURI_MODE_FLY:
    {
        s32 pulse;
        s32 brightness;
        s32 conflict_id;
        s32 inactive_sentinel;
        s32 bleed_color;
        s32 bleed_count;
        s32 bleed_range;
        Humanoid *hit_human;

        pulse = rsin(param->count * NEMURI_PULSE_STEP);
        /* Make the later signed shift truncate toward zero. */
        if (pulse < 0)
        {
            pulse += (1 << NEMURI_PULSE_SHIFT) - 1;
        }
        bleed_color = NEMURI_BLEED_COLOR;
        item->locate->locate.coord.t[0] +=
            item->param.napalm.vec.vx;
        bleed_range = NEMURI_BLEED_RANGE;
        bleed_count = NEMURI_BLEED_COUNT;
        item->locate->locate.coord.t[1] += param->vec.vy;
        item->locate->locate.coord.t[2] += param->vec.vz;
        brightness = (pulse >> NEMURI_PULSE_SHIFT) +
                     NEMURI_BASE_BRIGHTNESS;
        model->sprite.r = brightness;
        model->sprite.g = brightness;
        model->sprite.b = brightness;
        rotation_count = param->count;
        model->scale = brightness * NEMURI_SCALE_PER_BRIGHTNESS +
                       NEMURI_BASE_SCALE;
        model->sprite.rotate = rotation_count * NEMURI_ROTATION_STEP;
        SetBleeds(MODEL_POSITION(item->locate),
                  bleed_range, NEMURI_BLEED_SPREAD, bleed_count,
                  NEMURI_BLEED_LIFETIME, bleed_color);

        if (++param->count > NEMURI_MAX_FLIGHT_COUNT)
        {
            item->mode++;
        }

        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        {
            conflict_id = CONFLICT_NONE;
        }
        else
        {
            conflict_id = GetConflictResult(item->locate, CONFLICT_NONE);
        }
        inactive_sentinel = CONFLICT_NONE;
        if (conflict_id != inactive_sentinel)
        {
            hit_human = ConflictObject[conflict_id].common;
            if (is_humanoid_on_stage_(hit_human) != 0 &&
                hit_human != item->owner)
            {
                s16 hit_life;

                if (hit_human->model->n > 0)
                {
                    rand();
                }
                {
                    /* Retail computes this jittered position but never uses it. */
                    VECTOR random_position = {
                        .vx = rand() % NEMURI_HIT_JITTER_SPREAD -
                              NEMURI_HIT_JITTER_RADIUS,
                        .vy = rand() % NEMURI_HIT_JITTER_SPREAD -
                              NEMURI_HIT_JITTER_RADIUS,
                        .vz = rand() % NEMURI_HIT_JITTER_SPREAD -
                              NEMURI_HIT_JITTER_RADIUS
                    };

                    SoundEx(MODEL_POSITION(item->locate), SE_SMOKE_PUFF);
                    {
                        SVECTOR smoke_velocity = {
                            .vx = 0,
                            .vy = -NEMURI_SMOKE_RISE_SPEED,
                            .vz = 0
                        };
                        VECTOR smoke_position = {
                            .vx = hit_human->model->locate.coord.t[0],
                            .vy = hit_human->model->locate.coord.t[1],
                            .vz = hit_human->model->locate.coord.t[2]
                        };

                        SetSmoke(&smoke_position, &smoke_velocity,
                                 NEMURI_SMOKE_PARTICLES,
                                 NEMURI_SMOKE_LIFETIME);
                    }

                    hit_life = hit_human->life;
                    if (hit_life > 0 && hit_human->motion->mid != MOT_ACTION)
                    {
                        if ((hit_human->type & PAGE_MASK) != PAGE_BOSS &&
                            hit_life != inactive_sentinel)
                        {
                            EquipWeapon(hit_human, WEAPON_SHEATHED);
                            SetNowMotion(hit_human, MOT_STATE_SHEATHE, MOTION_MOVE_APPLY);
                            hit_human->think[0] = Think1sleep;
                            hit_human->attribute &= ~ATTR_PHASE;
                        }
                        SetNowMotion(hit_human, MOT_ACTION, MOTION_MOVE_APPLY);
                        Sound(hit_human, CHAR_VOICE_HURT);
                    }

                    if (item->proc == 0)
                    {
                        return;
                    }
                    DISPOSE_ITEM(item);
                    return;
                }
            }
        }

        if (GetAreaMapLevel(GlobalAreaMap,
                            item->locate->locate.coord.t[0],
                            item->locate->locate.coord.t[1],
                            item->locate->locate.coord.t[2],
                            AREA_LEVEL_DEFAULT) ==
            LEVEL_NONE)
        {
            if (item->proc == 0)
            {
                return;
            }
            DISPOSE_ITEM(item);
            return;
        }
        break;
    }

    case NEMURI_MODE_FINISH:
        if (item->proc == 0)
        {
            return;
        }
        DISPOSE_ITEM(item);
        return;
    }

    UpdateCoordinate(item->locate);
    model->locate = item->locate->locate;
    DrawSprite(model);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemNemuri(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2835, 17 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s2       struct param_napalm * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct Sprite3D *sprSmoke;
 * END PSX.SYM */

int ReqItemNemuri(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    param_napalm *param;

    item = TakeItemSlot();
    param = &item->param.napalm;
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemNemuri);
        item->model = (ModelType *)sprSmoke[SMOKE_SPRITE_NORMAL];
        item->collision.size = 0;
    }
    param->vec.vx = p->end.vx;
    param->vec.vy = p->end.vy;
    param->vec.vz = p->end.vz;
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemLightningBolt(struct tag_TItem *item);
 *     ITEM.C:2856, 57 src lines, frame 56 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct tag_TItem * item
 *     reg   $s2       struct param_lightningbolt * param
 *     stack sp+24     struct VECTOR target
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $s0       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern long GameClock;
 * END PSX.SYM */

void ProcItemLightningBolt(TItem *item)
{
    /* The bolt re-aims and re-strikes every third frame: STRIKE moves the
     * hit volume onto a fresh target, WAIT counts down to the next one. */
    enum
    {
        LIGHTNING_MODE_START = 0,
        LIGHTNING_MODE_STRIKE = 1,
        LIGHTNING_MODE_WAIT = 2,
        LIGHTNING_DURATION = 15,
        LIGHTNING_RESTRIKE_INTERVAL = 3,
        LIGHTNING_HIT_RADIUS = 100,
        LIGHTNING_COLOR_R = 100,
        LIGHTNING_COLOR_G = 100,
        LIGHTNING_COLOR_B = 200,
        LIGHTNING_FLASH_FRAME_MASK = 3,
        LIGHTNING_FLASH_GROUND_RANGE = 200,
        LIGHTNING_FLASH_SPREAD = 20,
        LIGHTNING_FLASH_BLEED_COUNT = 10,
        LIGHTNING_FLASH_BLEED_TIME = 20,
        LIGHTNING_FLASH_COLOR = RGB24(255, 255, 120),
        LIGHTNING_FLASH_IMPACT_SIZE = 4 * FIXED_ONE
    };
    param_lightningbolt *param = &item->param.lightningbolt;
    VECTOR target;
    u8 previous_count;
    s32 registered_collision;

    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = LIGHTNING_MODE_START;
        return;
    }
    switch (item->mode)
    {
    case LIGHTNING_MODE_START:
        param->count = LIGHTNING_DURATION;
        item->mode++;
        if (item->owner == CamState.Owner)
        {
            SoundEx(0, SE_LIGHTNING);
        }
        break;

    case LIGHTNING_MODE_STRIKE:
        SearchItemTarget2(item->owner, &param->rot,
                          &param->start, &target);
        copyVector(MODEL_POSITION(item->locate), &target);
        DeleteConflict(item->locate);
        registered_collision = InsertConflict(item->locate);
        SET_ITEM_COLLISION(registered_collision, LIGHTNING_HIT_RADIUS,
                           CONFLICT_OWNER_ITEM,
                           CONFLICT_HIT);
        item->mode++;
        break;

    case LIGHTNING_MODE_WAIT:
        if (GameClock % LIGHTNING_RESTRIKE_INTERVAL == 0)
        {
            item->mode = LIGHTNING_MODE_STRIKE;
        }
        break;
    }
    SetLightning(&param->start, MODEL_POSITION(item->locate),
                 LIGHTNING_COLOR_R,
                 LIGHTNING_COLOR_G,
                 LIGHTNING_COLOR_B);
    if ((GameClock & LIGHTNING_FLASH_FRAME_MASK) == 0)
    {
        SetBleeds(MODEL_POSITION(item->locate),
                  LIGHTNING_FLASH_GROUND_RANGE,
                  LIGHTNING_FLASH_SPREAD,
                  LIGHTNING_FLASH_BLEED_COUNT,
                  LIGHTNING_FLASH_BLEED_TIME,
                  LIGHTNING_FLASH_COLOR);
        SetImpact(&param->start, LIGHTNING_FLASH_IMPACT_SIZE,
                  IMPACT_SPRITE_FLASH);
    }
    previous_count = param->count--;
    if (previous_count == 0 && item->proc != 0)
    {
        DISPOSE_ITEM(item);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemLightningBolt(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2917, 17 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_lightningbolt * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

int ReqItemLightningBolt(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    TItem *ret;
    param_lightningbolt *param;
    int rx;
    int ry;
    s32 i;

    TAKE_ITEM_SLOT_VIA_CURSOR(found);
found:
    param = &item->param.lightningbolt;
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemLightningBolt);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
    }
    copyVector(&param->start, &p->start);
    GetVectorRotation(&p->start, &p->end, &rx, &ry);
    setVector(&param->rot, rx, ry, 0);
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemGun(struct tag_TItem *item);
 *     ITEM.C:2938, 72 src lines, frame 80 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct tag_TItem * item
 *     reg   $s2       struct param_gun * param
 *     stack sp+24     struct SVECTOR vec
 *     stack sp+32     struct VECTOR target
 *     reg   $s3       struct Humanoid * IsHuman
 *     reg   $s1       struct tag_TItem * item
 *     reg   $v0       int t
 *     stack sp+48     struct SVECTOR vec
 *     reg   $s1       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */


void ProcItemGun(TItem *item)
{
    enum
    {
        GUN_MODE_FLASH = 0,
        GUN_MODE_FIRE = 1,
        GUN_MODE_FINISH = 2
    };
    param_gun *param;
    SVECTOR vec;
    VECTOR target;

    param = &item->param.gun;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = GUN_MODE_FLASH;
        return;
    }
    switch (item->mode)
    {
    case GUN_MODE_FLASH:
        vec = (SVECTOR){
            .vx = 0,
            .vy = 0,
            .vz = -250
        };
        RotateVectorS(&vec, item->owner->model->rotate.vx, item->owner->model->rotate.vy, 0);
        SetImpact(MODEL_POSITION(item->locate), 2 * FIXED_ONE,
                  IMPACT_SPRITE_GUN);
        SetBleeds(MODEL_POSITION(item->locate), 100, 10, 10, 10, COLOR_GRAY_DARK);
        item->mode++;
        return;

    case GUN_MODE_FIRE:
    {
        s32 rx;
        s32 ry;
        Humanoid *IsHuman;
        s32 conflict_id;

        GetVectorRotation(MODEL_POSITION(item->locate), &param->vec, &rx, &ry);
        vec.vx = rx;
        vec.vy = ry;
        vec.vz = 0;
        IsHuman = SearchItemTarget2(item->owner, &vec, MODEL_POSITION(item->locate), &target);
        item->locate->locate.coord.t[0] = target.vx;
        item->locate->locate.coord.t[1] = target.vy;
        item->locate->locate.coord.t[2] = target.vz;
        DeleteConflict(item->locate);
        conflict_id = InsertConflict(item->locate);
        SET_ITEM_COLLISION(conflict_id, 100, CONFLICT_OWNER_ITEM,
                           CONFLICT_HIT);
        {
            SVECTOR vec = {
                .vx = 0,
                .vy = 0,
                .vz = 150
            };
            RotateVectorS(&vec, item->owner->model->rotate.vx, item->owner->model->rotate.vy, 0);
            if (IsHuman != 0)
            {
                SetImpact(&target, 6 * FIXED_ONE, IMPACT_SPRITE_GUN);
                SetBleedsDir(&target, &vec, 100, 15, 10, COLOR_RED);
                SoundEx(&target, SE_GUN_HIT_FLESH);
            }
            else
            {
                SetImpact(&target, 4 * FIXED_ONE, IMPACT_SPRITE_GUN);
                SetBleedsDir(&target, &vec, 100, 15, 10, COLOR_YELLOW);
                SoundEx(&target, SE_GUN_HIT_SOLID);
            }
        }
    }
        item->mode++;
        return;

    case GUN_MODE_FINISH:
        if (item->proc == 0)
            return;
        DISPOSE_ITEM(item);
        return;
    }
}

void ReqItemGun(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    VECTOR *pos;
    Humanoid *aowner;
    s32 atype;

    item = TakeItemSlot();
    if (item == 0)
        return;
    INITIALIZE_ITEM_FROM_REQUEST(ProcItemGun);
    item->collision.size = 0;
    item->param.gun.vec = p->end;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemNapalm(struct tag_TItem *item);
 *     ITEM.C:3014, 71 src lines, frame 80 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct tag_TItem * item
 *     reg   $s5       struct Sprite3D * model
 *     reg   $s3       struct param_napalm * param
 *     reg   $s0       int ex
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s0       struct Humanoid * human
 *     reg   $s0       struct Humanoid * human
 *     reg   $s1       struct ModelType * model
 *     stack sp+16     struct VECTOR pos
 *     reg   $s2       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct Sprite3D *sprNapalm2;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

void ProcItemNapalm(TItem *item)
{
    enum
    {
        NAPALM_MODE_START = 0,
        NAPALM_MODE_EXPAND = 1,
        NAPALM_MODE_FINISH = 2,
        NAPALM_MAX_COUNT = 20,
        NAPALM_POSITION_CURVE_DIVISOR = 100,
        NAPALM_BRIGHTNESS_JITTER = 25,
        NAPALM_BRIGHTNESS_BASE = 26,
        NAPALM_BRIGHTNESS_FADE = 230,
        NAPALM_COLOR_MAX = 0xff,
        NAPALM_SECONDARY_BRIGHTNESS_DIVISOR = 3,
        NAPALM_SCALE_DIVISOR = 50,
        NAPALM_COLLISION_FRAME = 10,
        NAPALM_COLLISION_RADIUS = 500,
        NAPALM_HIT_JITTER_SPREAD = 200,
        NAPALM_HIT_JITTER_RADIUS = NAPALM_HIT_JITTER_SPREAD / 2,
        NAPALM_HIT_FRAME_SIZE = 3 * FIXED_ONE,
        NAPALM_HIT_FRAME_LIFETIME = 60
    };
    Sprite3D *model;
    param_napalm *param;
    s32 progress_squared;
    s32 cid;

    model = (Sprite3D *)item->model;
    param = &item->param.napalm;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = NAPALM_MODE_START;
        return;
    }

    switch (item->mode)
    {
    case NAPALM_MODE_START:
        param->count = 0;
        item->mode++;
        return;

    case NAPALM_MODE_EXPAND:
    {
        u8 brightness;

        progress_squared = param->count * param->count;
        item->locate->locate.coord.t[0] +=
            param->vec.vx * progress_squared / NAPALM_POSITION_CURVE_DIVISOR;
        item->locate->locate.coord.t[1] +=
            param->vec.vy * progress_squared / NAPALM_POSITION_CURVE_DIVISOR;
        item->locate->locate.coord.t[2] +=
            param->vec.vz * progress_squared / NAPALM_POSITION_CURVE_DIVISOR;

        brightness = rand() % NAPALM_BRIGHTNESS_JITTER -
                     NAPALM_BRIGHTNESS_BASE -
                     param->count * NAPALM_BRIGHTNESS_FADE / NAPALM_MAX_COUNT;
        model->sprite.r = brightness;
        model->sprite.g = model->sprite.r;
        model->sprite.b = model->sprite.r;
        model->sprite.rotate = (rand() % 360) * FIXED_ONE;
        model->scale = progress_squared * FIXED_ONE /
                           NAPALM_SCALE_DIVISOR +
                       FIXED_ONE;

        sprNapalm2->sprite.r =
            (NAPALM_COLOR_MAX - model->sprite.r) /
            NAPALM_SECONDARY_BRIGHTNESS_DIVISOR;
        sprNapalm2->sprite.g = sprNapalm2->sprite.r;
        sprNapalm2->sprite.b = sprNapalm2->sprite.r;
        sprNapalm2->sprite.rotate = model->sprite.rotate;
        sprNapalm2->scale = model->scale;

        if (param->count == NAPALM_COLLISION_FRAME)
        {
            s32 conflict_id;

            DeleteConflict(item->locate);
            conflict_id = InsertConflict(item->locate);
            SET_ITEM_COLLISION(conflict_id, NAPALM_COLLISION_RADIUS,
                               CONFLICT_OWNER_ITEM,
                               CONFLICT_HIT);
        }

        if (++param->count > NAPALM_MAX_COUNT)
        {
            item->mode++;
        }

        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        {
            cid = CONFLICT_NONE;
        }
        else
        {
            cid = GetConflictResult(item->locate, CONFLICT_NONE);
        }
        if (cid != CONFLICT_NONE)
        {
            Humanoid *human;

            human = ConflictObject[cid].common;
            if (is_humanoid_on_stage_(human) != 0)
            {
                ModelType **objects;
                ModelType *model;

                objects = human->model->object;
                if (human->model->n > 0)
                {
                    objects += rand() % human->model->n;
                }
                model = *objects;
                {
                    VECTOR pos = {
                        .vx = rand() % NAPALM_HIT_JITTER_SPREAD -
                              NAPALM_HIT_JITTER_RADIUS,
                        .vy = rand() % NAPALM_HIT_JITTER_SPREAD -
                              NAPALM_HIT_JITTER_RADIUS,
                        .vz = rand() % NAPALM_HIT_JITTER_SPREAD -
                              NAPALM_HIT_JITTER_RADIUS
                    };

                    SetFrame(&pos, NAPALM_HIT_FRAME_SIZE,
                             NAPALM_HIT_FRAME_LIFETIME,
                             &model->locate);
                }
            }
        }

        if (GetAreaMapLevel(GlobalAreaMap,
                            item->locate->locate.coord.t[0],
                            item->locate->locate.coord.t[1],
                            item->locate->locate.coord.t[2],
                            AREA_LEVEL_DEFAULT) ==
            LEVEL_NONE)
        {
            if (item->proc == 0)
            {
                return;
            }
            DISPOSE_ITEM(item);
            return;
        }
        break;
    }

    case NAPALM_MODE_FINISH:
        if (item->proc == 0)
        {
            return;
        }
        DISPOSE_ITEM(item);
        return;
    }

    UpdateCoordinate(item->locate);
    sprNapalm2->locate = item->locate->locate;
    DrawSprite(sprNapalm2);
    model->locate = item->locate->locate;
    DrawSprite(model);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemLaunch(struct tag_TItem *item);
 *     ITEM.C:3089, 83 src lines, frame 128 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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



void ProcItemLaunch(TItem *item)
{
    enum
    {
        PROJECTILE_COLLISION_SIZE = 300,
        PROJECTILE_SPIN_STEP = ANGLE_FULL / 6,
        PROJECTILE_IMPACT_SIZE = 4 * FIXED_ONE,
        WALL_SPARK_SPREAD = 25,
        WALL_SPARK_COUNT = 10,
        WALL_SPARK_LIFETIME = 10
    };
    ModelType *model;
    param_launch *param;
    u8 t;
    s32 cid;
    s32 conflict_id;
    PARAM_ITEM_LAUNCH *p;
    PARAM_ITEM_LAUNCH rparam;

    model = item->model;
    param = &item->param.launch;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        DisposeAfterimage(param->effect);
        item->mode = ITEM_MODE_START;
        return;
    }
    MoveFly(item, &param->fly);
    t = param->count - 1;
    param->count = t;
    if (t == 0)
    {
        DeleteConflict(item->locate);
        conflict_id = InsertConflict(item->locate);
        SET_ITEM_COLLISION(conflict_id, PROJECTILE_COLLISION_SIZE,
                           CONFLICT_OWNER_ITEM,
                           CONFLICT_HIT);
    }
    item->locate->rotate.vx = 0;
    item->locate->rotate.vy = GameClock * PROJECTILE_SPIN_STEP;
    item->locate->rotate.vz = 0;
    UpdateCoordinate(item->locate);
    model->locate = item->locate->locate;
    DrawModel(model);
    DrawAfterimage(param->effect, 1);
    if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        cid = CONFLICT_NONE;
    else
        cid = GetConflictResult(item->locate, CONFLICT_NONE);
    if (cid != CONFLICT_NONE &&
        is_humanoid_on_stage_(ConflictObject[cid].common) != 0)
    {
        SetImpact(MODEL_POSITION(item->locate), PROJECTILE_IMPACT_SIZE,
                  IMPACT_SPRITE_HIT);
        SoundEx(MODEL_POSITION(item->locate), SE_PROJECTILE_HIT);
        if (item->proc != 0)
        {
            DISPOSE_ITEM(item);
        }
        return;
    }
    if (param->fly.mode == FLY_MODE_ARC)
        return;
    switch (param->fly.p.koro.status)
    {
    case KORO_WALL:
        SetBleeds(MODEL_POSITION(item->locate), 0, WALL_SPARK_SPREAD,
                  WALL_SPARK_COUNT, WALL_SPARK_LIFETIME, COLOR_YELLOW);
        SoundEx(MODEL_POSITION(item->locate), SE_PROJECTILE_IMPACT);
        reset_alert_duration();
        return;

    case KORO_GRAND:
    case KORO_STAY:
    {
        PARAM_ITEM_LAUNCH param;

        p = &param;
        *p = (PARAM_ITEM_LAUNCH){0};
        param.type = item->type;
        param.user = item->owner;
        param.start.vx = model->locate.coord.t[0];
        param.start.vy = model->locate.coord.t[1];
        param.start.vz = model->locate.coord.t[2];
        rparam = *p;
        if (item->proc != 0)
        {
            DISPOSE_ITEM(item);
        }
        ReqItemDrop(&rparam);
        return;
    }

    case KORO_WATER:
        if (item->proc != 0)
        {
            DISPOSE_ITEM(item);
        }
        return;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void UpdateItemState(void);
 *     ITEM.C:3176, 25 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s4       int i
 *     reg   $s1       int mode
 *     reg   $v0       int t
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

/* Per-axis item activation distance from ITEM.C's anonymous enum. */
enum
{
    LEN = 15000
};

static void UpdateItemState(void)
{
    ConflictObjectType *conflicts;
    TItem *item;
    GsRVIEW2 *view;
    s32 i;
    s32 hit;
    s32 sz, ofsY;
    ConflictClass mode;
    s16 conflict_id;

    i = 0;
    view = &ViewInfo;
    conflicts = ConflictObject;
    item = items;
loop:
    if (i < MAX_ITEMS)
    {
        if (item->proc != 0)
        {
            hit = 0;
            if (abs(ViewInfo.vpx - item->locate->locate.coord.t[0]) < LEN &&
                abs(view->vpy - item->locate->locate.coord.t[1]) < LEN)
            {
                hit = abs(view->vpz - item->locate->locate.coord.t[2]) < LEN;
            }
            if (hit)
            {
                if (item->collision.pause != ITEM_COLLISION_ACTIVE)
                {
                    sz = item->collision.size;
                    if (sz != 0)
                    {
                        ofsY = item->collision.ofsY;
                        mode = item->collision.mode;
                        DeleteConflict(item->locate);
                        conflict_id = InsertConflict(item->locate);
                        conflicts[conflict_id].offset.vx = 0;
                        conflicts[conflict_id].offset.vz = 0;
                        conflicts[conflict_id].offset.vy = ofsY;
                        conflicts[conflict_id].size.vz = sz;
                        conflicts[conflict_id].size.vy = sz;
                        conflicts[conflict_id].size.vx = sz;
                        conflicts[conflict_id].common =
                            (void *)CONFLICT_OWNER_ITEM;
                        conflicts[conflict_id].size.pad =
                            mode;
                        item->collision.size = sz;
                        item->collision.ofsY = ofsY;
                        item->collision.mode = mode;
                        item->collision.pause = ITEM_COLLISION_ACTIVE;
                    }
                }
            }
            else if (item->collision.pause == ITEM_COLLISION_ACTIVE &&
                     item->locate->locate.super == 0)
            {
                item->collision.pause = ITEM_COLLISION_PAUSED;
                DeleteConflict(item->locate);
            }
        }
        item++;
        i++;
        goto loop;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ReqItemDefault(struct Humanoid *user, enum TItemType ItemID);
 *     ITEM.C:3261, 24 src lines, frame 96 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * user
 *     param $a1       enum TItemType ItemID
 *     stack sp+16     struct PARAM_ITEM_LAUNCH param
 *     stack sp+56     struct VECTOR v
 *     stack sp+72     struct VECTOR v0
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

extern VECTOR vec_z_n100[]; /* {0,0,-100} */

void ReqItemDefault(Humanoid *user, TItemType ItemID)
{
    PARAM_ITEM_LAUNCH param;
    VECTOR v;
    VECTOR v0;
    ModelArchiveType *pm;
    s32 rx;
    s32 ry;
    s32 rz;

    param.type = ItemID;
    param.user = user;
    param.start.vx = user->model->locate.coord.t[0];
    param.start.vy = user->model->locate.coord.t[1] - THROW_HEIGHT;
    param.start.vz = user->model->locate.coord.t[2];
    v = vec_z_n100[0];
    v0 = (VECTOR){0};
    pm = param.user->model;
    if (CamState.Owner->model == pm && CamState.Mode == CMODE_DIRECTION)
    {
        GetVectorRotation(CAMERA_VIEWPOINT(&ViewInfo),
                          CAMERA_REFERENCE(&ViewInfo), &rx, &ry);
        rz = 0;
    }
    else
    {
        rx = pm->rotate.vx;
        rz = pm->rotate.vz;
        ry = pm->rotate.vy;
    }
    RotateVector(&v, rx, ry, rz);
    param.end.vx = param.start.vx + v.vx;
    param.end.vy = param.start.vy + v.vy;
    param.end.vz = param.start.vz + v.vz;
    ReqItemUse(&param);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemLaunch(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:3289, 25 src lines, frame 48 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s1       struct tag_TItem * item
 *     reg   $s2       struct param_launch * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $s0       struct VECTOR * pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct ModelType *SyurikenModel;
 * END PSX.SYM */

int ReqItemLaunch(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    TItem *ret;
    param_launch *param;
    VECTOR *pos;
    AfterimageType *ai;
    s32 i;

    TAKE_ITEM_SLOT_VIA_CURSOR(found);
found:
    param = &item->param.launch;
    if (item == 0)
        return 0;
    {
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemLaunch);
        item->collision.size = 0;
        item->model = SyurikenModel;
    }
    SetupFly(&param->fly, pos, &p->end, FIXED_QUARTER, FIXED_QUARTER, 300);
    item->param.launch.fly.mode = FLY_MODE_ARC;
    ai = SetupAfterimage(item->model, 10);
    param->effect = ai;
    ai->vector1.vx = 0x14;
    ai->vector1.vy = 0;
    ai->vector1.vz = 0;
    ai->vector2.vx = -0x14;
    ai->vector2.vy = 0;
    ai->vector2.vz = 0;
    param->count = 5;
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ArrangeLocalMatrix(struct ModelType *model, struct MATRIX *t);
 *     ITEM.C:3328, 38 src lines, frame 72 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct ModelType * model
 *     param $s3       struct MATRIX * t
 *     stack sp+16     struct MATRIX m
 *     reg   $a1       int i
 *     reg   $t4       int j
 *     reg   $t5       int k
 *     reg   $s1       long det
 *     reg   $a3       long t
 *     reg   $t1       long u
 * END PSX.SYM */

static void ArrangeLocalMatrix(ModelType *model, MATRIX *t)
{
    enum
    {
        n = 3
    };
    MATRIX m;
    s32 i;
    s32 j;
    s32 k;
    s32 det;

    GsGetLw(&model->locate, &m);
    det = FIXED_ONE;

    i = 0;
    while (1)
    {
        s32 t;

        if (i >= n)
        {
            break;
        }
        t = m.m[i][i];
        if (t == 0)
        {
            t = FIXED_ONE;
        }
        det = det * t / FIXED_ONE;

        for (k = 0; k < n; k++)
        {
            m.m[i][k] = m.m[i][k] * FIXED_ONE / t;
        }
        m.m[i][i] = FIXED_ONE * FIXED_ONE / t;

        j = 0;
        while (1)
        {
            if (j >= n)
            {
                break;
            }
            if (j != i)
            {
                s32 u;

                u = m.m[j][i];
                for (k = 0; k < n; k++)
                {
                    if (k != i)
                    {
                        m.m[j][k] -= m.m[i][k] * u / FIXED_ONE;
                    }
                    else
                    {
                        m.m[j][i] = (-u * FIXED_ONE) / t;
                    }
                }
            }
            j++;
        }
        i++;
    }

    if (det >= FIXED_HALF && det <= FIXED_ONE)
    {
        MulMatrix(&m, t);
        *t = m;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemArrow(struct tag_TItem *item);
 *     ITEM.C:3367, 118 src lines, frame 168 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct tag_TItem * item
 *     reg   $s4       struct ModelType * model
 *     reg   $s3       struct param_arrow * param
 *     stack sp+24     struct VECTOR v1
 *     stack sp+40     struct VECTOR v2
 *     stack sp+136    int rx
 *     stack sp+140    int ry
 *     reg   $a0       int cid
 *     reg   $s1       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       struct ModelType * model
 *     reg   $s2       struct Humanoid * m
 *     reg   $s2       struct Humanoid * human
 *     stack sp+56     struct PARAM_ITEM_LAUNCH param
 *     reg   $s1       struct tag_TItem * item
 *     reg   $s0       struct ModelType * model
 *     reg   $s1       struct tag_TItem * item
 *     reg   $s1       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern long GameClock;
 * END PSX.SYM */


void ProcItemArrow(TItem *item)
{
    enum
    {
        ARROW_MODE_FLY = 0,
        ARROW_MODE_WAIT = 1,
        ARROW_MODE_BLINK = 2,
        ARROW_COLLISION_RADIUS = 300,
        ARROW_HIT_IMPACT_SIZE = 6 * FIXED_ONE,
        ARROW_EMBED_DURATION = 120,
        ARROW_GROUND_BLOOD_SPREAD = 25,
        ARROW_GROUND_BLOOD_PARTICLES = 30,
        ARROW_GROUND_BLOOD_LIFETIME = 30,
        ARROW_GROUND_WAIT_DURATION = 30,
        ARROW_BLINK_DURATION = 15,
        ARROW_BLINK_INTERVAL = 2,
        ARROW_ROLL_STEP = ANGLE_FULL / 16
    };
    ModelType *model;
    param_arrow *param;
    item_mode mode_index;
    VECTOR v1;
    VECTOR v2;
    int rx;
    int ry;

    model = item->model;
    param = &item->param.arrow;
    mode_index = item->mode;
    if (mode_index == ITEM_MODE_DISPOSE)
    {
        item->mode = ARROW_MODE_FLY;
        return;
    }

    switch (item->mode)
    {
    case ARROW_MODE_FLY:
    {
        s32 cid;

        copyVector(&v1, MODEL_POSITION(item->locate));
        MoveFly(item, &param->fly);
        if (--param->count == 0)
        {
            s32 conflict_id;

            DeleteConflict(item->locate);
            conflict_id = InsertConflict(item->locate);
            SET_ITEM_COLLISION(conflict_id, ARROW_COLLISION_RADIUS,
                               CONFLICT_OWNER_ITEM,
                               CONFLICT_HIT);
        }

        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        {
            cid = CONFLICT_NONE;
        }
        else
        {
            cid = GetConflictResult(item->locate, CONFLICT_NONE);
        }
        if (cid != CONFLICT_NONE)
        {
            Humanoid *human;

            human = ConflictObject[cid].common;
            if (is_humanoid_on_stage_(human) != 0)
            {
                if ((ConflictObject[cid].size.pad &
                     CONFLICT_HIT) != 0)
                {
                    if (item->proc == 0)
                    {
                        return;
                    }
                    DISPOSE_ITEM(item);
                    return;
                }
                else
                {
                    ModelType **models;
                    ModelType *model;

                    models = human->model->object;
                    if (human->model->n > 0)
                    {
                        models += rand() % human->model->n;
                    }
                    model = *models;
                    SetImpact(GetAbsolutePosition(model, 0, 0, 0),
                              ARROW_HIT_IMPACT_SIZE, IMPACT_SPRITE_HIT);
                    SoundEx(GetAbsolutePosition(model, 0, 0, 0), SE_PROJECTILE_HIT);
                    ArrangeLocalMatrix(model,
                                       &item->locate->locate.coord);
                    item->locate->locate.flg = 0;
                    item->locate->locate.super =
                        &model->locate;
                    item->locate->locate.coord.t[0] = 0;
                    item->locate->locate.coord.t[1] = 0;
                    item->locate->locate.coord.t[2] = 0;
                    param->count = ARROW_EMBED_DURATION;
                    item->mode++;
                    DeleteConflict(item->locate);
                    break;
                }
            }
        }
        else
        {
            if (param->fly.mode != FLY_MODE_ARC &&
                param->fly.p.koro.status != KORO_NORMAL)
            {
                if (param->fly.p.koro.status == KORO_WATER)
                {
                    if (item->proc == 0)
                    {
                        return;
                    }
                    DISPOSE_ITEM(item);
                    return;
                }
                SoundEx(MODEL_POSITION(item->locate), SE_PROJECTILE_IMPACT);
                SetBleeds(MODEL_POSITION(item->locate),
                          0, ARROW_GROUND_BLOOD_SPREAD,
                          ARROW_GROUND_BLOOD_PARTICLES,
                          ARROW_GROUND_BLOOD_LIFETIME, COLOR_YELLOW);
                param->count = ARROW_GROUND_WAIT_DURATION;
                item->mode++;
                DeleteConflict(item->locate);
                return;
            }
        }

        copyVector(&v2, MODEL_POSITION(item->locate));
        GetVectorRotation(&v1, &v2, &rx, &ry);
        item->locate->rotate.vx = rx;
        item->locate->rotate.vy = ry;
        {
            s32 clock;

            clock = GameClock;
            item->locate->rotate.vz = clock * ARROW_ROLL_STEP;
        }
        UpdateCoordinate(item->locate);
        break;
    }

    case ARROW_MODE_WAIT:
    {
        if (--param->count != 0)
        {
            break;
        }
        param->count = ARROW_BLINK_DURATION;
        item->mode++;
        break;
    }

    case ARROW_MODE_BLINK:
    {
        u8 count;

        count = --param->count;
        if (count == 0)
        {
            if (item->proc == 0)
            {
                return;
            }
            DISPOSE_ITEM(item);
            return;
        }
        if ((count & (ARROW_BLINK_INTERVAL - 1)) != 0)
        {
            return;
        }
        break;
    }
    }

    model->locate = item->locate->locate;
    DrawModel(model);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemArrow(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:3488, 16 src lines, frame 48 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s1       struct tag_TItem * item
 *     reg   $s3       struct param_arrow * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $s0       struct VECTOR * pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct ModelType *ArrowModel;
 * END PSX.SYM */

int ReqItemArrow(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    TItem *ret;
    param_arrow *param;
    VECTOR *pos;
    SVECTOR dir;
    VECTOR target;
    int rx;
    int ry;
    s32 i;

    GetVectorRotation(&p->start, &p->end, &rx, &ry);
    dir.vz = 0;
    dir.vx = rx;
    dir.vy = ry;
    SearchItemTarget2(p->user, &dir, &p->start, &target);
    TAKE_ITEM_SLOT_VIA_CURSOR(found);
found:
    param = &item->param.arrow;
    if (item == 0)
        return 0;
    {
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemArrow);
        item->collision.size = 0;
        item->model = ArrowModel;
    }
    SetupFly(&param->fly, pos, &target, 0, FIXED_HALF, 300);
    param->count = 5;
    return 1;
}


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemJirai(struct tag_TItem *item);
 *     ITEM.C:3527, 78 src lines, frame 104 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct tag_TItem * item
 *     reg   $s6       struct Sprite3D * model
 *     reg   $v1       struct param_smoke * param
 *     reg   $s2       struct tag_TItem * item
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s4       struct Humanoid * human
 *     reg   $s4       struct Humanoid * human
 *     reg   $s3       int i
 *     reg   $s0       struct ModelType * model
 *     stack sp+24     struct VECTOR pos
 *     stack sp+24     struct SVECTOR vec
 *     stack sp+32     struct VECTOR pos
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $s2       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct AreaNodeType *FieldArea;
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */

void ProcItemJirai(TItem *item)
{
    enum
    {
        JIRAI_MODE_PLACE = 0,
        JIRAI_MODE_ARMED = 1,
        JIRAI_MODE_EXPLODE = 2,
        JIRAI_MODE_BLAST = 3,
        JIRAI_TRIGGER_RADIUS = 500,
        JIRAI_BLAST_RADIUS = 1500,
        JIRAI_BLAST_COUNTDOWN_START = 3,
        JIRAI_FRAME_EFFECT_COUNT = 10,
        JIRAI_EXPLOSION_RISE_SPEED = 25,
        JIRAI_EMBER_HORIZONTAL_POWER = 75,
        JIRAI_EMBER_VERTICAL_POWER = 200,
        JIRAI_EMBER_COUNT = 10,
        JIRAI_SMOKE_RISE_SPEED = 400,
        JIRAI_SMOKE_COUNT = 20,
        JIRAI_SMOKE_LIFETIME = 6,
        JIRAI_FRAME_JITTER_SPREAD = 200,
        JIRAI_FRAME_JITTER_RADIUS = JIRAI_FRAME_JITTER_SPREAD / 2,
        JIRAI_FRAME_SIZE = 3 * FIXED_ONE,
        JIRAI_FRAME_LIFETIME_SPREAD = 60,
        JIRAI_FRAME_LIFETIME_MIN = 60,
        JIRAI_COUNTDOWN_END = 0xff
    };
    Sprite3D *sprite;
    param_smoke *param;

    sprite = (Sprite3D *)item->model;
    param = &item->param.smoke;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = JIRAI_MODE_PLACE;
        return;
    }

    switch (item->mode)
    {
    case JIRAI_MODE_PLACE:
    {
        ConflictClass conflict_class;
        s32 new_conflict_id;

        item->locate->locate.coord.t[1] =
            GetAreaMapLevel(GlobalAreaMap,
                            item->locate->locate.coord.t[0],
                            item->locate->locate.coord.t[1],
                            item->locate->locate.coord.t[2],
                            AREA_LEVEL_STEP_DOWN);
        if (item->locate->locate.coord.t[1] == LEVEL_NONE ||
            ((u16)FieldArea->attribute & MAP_WATER) != 0)
        {
            u8 item_count;

            item_count = item->owner->item[item->type];
            if (item_count != ITEM_INFINITE)
            {
                item->owner->item[item->type] = item_count + 1;
            }
            if (item->proc == 0)
            {
                return;
            }
            DISPOSE_ITEM(item);
            return;
        }

        DeleteConflict(item->locate);
        new_conflict_id = InsertConflict(item->locate);
        conflict_class = CONFLICT_SOFT;
        SET_ITEM_COLLISION(new_conflict_id, JIRAI_TRIGGER_RADIUS,
                           CONFLICT_OWNER_ITEM, conflict_class);
        item->mode++;
        break;
    }

    case JIRAI_MODE_ARMED:
    {
        s32 conflict_id;

        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        {
            conflict_id = CONFLICT_NONE;
        }
        else
        {
            conflict_id = GetConflictResult(item->locate, CONFLICT_NONE);
        }
        if (conflict_id != CONFLICT_NONE &&
            is_humanoid_on_stage_(
                ConflictObject[conflict_id].common) != 0)
        {
            s32 new_conflict_id;

            DeleteConflict(item->locate);
            new_conflict_id = InsertConflict(item->locate);
            SET_ITEM_COLLISION(new_conflict_id, JIRAI_BLAST_RADIUS,
                               CONFLICT_OWNER_ITEM, CONFLICT_HIT);
            item->mode++;
        }
        break;
    }

    case JIRAI_MODE_EXPLODE:
    {
        {
            SVECTOR velocity = {
                .vx = 0,
                .vy = -JIRAI_EXPLOSION_RISE_SPEED,
                .vz = 0
            };
            VECTOR position = {
                .vx = item->locate->locate.coord.t[0],
                .vy = item->locate->locate.coord.t[1],
                .vz = item->locate->locate.coord.t[2]
            };

            SetExplosion(&position, &velocity);
            velocity.vx = JIRAI_EMBER_HORIZONTAL_POWER;
            velocity.vy = JIRAI_EMBER_VERTICAL_POWER;
            velocity.vz = JIRAI_EMBER_HORIZONTAL_POWER;
            SetHinoko(&position, &velocity, JIRAI_EMBER_COUNT);
            velocity.vx = 0;
            velocity.vy = -JIRAI_SMOKE_RISE_SPEED;
            velocity.vz = 0;
            SetSmoke(&position, &velocity, JIRAI_SMOKE_COUNT,
                     JIRAI_SMOKE_LIFETIME);
            SoundEx(&position, SE_EXPLOSION);
            item->mode++;
            param->count = JIRAI_BLAST_COUNTDOWN_START;
            reset_alert_duration();
            break;
        }
    }

    case JIRAI_MODE_BLAST:
    {
        s32 conflict_id;

        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        {
            conflict_id = CONFLICT_NONE;
        }
        else
        {
            conflict_id = GetConflictResult(item->locate, CONFLICT_NONE);
        }
        if (conflict_id != CONFLICT_NONE)
        {
            Humanoid *hit_human;

            hit_human = ConflictObject[conflict_id].common;
            if (is_humanoid_on_stage_(hit_human) != 0)
            {
                s32 frame_index = 0;

                while (1)
                {
                    ModelType **model_objects;
                    ModelType *model;
                    if (frame_index >= JIRAI_FRAME_EFFECT_COUNT)
                    {
                        break;
                    }
                    model_objects = hit_human->model->object;
                    if (hit_human->model->n > 0)
                    {
                        model_objects += rand() % hit_human->model->n;
                    }
                    model = *model_objects;
                    frame_index++;
                    {
                        VECTOR position = {
                            .vx = rand() % JIRAI_FRAME_JITTER_SPREAD -
                                  JIRAI_FRAME_JITTER_RADIUS,
                            .vy = rand() % JIRAI_FRAME_JITTER_SPREAD -
                                  JIRAI_FRAME_JITTER_RADIUS,
                            .vz = rand() % JIRAI_FRAME_JITTER_SPREAD -
                                  JIRAI_FRAME_JITTER_RADIUS
                        };

                        SetFrame(&position, JIRAI_FRAME_SIZE,
                                 rand() % JIRAI_FRAME_LIFETIME_SPREAD +
                                     JIRAI_FRAME_LIFETIME_MIN,
                                 &model->locate);
                    }
                }
            }
        }

        if (--param->count != JIRAI_COUNTDOWN_END)
        {
            return;
        }
        if (item->proc == 0)
        {
            return;
        }
        DISPOSE_ITEM(item);
        return;
    }
    }

    UpdateCoordinate(item->locate);
    sprite->locate = item->locate->locate;
    DrawSprite(sprite);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemJirai(struct PARAM_ITEM_DROP *p);
 *     ITEM.C:3609, 18 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct PARAM_ITEM_DROP * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_smoke * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       int atype
 *     reg   $a0       struct VECTOR * pos
 *     reg   $s1       struct param_korogari * param
 *     reg   $v1       int x
 *     reg   $a0       int y
 *     reg   $a1       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

int ReqItemJirai(PARAM_ITEM_DROP *p)
{
    TItem *item;
    param_smoke *param;
    s32 x;
    s32 y;
    s32 z;

    item = TakeItemSlot();
    param = &item->param.smoke;
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemJirai);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
    }
    x = p->vec.vx;
    y = p->vec.vy;
    z = p->vec.vz;
    param->koro.vx = x;
    param->koro.vy = y;
    param->koro.vz = z;
    item->param.smoke.koro.hint = 0;
    param->koro.status = KORO_NORMAL;
    return 1;
}

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
extern VECTOR vec_z_100[];         /* {0,0,100} */
extern VECTOR vec_z_n60[];         /* {0,0,-60} */
extern VECTOR vec_y_n120_z_n240[]; /* {0,-120,-240} */
extern VECTOR vec_z_n120[];        /* {0,0,-120} */
extern VECTOR vec_y_n120_z_n120[]; /* {0,-120,-120} */
extern VECTOR vec_z_n4096[];       /* {0,0,-4096} */
extern VECTOR vec_z_n1000[];       /* {0,0,-1000} */
extern VECTOR vec_z_n500[];        /* {0,0,-500} */

int ReqItemUse(PARAM_ITEM_LAUNCH *p)
{
    enum
    {
        MAKIBISHI_SCATTER_WIDTH = 100,
        MAKIBISHI_SCATTER_RADIUS = MAKIBISHI_SCATTER_WIDTH / 2,
        MAKIBISHI_SCATTER_COUNT = 5,
        SHURIKEN_ARM_DELAY = 5,
        KAGINAWA_CAMERA_PITCH = -(ANGLE_FULL / 12)
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
            if (i >= MAKIBISHI_SCATTER_COUNT)
                break;
            i++;
            param.drop.vec.vx = work.vector.vx +
                                rand() % MAKIBISHI_SCATTER_WIDTH -
                                MAKIBISHI_SCATTER_RADIUS;
            param.drop.vec.vy = work.vector.vy -
                                rand() % MAKIBISHI_SCATTER_RADIUS -
                                MAKIBISHI_SCATTER_RADIUS;
            param.drop.vec.vz = work.vector.vz +
                                rand() % MAKIBISHI_SCATTER_WIDTH -
                                MAKIBISHI_SCATTER_RADIUS;
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
            param->count = SHURIKEN_ARM_DELAY;
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
        CamState.DirectionRX = KAGINAWA_CAMERA_PITCH;
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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void TurnAroundAllItems(struct Humanoid *user);
 *     ITEM.C:4023, 10 src lines, frame 88 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s5       struct Humanoid * user
 *     reg   $s4       int i
 *     reg   $s1       int j
 *     reg   $s5       struct Humanoid * human
 *     reg   $s4       int itemID
 *     stack sp+16     struct PARAM_ITEM_LAUNCH p
 * END PSX.SYM */


void TurnAroundAllItems(Humanoid *user)
{
    s32 i;
    s32 j;

    i = 0;
    while (1)
    {
        if (i >= ITEM_N)
            break;
        j = 0;
        while (1)
        {
            VECTOR *pos;
            Humanoid *human;
            s32 itemID;

            if (j >= user->item[i])
                break;
            pos = GetAbsolutePosition(user->model->object[MODEL_PART_WAIST], 0, 0, 0);
            human = user;
            itemID = i;
            {
                PARAM_ITEM_LAUNCH p = {
                    .type = itemID,
                    .user = human
                };

                p.start.vx = pos->vx;
                p.start.vy = pos->vy;
                p.start.vz = pos->vz;
                p.end.vx = rand() % 200 - 100;
                p.end.vy = rand() % 100 - 200;
                p.end.vz = rand() % 200 - 100;
                ReqItemDrop(&p);
            }
            j++;
        }
        user->item[i] = 0;
        i++;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DoItemProc(void);
 *     ITEM.C:3205, 52 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

void DoItemProc(void)
{
    TItem *it;
    s32 i;

    if (fInitial == 0)
    {
        InitializeItem();
    }
    if (GameClock % 10 == 0)
    {
        UpdateItemState();
    }
    i = 0;
    it = items;
    while (1)
    {
        if (i >= MAX_ITEMS)
            break;
        if (it->proc != 0)
        {
            it->proc(it);
        }
        it++;
        i++;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcItemManebue(struct tag_TItem *item);
 *     ITEM.C:1256, 30 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct tag_TItem * item
 *     reg   $a0       struct param_drop * param
 *     reg   $s0       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern long EmergencyNotice;
 * END PSX.SYM */

static void ProcItemManebue(TItem *item)
{
    enum
    {
        MANEBUE_MODE_PLAY = 0,
        MANEBUE_MODE_WAIT = 1
    };
    param_drop *param;

    param = &item->param.drop;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = MANEBUE_MODE_PLAY;
        return;
    }
    switch (item->mode)
    {
    case MANEBUE_MODE_PLAY:
        EmergencyNotice = 0;
        item->owner->active_item = item->type;
        SoundEx(0, SE_LURE_FLUTE);
        param->count = MANEBUE_DURATION;
        item->mode++;
        return;
    case MANEBUE_MODE_WAIT:
        param->count--;
        if (param->count == 0)
        {
            item->owner->active_item = ACTIVE_ITEM_NONE;
            if (item->proc != 0)
            {
                DISPOSE_ITEM(item);
            }
        }
        break;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemStay(struct PARAM_ITEM_STAY *p);
 *     ITEM.C:1140, 27 src lines, frame 64 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct PARAM_ITEM_STAY * p
 *     stack sp+16     struct PARAM_ITEM_LAUNCH param
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

int ReqItemStay(PARAM_ITEM_STAY *p)
{
    DropStayedItem(p);
    return 1;
}

extern char fmt_not_support_yet[]; /* not support yet %d */

s32 spare_item_slot_(enum spare_item_slot_operation operation, Humanoid *human)
{
    switch (operation)
    {
    case SPARE_ITEM_SLOT_CLEAR:
    {
        Humanoid *p = human;
        if (p == 0)
            p = CamState.Owner;
        p->item[ITEM_N] = 0;
        break;
    }
    case SPARE_ITEM_SLOT_QUERY:
        if (human == 0)
            human = CamState.Owner;
        return human->item[ITEM_N] == 1;
    default:
        AdtMessageBox(fmt_not_support_yet, operation);
        break;
    }
    return 0;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * enum TItemType GetItemType(int ConflictID);
 *     ITEM.C:601, 12 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int ConflictID
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

TItemType GetItemType(s32 ConflictID)
{
    s32 i;

    for (i = 0; i < MAX_ITEMS; i++)
    {
        if (items[i].locate->id == ConflictID)
        {
            return items[i].type;
        }
    }
    return ITEM_KAGINAWA;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct tag_TItem * GetFreeItemSlot(void);
 *     ITEM.C:577, 20 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 * END PSX.SYM */

TItem *GetFreeItemSlot(void)
{
    TItem *item;
    s32 i;

    i = 0;
    do
    {
        ic++;
        if (ic >= MAX_ITEMS)
            ic = 0;
        item = items + ic;
        if (item->proc == 0)
            return item;
        i++;
    } while (i < MAX_ITEMS - 1);

    item->mode = ITEM_MODE_DISPOSE;
    item->proc(item);
    DeleteConflict(item->locate);
    if (item->mode != ITEM_MODE_START)
    {
        AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
    }
    item->owner = 0;
    item->proc = 0;
    return item;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ClearItemLayout(void);
 *     ITEM.C:461, 8 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

void ClearItemLayout(void)
{
    TItem *it;
    s32 i;

    for (i = 0; ; i++)
    {
        if (i >= MAX_ITEMS)
            return;
        it = &items[i];
        if (it->proc != 0)
        {
            DISPOSE_ITEM(it);
        }
    }
}

u8 get_henshin_type_(short chr, short idx)
{
    int flag;

    flag = (chr == AYAME_0);
    return HensinT[idx][flag];
}
