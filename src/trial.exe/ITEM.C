#include "common.h"
#include "tuning.h"
#include "../main.exe/main.exe.h"
#include "../main.exe/adt.h"
#include "../main.exe/action.h"
#include "../main.exe/item.h"
#include "../main.exe/images.h"
#include "../main.exe/model.h"
#include "../main.exe/appear.h"
#include "../main.exe/sound.h"
#include <psxsdk/libgs.h>
#include "../main.exe/effect.h"
#include "../main.exe/humanoid.h"
#include "../main.exe/afterimage.h"

/* TRIAL keeps the shared projectile, medicine, grapple, and dropped-item
 * handlers. Disabled item requests retain their original argument setup.
 * Fifteen unreferenced eight-byte entries have no recoverable identity;
 * UnusedTrialItemEntry names deliberately do not assert an original API. */

void ProcItemTeleport(TItem *item);

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

extern VECTOR ItemTargetRay;


static Humanoid *SearchItemTarget2(Humanoid *owner, SVECTOR *rot,
                                  VECTOR *start, VECTOR *target)
{
    enum item_targeting_policy
    {
        ITEM_TARGET_MIN_FORWARD_DISTANCE = 100,
        ITEM_TARGET_HORIZONTAL_LIMIT = 500,
        ITEM_TARGET_VERTICAL_LIMIT = 1000
    };
    int i;
    int nearest_distance;
    Humanoid *nearest_target;
    Humanoid *human;
    MATRIX mat;
    SVECTOR inverse_rotation;
    VECTOR world_position;
    VECTOR local_position;
    int in_target_cone;

    nearest_target = NULL;
    world_position = ItemTargetRay;
    RotateVector(&world_position, rot->vx, rot->vy, rot->vz);
    world_position.vx += start->vx;
    world_position.vy += start->vy;
    world_position.vz += start->vz;
    trace_ground_(start, &world_position, &local_position, 0);
    setVector(target, local_position.vx, local_position.vy,
              local_position.vz);
    nearest_distance = GetVectorDistance(&local_position, start);
    inverse_rotation.vx = -rot->vx;
    inverse_rotation.vy = -rot->vy;
    inverse_rotation.vz = -rot->vz;
    RotMatrix(&inverse_rotation, &mat);

    i = 0;
    while (1)
    {
        if (i >= Humans)
            break;
        human = HumanGroup[i];
        world_position = *GetAbsolutePosition(
            human->model->object[MODEL_PART_WAIST], 0, 0, 0);
        local_position = world_position;
        if (human->life > 0 && human != owner)
        {
            local_position.vx -= start->vx;
            local_position.vy -= start->vy;
            local_position.vz -= start->vz;
            ApplyMatrixLV(&mat, &local_position, &local_position);
            local_position.vz = -local_position.vz;
            if (local_position.vz > ITEM_TARGET_MIN_FORWARD_DISTANCE)
            {
                in_target_cone = 0;
                if (abs(local_position.vx) < ITEM_TARGET_HORIZONTAL_LIMIT)
                {
                    in_target_cone =
                        abs(local_position.vy) < ITEM_TARGET_VERTICAL_LIMIT;
                }
                if (in_target_cone &&
                    local_position.vz < nearest_distance)
                {
                    nearest_distance = local_position.vz;
                    *target = world_position;
                    nearest_target = human;
                }
            }
        }
        i++;
    }
    return nearest_target;
}


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
        return;
    }

    for (i = 0; i < MAX_ITEMS; i++)
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
    }
}


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
        GetAreaMapLevel(GlobalAreaMap, item->locate->locate.coord.t[0],
                        item->locate->locate.coord.t[1] - 500,
                        item->locate->locate.coord.t[2], AREA_LEVEL_DEFAULT);
        mv.attrib = FieldAttrib;
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
    scaled_horizontal_spread = len * (yw / 2);
    fly->count2 = fly->count;
    yw = scaled_horizontal_spread / FIXED_ONE;
    len = len * (yh / 2);
    if (len < 0)
    {
        len += FIXED_TRUNC_BIAS;
    }
    yh = len >> FIXED_SHIFT;
    mid_x = (fly->sx + fly->vx) / 2;
    horizontal_span = yw * 2;
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
    horizontal_span = yw * 2;
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

void ProcSightShot(TItem *item)
{
    enum
    {
        SIGHT_SHOT_DROP_HORIZONTAL_SPREAD = 200,
        SIGHT_SHOT_DROP_HORIZONTAL_RADIUS =
            SIGHT_SHOT_DROP_HORIZONTAL_SPREAD / 2,
        SIGHT_SHOT_DROP_VERTICAL_SPREAD = 100,
        SIGHT_SHOT_DROP_UPWARD_SPEED = 200
    };
    param_launch *launch_state = &item->param.launch;
    s32 dispose_mode;
    PARAM_ITEM_LAUNCH request;
    SVECTOR aim_rotation;
    int aim_pitch;
    int aim_yaw;
    Humanoid *human;

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
        u8 item_count = human->item[item->type];

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
        VECTOR *drop_position =
            GetAbsolutePosition(item->locate, 0, 0, 0);
        Humanoid *drop_owner = item->owner;
        TItemType item_type = item->type;

        request = (PARAM_ITEM_LAUNCH){0};
        request.type = item_type;
        request.user = drop_owner;
        copyVector(&request.start, drop_position);
        request.end.vx =
            rand() % SIGHT_SHOT_DROP_HORIZONTAL_SPREAD -
            SIGHT_SHOT_DROP_HORIZONTAL_RADIUS;
        request.end.vy =
            rand() % SIGHT_SHOT_DROP_VERTICAL_SPREAD -
            SIGHT_SHOT_DROP_UPWARD_SPEED;
        request.end.vz =
            rand() % SIGHT_SHOT_DROP_HORIZONTAL_SPREAD -
            SIGHT_SHOT_DROP_HORIZONTAL_RADIUS;
        ReqItemDrop(&request);

        if (item->proc != 0)
        {
            DISPOSE_ITEM_WITH_MODE(item, dispose_mode);
        }
        return;
    }

    {
        u8 count;
        ModelArchiveType *model;

        count = launch_state->count;
        if (count != 0)
        {
            launch_state->count--;
        }
        if ((item->owner->pad.data & PADRup) != 0)
        {
            if (launch_state->count != 0)
            {
                return;
            }
            SetCameraMode(CMODE_SIGHT);
            GsSortSprite(TargetSprite, OTablePt, 0);
            return;
        }

        count = launch_state->count;
        model = item->owner->model;
        if (count == 0)
        {
            GsRVIEW2 *view = &ViewInfo;

            request.type = item->type;
            request.user = item->owner;
            copyVector(&request.start, MODEL_POSITION(item->locate));
            GetVectorRotation(CAMERA_VIEWPOINT(view), CAMERA_REFERENCE(view),
                              &aim_pitch, &aim_yaw);
            aim_rotation.vz = 0;
            aim_rotation.vx = aim_pitch;
            aim_rotation.vy = aim_yaw;
            SearchItemTarget2(request.user, &aim_rotation,
                              CAMERA_VIEWPOINT(view), &request.end);
            if (item->proc != 0)
            {
                DISPOSE_ITEM_WITH_MODE(item, dispose_mode);
            }
            SetCameraMode(CMODE_LOCK);
        }
        else
        {
            request.type = item->type;
            request.user = item->owner;
            copyVector(&request.start, MODEL_POSITION(item->locate));
            SearchItemTarget2(request.user, &model->rotate, &request.start,
                              &request.end);
            if (item->proc != 0)
            {
                DISPOSE_ITEM_WITH_MODE(item, dispose_mode);
            }
        }
        ReqItemLaunch(&request);
    }
}

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

static void ProcItemManebue(TItem *item);


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
    Sprite3D *model = (Sprite3D *)item->model;
    s32 particle_index;

    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = KUSURI_MODE_START;
        return;
    }
    switch (item->mode)
    {
    case KUSURI_MODE_START:
    {
        Humanoid *human = item->owner;
        ModelArchiveType *archive;

        if (ActionHalt == ACTION_HALT_NONE && human->life > 0)
        {
            MotionDataType *motion_data;

            dispose_weapon_data_of_char_(human, ATTACK_CANCEL_ALL);
            UpdateMotion(human->motion, MOT_ITEM_DRINK);
            human->status = STAT_ITEM;
            motion_data = human->motion->motion;
            MoveHumanoid(human, motion_data->orderspd,
                         motion_data->sidespd);
        }

        archive = item->owner->model;
        if (archive->n > MODEL_PART_WEAPON_HAND_1)
            item->locate->locate.super =
                &archive->object[MODEL_PART_WEAPON_HAND_1]->locate;
        else
            item->locate->locate.super =
                &archive->object[MODEL_PART_HEAD]->locate;
        item->locate->locate.coord.t[0] = 0;
        item->locate->locate.coord.t[1] = KUSURI_MODEL_HAND_OFFSET;
        item->locate->locate.coord.t[2] = 0;
        item->mode++;
        return;
    }

    case KUSURI_MODE_DRINK:
    {
        MotionManager *motion = item->owner->motion;

        if (motion->mid != MOT_ITEM_DRINK)
        {
            VECTOR *drop_position =
                GetAbsolutePosition(item->locate, 0, 0, 0);
            Humanoid *drop_owner = item->owner;
            TItemType item_type = item->type;

            {
                PARAM_ITEM_LAUNCH drop_request = {
                    .type = item_type,
                    .user = drop_owner
                };

                copyVector(&drop_request.start, drop_position);
                drop_request.end.vx =
                    rand() % KUSURI_DROP_HORIZONTAL_SPREAD -
                    KUSURI_DROP_HORIZONTAL_SPREAD / 2;
                drop_request.end.vy =
                    rand() % KUSURI_DROP_VERTICAL_SPREAD +
                    KUSURI_DROP_VERTICAL_BASE;
                drop_request.end.vz =
                    rand() % KUSURI_DROP_HORIZONTAL_SPREAD -
                    KUSURI_DROP_HORIZONTAL_SPREAD / 2;
                ReqItemDrop(&drop_request);
            }
            if (item->proc == 0)
                return;
            DISPOSE_ITEM(item);
            return;
        }
        {
            s16 frame;

            frame = motion->count;
            if (frame == KUSURI_HEAL_FRAME)
            {
                item->mode = KUSURI_MODE_HEAL;
                return;
            }
            if (frame < KUSURI_VISIBLE_START_FRAME)
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
        particle_index = 0;
        item->owner->life = item->owner->lifemax;
        while (1)
        {
            if (particle_index >= N_KUSURI_HEAL_PARTICLES)
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
            particle_index++;
        }
        SoundEx(item->owner->locate, SE_MEDICINE);
        if (item->proc == 0)
            return;
        DISPOSE_ITEM(item);
    }
    }
}

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


void ProcItemGun(TItem *item)
{
    enum
    {
        GUN_MODE_FLASH = 0,
        GUN_MODE_FIRE = 1,
        GUN_MODE_FINISH = 2,
        GUN_MUZZLE_VECTOR_Z = -250,
        GUN_MUZZLE_IMPACT_SIZE = 2 * FIXED_ONE,
        GUN_MUZZLE_BLEED_GROUND_RANGE = 100,
        GUN_MUZZLE_BLEED_SPREAD = 10,
        GUN_MUZZLE_BLEED_COUNT = 10,
        GUN_MUZZLE_BLEED_TIME = 10,
        GUN_HIT_RADIUS = 100,
        GUN_IMPACT_VECTOR_Z = 150,
        GUN_FLESH_IMPACT_SIZE = 6 * FIXED_ONE,
        GUN_SOLID_IMPACT_SIZE = 4 * FIXED_ONE,
        GUN_HIT_BLEED_GROUND_RANGE = 100,
        GUN_HIT_BLEED_COUNT = 15,
        GUN_HIT_BLEED_TIME = 10
    };
    param_gun *param = &item->param.gun;
    SVECTOR aim;
    VECTOR hit_position;

    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = GUN_MODE_FLASH;
        return;
    }
    switch (item->mode)
    {
    case GUN_MODE_FLASH:
        aim = (SVECTOR){
            .vx = 0,
            .vy = 0,
            .vz = GUN_MUZZLE_VECTOR_Z
        };
        RotateVectorS(&aim, item->owner->model->rotate.vx,
                      item->owner->model->rotate.vy, 0);
        SetImpact(MODEL_POSITION(item->locate), GUN_MUZZLE_IMPACT_SIZE,
                  IMPACT_SPRITE_GUN);
        SetBleeds(MODEL_POSITION(item->locate),
                  GUN_MUZZLE_BLEED_GROUND_RANGE,
                  GUN_MUZZLE_BLEED_SPREAD,
                  GUN_MUZZLE_BLEED_COUNT,
                  GUN_MUZZLE_BLEED_TIME, COLOR_GRAY_DARK);
        item->mode++;
        return;

    case GUN_MODE_FIRE:
    {
        s32 rx;
        s32 ry;
        Humanoid *hit_human;
        s32 registered_collision;

        GetVectorRotation(MODEL_POSITION(item->locate), &param->vec, &rx, &ry);
        setVector(&aim, rx, ry, 0);
        hit_human = SearchItemTarget2(item->owner, &aim,
                                      MODEL_POSITION(item->locate),
                                      &hit_position);
        copyVector(MODEL_POSITION(item->locate), &hit_position);
        DeleteConflict(item->locate);
        registered_collision = InsertConflict(item->locate);
        SET_ITEM_COLLISION(registered_collision, GUN_HIT_RADIUS,
                           CONFLICT_OWNER_ITEM,
                           CONFLICT_HIT);
        {
            SVECTOR impact_direction = {
                .vx = 0,
                .vy = 0,
                .vz = GUN_IMPACT_VECTOR_Z
            };
            RotateVectorS(&impact_direction,
                          item->owner->model->rotate.vx,
                          item->owner->model->rotate.vy, 0);
            if (hit_human != 0)
            {
                SetImpact(&hit_position, GUN_FLESH_IMPACT_SIZE,
                          IMPACT_SPRITE_GUN);
                SetBleedsDir(&hit_position, &impact_direction,
                             GUN_HIT_BLEED_GROUND_RANGE,
                             GUN_HIT_BLEED_COUNT,
                             GUN_HIT_BLEED_TIME, COLOR_RED);
                SoundEx(&hit_position, SE_GUN_HIT_FLESH);
            }
            else
            {
                SetImpact(&hit_position, GUN_SOLID_IMPACT_SIZE,
                          IMPACT_SPRITE_GUN);
                SetBleedsDir(&hit_position, &impact_direction,
                             GUN_HIT_BLEED_GROUND_RANGE,
                             GUN_HIT_BLEED_COUNT,
                             GUN_HIT_BLEED_TIME, COLOR_YELLOW);
                SoundEx(&hit_position, SE_GUN_HIT_SOLID);
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
    s32 nearby;
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
            nearby =
                abs(ViewInfo.vpx - item->locate->locate.coord.t[0]) < LEN &&
                abs(view->vpy - item->locate->locate.coord.t[1]) < LEN &&
                abs(view->vpz - item->locate->locate.coord.t[2]) < LEN;
            if (nearby)
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

int ReqItemLaunch(PARAM_ITEM_LAUNCH *p)
{
    enum
    {
        SHURIKEN_FLIGHT_SPEED = 300,
        SHURIKEN_AFTERIMAGE_LENGTH = 10,
        SHURIKEN_AFTERIMAGE_HALF_WIDTH = 20,
        SHURIKEN_ARMING_DELAY = 5
    };
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
    SetupFly(&param->fly, pos, &p->end, FIXED_QUARTER, FIXED_QUARTER,
             SHURIKEN_FLIGHT_SPEED);
    item->param.launch.fly.mode = FLY_MODE_ARC;
    ai = SetupAfterimage(item->model, SHURIKEN_AFTERIMAGE_LENGTH);
    param->effect = ai;
    setVector(&ai->vector1, SHURIKEN_AFTERIMAGE_HALF_WIDTH, 0, 0);
    setVector(&ai->vector2, -SHURIKEN_AFTERIMAGE_HALF_WIDTH, 0, 0);
    param->count = SHURIKEN_ARMING_DELAY;
    return 1;
}

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

int ReqItemArrow(PARAM_ITEM_LAUNCH *p)
{
    enum
    {
        ARROW_VERTICAL_ARC_SPREAD = FIXED_HALF,
        ARROW_FLIGHT_SPEED = 300,
        ARROW_ARMING_DELAY = 5
    };
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
    SetupFly(&param->fly, pos, &target, 0, ARROW_VERTICAL_ARC_SPREAD,
             ARROW_FLIGHT_SPEED);
    param->count = ARROW_ARMING_DELAY;
    return 1;
}


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

/* Disabled requests still evaluate their position/rotation setup. The
 * original dispatcher discards these requests after updating inventory. */
static __inline__ void IgnoreDisabledItemRequest(void *request)
{
}

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
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        if (p->user == CamState.Owner)
        {
            param_launch *param;

            item = TakeItemSlot();
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
        REQUEST_ROTATED_ITEM(vec_z_n60, IgnoreDisabledItemRequest);
        break;
    }
    case ITEM_DOKUDANGO:
    {
        REQUEST_ROTATED_ITEM(vec_y_n120_z_n240, IgnoreDisabledItemRequest);
        break;
    }
    case ITEM_NEMURI:
    {
        REQUEST_ROTATED_ITEM(vec_z_n120, IgnoreDisabledItemRequest);
        break;
    }
    case ITEM_NINGYO:
    {
        REQUEST_ROTATED_ITEM(vec_y_n120_z_n120, IgnoreDisabledItemRequest);
        break;
    }
    case ITEM_GOSHIKIMAI:
    {
        REQUEST_ROTATED_ITEM(vec_z_n100, IgnoreDisabledItemRequest);
        break;
    }
    case ITEM_KAENGEKI:
    {
        param.vector = vec_z_n60[0];
        p->end.vx = param.vector.vx;
        p->end.vy = param.vector.vy;
        p->end.vz = param.vector.vz;
        IgnoreDisabledItemRequest(p);
        break;
    }
    case ITEM_NINKEN:
    {
        REQUEST_ROTATED_ITEM(vec_y_n120_z_n120, IgnoreDisabledItemRequest);
        break;
    }
    case ITEM_HAPPOU:
    {
        REQUEST_ROTATED_ITEM(vec_y_n120_z_n120, IgnoreDisabledItemRequest);
        break;
    }
    case ITEM_FIRE:
    {
        REQUEST_ROTATED_ITEM(vec_y_n120_z_n120, IgnoreDisabledItemRequest);
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
        IgnoreDisabledItemRequest(p);
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
        IgnoreDisabledItemRequest(&param.drop);
        break;
    }
    case ITEM_KAGINAWA:
    {
        TItem *item;
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        item = TakeItemSlot();
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
        REQUEST_ROTATED_ITEM(vec_z_n500, IgnoreDisabledItemRequest);
        break;
    }
    case ITEM_TELEPORT:
    {
        TItem *item;
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        item = TakeItemSlot();
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
        REQUEST_ROTATED_ITEM(vec_y_n120_z_n120, IgnoreDisabledItemRequest);
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
        param_napalm *pp;
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        item = TakeItemSlot();
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
        IgnoreDisabledItemRequest(p);
        break;
    case ITEM_KAWARIMI:
        IgnoreDisabledItemRequest(p);
        break;
    case ITEM_MANEBUE:
        IgnoreDisabledItemRequest(p);
    case ITEM_ARMOUR:
    default:
        break;
    }
    return 1;
}
#undef SETUP_ROTATED_DROP
#undef REQUEST_ROTATED_ITEM

void TurnAroundAllItems(Humanoid *user)
{
    enum
    {
        INVENTORY_DROP_HORIZONTAL_SPREAD = 200,
        INVENTORY_DROP_HORIZONTAL_RADIUS =
            INVENTORY_DROP_HORIZONTAL_SPREAD / 2,
        INVENTORY_DROP_VERTICAL_SPREAD = 100,
        INVENTORY_DROP_UPWARD_SPEED = 200
    };
    s32 item_index;
    s32 copy_index;

    item_index = 0;
    while (1)
    {
        if (item_index >= ITEM_N)
            break;
        copy_index = 0;
        while (1)
        {
            if (copy_index >= user->item[item_index])
                break;
            {
                VECTOR *drop_position = GetAbsolutePosition(
                    user->model->object[MODEL_PART_WAIST], 0, 0, 0);
                Humanoid *drop_owner = user;
                TItemType item_type = (TItemType)item_index;
                PARAM_ITEM_LAUNCH drop_request = {
                    .type = item_type,
                    .user = drop_owner
                };

                copyVector(&drop_request.start, drop_position);
                drop_request.end.vx =
                    rand() % INVENTORY_DROP_HORIZONTAL_SPREAD -
                    INVENTORY_DROP_HORIZONTAL_RADIUS;
                drop_request.end.vy =
                    rand() % INVENTORY_DROP_VERTICAL_SPREAD -
                    INVENTORY_DROP_UPWARD_SPEED;
                drop_request.end.vz =
                    rand() % INVENTORY_DROP_HORIZONTAL_SPREAD -
                    INVENTORY_DROP_HORIZONTAL_RADIUS;
                ReqItemDrop(&drop_request);
            }
            copy_index++;
        }
        user->item[item_index] = 0;
        item_index++;
    }
}

void UnusedTrialItemEntry01(void)
{
}

void UnusedTrialItemEntry02(void)
{
}

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


void UnusedTrialItemEntry03(void)
{
}

void UnusedTrialItemEntry04(void)
{
}

void UnusedTrialItemEntry05(void)
{
}

void UnusedTrialItemEntry06(void)
{
}

int ReqItemFire(PARAM_ITEM_LAUNCH *p)
{
}

void UnusedTrialItemEntry07(void)
{
}

void UnusedTrialItemEntry08(void)
{
}

void UnusedTrialItemEntry09(void)
{
}

int ReqItemDokudango(PARAM_ITEM_LAUNCH *p)
{
}

void UnusedTrialItemEntry10(void)
{
}

int ReqItemSmoke(PARAM_ITEM_LAUNCH *p)
{
}

void UnusedTrialItemEntry11(void)
{
}

void UnusedTrialItemEntry12(void)
{
}

int ReqItemStay(PARAM_ITEM_STAY *p)
{
    DropStayedItem(p);
    return 1;
}

extern char fmt_not_support_yet[]; /* not support yet %d */

void ProcItemTeleport(TItem *item)
{
}

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

void UnusedTrialItemEntry13(void)
{
}

TItem *GetFreeItemSlot(void)
{
    TItem *item;
    s32 i;

    for (i = 0; i < MAX_ITEMS - 1; i++)
    {
        ic++;
        if (ic >= MAX_ITEMS)
            ic = 0;
        item = items + ic;
        if (item->proc == 0)
            return item;
    }

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

void UnusedTrialItemEntry14(void)
{
}

void UnusedTrialItemEntry15(void)
{
}

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


