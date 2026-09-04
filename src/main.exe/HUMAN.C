#include "common.h"
#include "main.exe.h"
#include "action.h"
#include "appear.h"
#include "effect.h"
#include "humanoid.h"
#include "item.h"
#include "model.h"
#include "sound.h"
#include "afterimage.h"
#include "tmdfast.h"
#include "vmemory.h"
#include <psxsdk/libgpu.h>

/*
 * Retail removes the demo's KillAllHumanoid and RestartTraceLine, and adds
 * draw_visible_characters_ and is_humanoid_on_stage_. The translation-unit
 * manifest retains both builds' orders.
 */

extern char msg_human_overflow[]; /* HUMAN OVERFLOW */
extern char msg_no_trace_point[]; /* NO TRACE POINT */
extern char fmt_dbg_pos[];        /* ~c800%02x~c888(%d,%d,%d)  */
extern char fmt_dbg_word[];       /* ~c880%04x=%02x  */
extern char fmt_dbg_pair[];       /* ~c080%02x/%d%d  */
extern char fmt_dbg_rot[];

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Humanoid * CreateHumanoid(short type, unsigned long *mad);
 *     HUMAN.C:36, 31 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       short type
 *     param $s0       unsigned long * mad
 *     reg   $s0       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct ModelType World;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct Humanoid *HumanGroup[32];
 * END PSX.SYM */

Humanoid *CreateHumanoid(character_kind type, unsigned long *mad)
{
    enum
    {
        BEAR_COLLISION_Y_OFFSET = -0x1C5,
        BEAR_COLLISION_Z_OFFSET = 0xC0
    };
    Humanoid *human;
    s16 conflict_id;
    u16 hh;
    u16 hh2;
    u16 ww;
    s32 half;
    s32 nhalf;
    s16 oldHumans;

    if (mad == 0 || Humans >= MAX_HUMANS)
    {
        SystemOut(msg_human_overflow);
    }
    human = (Humanoid *)vcalloc(sizeof(Humanoid), 0);
    human->type = type;
    human->status = STAT_NORMAL;
    human->attribute = 0;
    human->model = LoadModelArchive(mad, &World);
    human->locate = MODEL_POSITION(human->model);
    human->rotate = &human->model->rotate;
    human->model->attribute = MODEL_ATTR_CULL_BEHIND | MODEL_ATTR_CULL_SCREEN |
                              MODEL_ATTR_CULL_FAR;
    SetupThinkFunction(human, THINK_MIX_NONE);
    SetupCharacterParameter(type, human);
    hh = human->height;
    human->model->clip.vy = -((s16)hh / 2);
    UpdateMotion(human->motion, 0);
    GetAreaMapVector(GlobalAreaMap, &human->map, human->locate, human->width,
                     AREA_LEVEL_STEP_DOWN);
    SetupWeapon(human);
    conflict_id = InsertConflict(human->model->object[MODEL_PART_WAIST]);
    hh2 = human->height;
    ConflictObject[conflict_id].size.vy = half = (s16)hh2 / 2;
    nhalf = -half;
    ConflictObject[conflict_id].offset.vy =
        nhalf - human->model->rotate.pad;
    ww = human->width;
    ConflictObject[conflict_id].common = human;
    ConflictObject[conflict_id].size.vx =
        ConflictObject[conflict_id].size.vz =
        (s16)ww / 2;
    if (type == KUMA_0 || type == KUMA_1)
    {
        ConflictObject[conflict_id].offset.vy = BEAR_COLLISION_Y_OFFSET;
        ConflictObject[conflict_id].offset.vz = BEAR_COLLISION_Z_OFFSET;
    }
    oldHumans = Humans;
    Humans++;
    HumanGroup[oldHumans] = human;
    return human;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ControlHumanoid(struct Humanoid *human);
 *     HUMAN.C:107, 44 src lines, frame 72 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct Humanoid * human
 *     reg   $s1       struct ModelArchiveType * model
 *     reg   $s2       long m
 *     stack sp+24     struct MATRIX mat
 *
 * Globals it touches, as the original declared them:
 *     extern enum TSystemFlag SystemFlag;
 *     extern short SkipFrame;
 *     extern struct Humanoid *StagePlayer;
 *     extern short ActionHalt;
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

void ControlHumanoid(Humanoid *human)
{
    ModelArchiveType *model;
    s32 m;
    MATRIX mat;
    ModelType *head;
    s32 direction;
    s32 magnitude;
    s32 rotation_pair;

    model = human->model;
    m = 1;
    if (model->object[MODEL_PART_WAIST]->id >= 0)
    {
        DefaultActionHumanoid(human);
        StateTransition(human);
        DrawShadow(human);
    }
    else if (human->status == STAT_DEAD)
    {
        register_character_death(human);
        spread_blood_pool_(human);
    }
    HumanActionControl(human);
    if ((SystemFlag & SYSFLAG_DEBUGMODE) != 0 && SkipFrame == 0 &&
        human == StagePlayer)
    {
        FntPrint(fmt_dbg_pos, human->type,
                 human->locate->vx / 1000,
                 human->locate->vy / 1000,
                 human->locate->vz / 1000);
        FntPrint(fmt_dbg_word, (u16)human->attribute,
                 (u8)human->status);
        FntPrint(fmt_dbg_pair, (u8)human->motion->mid,
                 human->motion->loop, human->motion->count);
        FntPrint(fmt_dbg_rot, human->rotate->vy,
                 human->model->object[MODEL_PART_WAIST]->id);
    }

    if (SkipFrame != 0)
    {
        m = 0;
    }
    else
    {
        if (human != StagePlayer)
        {
            s32 clip;

            GsGetLs(&model->locate, &mat);
            GsSetLsMatrix(&mat);
            clip = DrawClip((ModelType *)model, 0);
            m = 0;
            if (clip >= 0)
            {
                m = -1;
            }
        }
    }

    PlayMotion(human->motion, human->status == STAT_ATTACK ? -1 : m);
    human->slocate = *human->locate;
    human->locate->vx += human->vector.vx;
    human->locate->vz += human->vector.vz;
    human->locate->vy += human->vector.vy;
    UpdateCoordinate((ModelType *)model);

    if (m == 0)
    {
        return;
    }

    /* Retail reads only the low halfword here. */
    DrawModeSave[VISIBLE_ENEMIES_] = DrawTMDmode;
    VISIBLE_CHARACTERS_ON_STAGE_[VISIBLE_ENEMIES_] = human;
    VISIBLE_ENEMIES_++;
    if (ActionHalt != ACTION_HALT_NONE || human->life <= 0)
    {
        return;
    }

    if (human == StagePlayer)
    {
        if (human->status == STAT_STICKON)
        {
            return;
        }
        head = human->model->object[MODEL_PART_HEAD];
        if (CamState.Mode != CMODE_DIRECTION && CamState.Mode != CMODE_SIGHT)
        {
            MotionElementType *rotation;

            if (human->motion->loop != MOTION_LOOP_DISABLED)
            {
                return;
            }
            rotation =
                human->motion->motion->rotate[MODEL_PART_HEAD];
            if (head->rotate.vx == rotation->x &&
                head->rotate.vy == rotation->y)
            {
                return;
            }
            head->rotate.vx = rotation->x;
            head->rotate.vy =
                human->motion->motion->rotate[MODEL_PART_HEAD]->y;
            UpdateCoordinate(head);
            return;
        }
        else
        {
            rotation_pair = human->model->object[MODEL_PART_WAIST]->rotate.vy +
                            human->model->object[MODEL_PART_TORSO]->rotate.vy;
            {
                s32 magnitude;

                direction = CamState.DirectionRY - rotation_pair;
                magnitude = direction >= 0 ? direction : -direction;
                if (magnitude > 900)
                {
                    head->rotate.vy = magnitude * 900 / direction;
                }
                else
                {
                    head->rotate.vy = direction;
                }
            }
            {
                s32 magnitude;

                direction = CamState.DirectionRX;
                magnitude = direction >= 0 ? direction : -direction;
                if (magnitude > 500)
                {
                    head->rotate.vx = magnitude * 500 / direction;
                }
                else
                {
                    head->rotate.vx = direction;
                }
            }
        }
        UpdateCoordinate(head);
        return;
    }
    if ((human->attribute & ATTR_PHASE) != PHASE_ALERT &&
        (human->target == &StagePlayer->model->locate ||
         human->motion->mid == MOT_ACTION))
    {
        return;
    }

    rotation_pair = human->model->object[MODEL_PART_WAIST]->rotate.vy +
                    human->model->object[MODEL_PART_TORSO]->rotate.vy +
                    human->rotate->vy;
    direction = GetDirection(
        human->target->coord.t[0] - human->locate->vx,
        human->target->coord.t[2] - human->locate->vz,
        (s16)rotation_pair);
    magnitude = direction >= 0 ? direction : -direction;
    if (magnitude >= 1800)
    {
        return;
    }

    head = human->model->object[MODEL_PART_HEAD];
    if (magnitude > 900)
    {
        head->rotate.vy = magnitude * 900 / direction;
    }
    else
    {
        head->rotate.vy = direction;
    }

    direction = (human->target->coord.t[1] - human->locate->vy) / 2;
    if (direction != 0)
    {
        if (direction >= -500)
        {
            if (direction <= 100)
            {
                head->rotate.vx = direction;
            }
            else
            {
                head->rotate.vx = 100;
            }
        }
        else
        {
            head->rotate.vx = -500;
        }
    }
    UpdateCoordinate(head);
}

/* Debug builds keep the original loop-shaped logging macro. */
#ifdef DEBUG
#define DBG(x) do { FntPrint x; } while (0)
#else
#define DBG(x) do { } while (0)
#endif

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DefaultActionHumanoid(struct Humanoid *human);
 *     HUMAN.C:155, 175 src lines, frame 120 bytes, saved-reg mask 0x80ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct Humanoid * human
 *     reg   $s3       struct MapVector * map
 *     reg   $s5       struct SVECTOR * vector
 *     reg   $s2       struct VECTOR * locate
 *     reg   $s4       struct VECTOR * slocate
 *     reg   $s7       struct ModelType * object
 *     reg   $a2       long i
 *     reg   $s4       long xx
 *     reg   $v1       long yy
 *     reg   $s0       long zz
 *     reg   $s0       long ry
 *     stack sp+32     struct VECTOR position
 *     stack sp+48     struct MapVector mv
 *     stack sp+64     struct VECTOR position
 *
 * Globals it touches, as the original declared them:
 *     extern struct AreaNodeType *FieldArea;
 *     extern struct NodeIndexType *FieldIndex;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct Humanoid *StagePlayer;
 *     extern short RefrectMove[16][2];
 *     extern struct SVECTOR ConflictDistance;
 * END PSX.SYM */

short DefaultActionHumanoid(Humanoid *human)
{
    enum
    {
        FAST_MOVEMENT_PROBE_SPEED = 80,
        WALL_RECOVERY_PROBE_Y_OFFSET = 500,
        WALL_RECOVERY_PROBE_RADIUS = 300,
        WALL_RECOVERY_STEP = 20,
        WALL_DIRECTION_WRAP_THRESHOLD = 2000,
        WALL_DIRECTION_LIMIT = 1800,
        WALL_TURN_STEP = 32,
        CONFLICT_REACTION_HALF_ANGLE = 1100,
        CONFLICT_VERTICAL_ESCAPE_SPEED = 10,
        TRACE_COLLISION_PAUSE = -20
    };
    MapVector *map;
    SVECTOR *vector;
    VECTOR *locate;
    VECTOR *slocate;
    ModelType *object;
    long i;
    long xx;
    long yy;
    long zz;
    long ry;
    long direction;

    i = AREA_LEVEL_STEP_DOWN;
    map = &human->map;
    locate = human->locate;
    vector = &human->vector;
    object = *human->model->object;
    human->rotate->vy &= ANGLE_MASK;
    /* Narrow through u8 before reloading the attribute. */
    human->attribute = (u8)human->attribute;
    slocate = &human->slocate;

    if (map->vector == 0)
    {
        i = AREA_LEVEL_STEP_DOWN | AREA_LEVEL_REUSE_CACHED;
    }
    if (human->type != BALMA)
    {
        FieldArea = map->area;
        FieldIndex = map->index;
    }

    if (human->status == STAT_ATTACK ||
        (human->status != STAT_SQUAT && map->height == 0 &&
         (__builtin_abs(vector->vx) > FAST_MOVEMENT_PROBE_SPEED ||
          __builtin_abs(vector->vz) > FAST_MOVEMENT_PROBE_SPEED)))
    {
        VECTOR position;

        position = ConflictObject[(*human->model->object)->id].position;
        position.vy = locate->vy;
        GetAreaMapVector(GlobalAreaMap, map, &position, human->width,
                         (short)i);
    }
    else
    {
        GetAreaMapVector(GlobalAreaMap, map, locate, human->width, (short)i);
    }

    /* Demo log text and fields are recovered; retail compiles this away. */
    if (map->level == LEVEL_NONE)
    {
        DBG(("l(ia) h%d v%x ah%x al%x %04x\n", map->height, map->vector,
             map->angleH, map->angleL, map->attrib));
    }
    else
    {
        DBG(("l%d h%d v%x ah%x al%x %04x\n", map->level, map->height,
             map->vector, map->angleH, map->angleL, map->attrib));
    }

    if (map->attrib & MAP_BUOYANT)
    {
        human->attribute |= ATTR_BUOYANT;
        if (vector->vy < 0)
        {
            vector->vy = 0;
        }
        map->height = 1;
    }

    if (map->height > 0 && (human->attribute & ATTR_FLOAT) == 0)
    {
        human->attribute |= ATTR_FALL;
        if (vector->vy < FALL_SPEED_MAX)
        {
            vector->vy += GRAVITY_ACCEL;
        }
        if (map->attrib & MAP_DEATH)
        {
            if (map->height < DEATH_FALL_HEIGHT && human->life != 0)
            {
                if (human == StagePlayer)
                {
                    Sound(human, SE_FATAL_FALL);
                    SetCameraMode(CMODE_FALL);
                }
                else
                {
                    Sound(human, CHAR_VOICE_HURT_HEAVY);
                }
                human->life = 0;
                if ((human->type & PAGE_MASK) != PAGE_BOSS && human != StagePlayer)
                {
                    ReqLifeBar(human);
                }
            }
        }
    }
    else if (vector->vy > 0 || map->level == LEVEL_NONE)
    {
        if ((map->attrib & MAP_DEATH) == 0)
        {
            human->attribute |= ATTR_NOFLOOR;
        }
        if (map->level != LEVEL_NONE)
        {
            locate->vy = map->level;
        }
        vector->vy = 0;
    }
    if ((map->attrib & MAP_DEATH) && map->height == 0 && human->status != STAT_DEAD)
    {
        SetNowMotion(human, MOT_DEAD, MOTION_MOVE_APPLY);
    }

    if (map->angleL != 0 || map->angleH != 0)
    {
        u16 attribute;

        attribute = human->attribute;
        human->attribute = attribute | ATTR_WALLANGLE;
        if (map->height < STEP_DROP_LIMIT)
        {
            human->attribute = attribute | ATTR_WALLANGLE | ATTR_LEDGE;
            locate->vy = map->level;
        }
        else if (map->height < 0)
        {
            locate->vy = map->level;
        }
    }

    if (map->vector != 0)
    {
        human->attribute |= ATTR_WALL;
        zz = map->level;
        if (zz == LEVEL_NONE)
        {
            MapVector mv;
            VECTOR position;
            s32 dx;
            s32 dz;
            s32 coefficient_x;
            s32 coefficient_z;

            position = ConflictObject[(*human->model->object)->id].position;
            locate->vx = slocate->vx;
            locate->vz = slocate->vz;
            locate->vy = slocate->vy;
            position.vy = locate->vy - WALL_RECOVERY_PROBE_Y_OFFSET;
            GetAreaMapVector(GlobalAreaMap, &mv, &position,
                             WALL_RECOVERY_PROBE_RADIUS,
                             AREA_LEVEL_ALLOW_DEEP);

            coefficient_x = RefrectMove[mv.vector][0];
            dx = position.vx - locate->vx;
            locate->vx += coefficient_x * ((dx >= 0) ? dx : -dx);

            coefficient_z = RefrectMove[mv.vector][1];
            dz = position.vz - locate->vz;
            locate->vz += coefficient_z * ((dz >= 0) ? dz : -dz);

            if (mv.level != LEVEL_NONE && locate->vy < mv.level)
            {
                locate->vy = (vector->vy > 0)
                                 ? locate->vy + vector->vy
                                 : locate->vy + WALL_RECOVERY_STEP;
            }
            else
            {
                vector->vz = 0;
                vector->vx = 0;
            }
        }
        else
        {
            s32 angle_abs;
            long reflect_dz;

            direction = map->vector;
            ry = RefrectVector[direction];
            if (ry == ANGLE_NONE)
            {
                if (vector->vx != 0 || vector->vz != 0)
                {
                    xx = -vector->vx;
                    zz = -vector->vz;
                }
                else
                {
                    i = (short)(human->width >> 2);
                    xx = RefrectMove[direction][0] * i;
                    zz = RefrectMove[direction][1] * i;
                }
                locate->vx += xx;
                locate->vz += zz;
            }
            else
            {
                xx = (rsin(ry) * human->width) >> 14;
                zz = (rcos(ry) * human->width) >> 14;
                i = human->rotate->vy - ry;
                angle_abs = __builtin_abs(i);
                if (angle_abs >= WALL_DIRECTION_WRAP_THRESHOLD)
                {
                    i = (i > 0) ? i - ANGLE_FULL : i + ANGLE_FULL;
                }
                angle_abs = __builtin_abs(i);
                if (angle_abs < WALL_DIRECTION_LIMIT || human != StagePlayer || map->height != 0)
                {
                    if (map->angleH == 0 &&
                        (human->status == STAT_MOVE || human->status == STAT_CHASE))
                    {
                        SVECTOR *rotate;
                        s32 rotate_y;

                        /* Preserve the displacement selected before the
                         * movement code runs. */
                        reflect_dz = zz;
                        rotate = human->rotate;
                        rotate_y = rotate->vy;
                        if (i > 0)
                        {
                            rotate_y -= WALL_TURN_STEP;
                        }
                        else
                        {
                            rotate_y += WALL_TURN_STEP;
                        }
                        rotate->vy = rotate_y;
                        MoveHumanoid(human, human->motion->motion->orderspd,
                                     human->motion->motion->sidespd);
                    }
                    else
                    {
                        reflect_dz = zz;
                    }
                    xx >>= 1;
                    /* Make the fixed-point displacement even, then take its
                     * exact half. */
                    reflect_dz &= ~1;
                    reflect_dz >>= 1;
                }
                else
                {
                    reflect_dz = zz;
                }
                locate->vx -= xx;
                /* Turn the outward displacement into the correction to
                 * apply. */
                reflect_dz = -reflect_dz;
                locate->vz += reflect_dz;
            }
        }
    }

    if (object->attribute & MODEL_ATTR_CONFLICT)
    {
        while ((i = GetConflictResult(object, CONFLICT_NONE)) >= 0)
        {
            if (ConflictObject[i].common == human)
            {
                continue;
            }
            if (ConflictObject[i].size.pad & CONFLICT_HIT)
            {
                if (human->status != STAT_DEAD)
                {
                    u16 attribute;

                    attribute = human->attribute;
                    human->vector.pad = i;
                    human->attribute = attribute | ATTR_HIT;
                }
                continue;
            }
            if ((ConflictObject[i].size.pad &
                 CONFLICT_SOFT) == 0 &&
                (human->attribute & ATTR_FLOAT) == 0)
            {
                s32 top;
                s32 object_y;
                s32 size_y;
                s32 object_id;
                ConflictObjectType *conflict;

                human->attribute |= ATTR_PUSH;
                xx = locate->vx;
                zz = locate->vz;

                locate->vx -= ((ConflictDistance.vx >= 0)
                                   ? human->width : -human->width) / 8;
                locate->vz -= ((ConflictDistance.vz >= 0)
                                   ? human->width : -human->width) / 8;

                /* Retail retains three empty debug sites; their original text is unknown. */
                conflict = &ConflictObject[i];
                DBG(("deleted debug print (text lost)\n"));
                object_id = object->id;
                DBG(("deleted debug print (text lost)\n"));
                size_y = conflict->size.vy;
                yy = conflict->position.vy;
                DBG(("deleted debug print (text lost)\n"));
                object_y = ConflictObject[object_id].position.vy;
                top = yy - size_y;
                if (object_y < top)
                {
                    locate->vx = xx;
                    locate->vz = zz;
                    if (conflict->size.pad &
                        CONFLICT_STAND)
                    {
                        vector->vy = 0;
                        map->level = top;
                        locate->vy = top;
                        map->height = 0;
                        /* Standing on the object: wipe the ground-material bits. */
                        map->attrib &= ~MAP_MATERIAL_MASK;
                        human->attribute &= ~(ATTR_PUSH | ATTR_FALL);
                    }
                    else
                    {
                        vector->vy = CONFLICT_VERTICAL_ESCAPE_SPEED;
                    }
                    continue;
                }
                if (yy + size_y < object_y &&
                    (human->status == STAT_NORMAL || human->status == STAT_MOVE ||
                     human->status == STAT_ENGAGE || human->status == STAT_CHASE))
                {
                    s32 direction_abs;

                    zz = GetDirection(ConflictObject[i].position.vx - locate->vx,
                                      ConflictObject[i].position.vz - locate->vz,
                                      /* locate->vy — the world Y, not a
                                       * rotation: retail's own bug (every
                                       * other caller passes rotate->vy). */
                                      (s16)human->locate->vy);
                    direction_abs = zz >= 0 ? zz : -zz;
                    direction = MOT_DAMAGE_BACK_LIGHT;
                    if (direction_abs < CONFLICT_REACTION_HALF_ANGLE)
                    {
                        direction = MOT_DAMAGE;
                    }
                    SetNowMotion(human, direction, MOTION_MOVE_APPLY);
                    Sound(human, CHAR_VOICE_HURT);
                }

                {
                    SVECTOR *rotate;
                    s32 rotate_y;

                    rotate = human->rotate;
                    yy = rotate->vy;
                    if (i & 1)
                    {
                        rotate_y = yy + human->turn;
                    }
                    else
                    {
                        rotate_y = yy - human->turn;
                    }
                    rotate->vy = rotate_y;
                }
                if (human->vector.vx != 0 || human->vector.vz != 0)
                {
                    MoveHumanoid(human, human->motion->motion->orderspd,
                                 human->motion->motion->sidespd);
                    if (human->trace != 0 && (human->attribute & ATTR_TRACE))
                    {
                        human->trace->count = TRACE_COLLISION_PAUSE;
                    }
                }
            }
        }
    }
    return human->attribute;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SearchTarget(struct Humanoid *human, long *distance, short *degree);
 *     HUMAN.C:436, 46 src lines, frame 64 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct Humanoid * human
 *     param $s3       long * distance
 *     param $s4       short * degree
 *     reg   $s2       struct VECTOR * head
 *     stack sp+16     struct VECTOR vect
 *     stack sp+32     struct SVECTOR svect
 *     reg   $s0       short mode
 *     reg   $a0       long dx
 *     reg   $a1       long dz
 *     reg   $s0       short roty
 *     reg   $a1       short degree
 *     reg   $a3       short n
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct Humanoid *StagePlayer;
 *     extern long EmergencyNotice;
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

typedef struct
{
    s32 sight_distance; /* beyond this: SR_UNSEEN */
    s32 clear_distance; /* within this: SR_SEEN, else SR_GLIMPSE */
    s32 far_distance;   /* beyond this: SR_GONE (target lost) */
} SearchSight;

enum sight_profile
{
    SIGHT_PROFILE_STANDING,
    SIGHT_PROFILE_SNEAKING,
    N_SIGHT_PROFILES
};

enum
{
    SIGHT_CHECK_INTERVAL = 32,
    SIGHT_VERTICAL_LIMIT = 3000,
    SIGHT_VERTICAL_NEAR_DISTANCE = 4000,
    STANDING_SIGHT_HALF_ANGLE = 900,
    SNEAKING_SIGHT_HALF_ANGLE = 450,
    SIGHT_RAY_ORIGIN_HEIGHT = 300,
    STANDING_SIGHT_RAY_LIMIT = 500,
    SNEAKING_SIGHT_RAY_LIMIT = 300
};

static SearchSight searchsight[N_SIGHT_PROFILES] = {
    {
        .sight_distance = 16000,
        .clear_distance = 10000,
        .far_distance = 20000
    },
    {
        .sight_distance = 12000,
        .clear_distance = 7000,
        .far_distance = 16000
    }
};

search_result SearchTarget(Humanoid *human, long *distance, short *degree)
{
    VECTOR vect;
    VECTOR position;
    SVECTOR svect;
    s32 raw_degree;
    s32 roty;
    s16 profile;
    s32 limit;
    s32 absolute;
    s32 base_y;
    s32 initial_delta_y;
    s32 delta_y;
    s32 full_height;
    s32 half_height;
    s32 adjusted_y;
    u16 player_height;
    s16 signed_degree;
    s16 result_degree;
    s16 n;

    position = *human->locate;
    n = 1;
    if (human->target == 0)
    {
        return SR_NONE;
    }

    vect.vx = human->target->coord.t[0] - position.vx;
    vect.vy = human->target->coord.t[1] - position.vy;
    vect.vz = human->target->coord.t[2] - position.vz;
    *distance = SquareRoot0(vect.vx * vect.vx + vect.vy * vect.vy +
                            vect.vz * vect.vz);

    roty = (u16)human->rotate->vy;
    raw_degree = ratan2(-vect.vx, -vect.vz) - roty;
    signed_degree = raw_degree;
    result_degree = raw_degree;
    if (signed_degree > ANGLE_HALF)
    {
        result_degree = ANGLE_FULL - raw_degree;
    }
    else if (signed_degree <= -ANGLE_HALF)
    {
        result_degree = raw_degree + ANGLE_FULL;
    }
    *degree = result_degree;

    if (((GameClock + human->model->object[MODEL_PART_WAIST]->id) &
         (SIGHT_CHECK_INTERVAL - 1)) != 0)
    {
        return SR_NONE;
    }

    /* Sneaking (STAT_SQUAT or STAT_STICKON) selects the short-range
     * sight row. */
    profile = (u16)(StagePlayer->status - STAT_SQUAT) < 2
                  ? SIGHT_PROFILE_SNEAKING
                  : SIGHT_PROFILE_STANDING;
    if (StagePlayer->status == STAT_HANG)
    {
        if (vect.vy >= 0 ||
            (vect.vy < -SIGHT_VERTICAL_LIMIT &&
             *distance < SIGHT_VERTICAL_NEAR_DISTANCE))
        {
            return SR_GONE;
        }
    }

    if (__builtin_abs(vect.vy) >= SIGHT_VERTICAL_LIMIT)
    {
        if (EmergencyNotice == 0 ||
            *distance < SIGHT_VERTICAL_NEAR_DISTANCE)
        {
            return SR_GONE;
        }
    }

    if (*distance >= searchsight[profile].far_distance)
    {
        return SR_GONE;
    }

    absolute = __builtin_abs(*degree);
    if (absolute < STANDING_SIGHT_HALF_ANGLE)
    {
        if (absolute >= SNEAKING_SIGHT_HALF_ANGLE &&
            profile != SIGHT_PROFILE_STANDING)
        {
            return SR_UNSEEN;
        }
        if (*distance >= searchsight[profile].sight_distance)
        {
            return SR_UNSEEN;
        }

        limit = STANDING_SIGHT_RAY_LIMIT;
        if (profile != SIGHT_PROFILE_STANDING)
        {
            limit = SNEAKING_SIGHT_RAY_LIMIT;
        }
        initial_delta_y = vect.vy;
        delta_y = initial_delta_y - SIGHT_RAY_ORIGIN_HEIGHT;
        position.vy += SIGHT_RAY_ORIGIN_HEIGHT - human->height;
        base_y = delta_y + human->height;
        vect.vy = base_y;
        player_height = StagePlayer->height;
        full_height = (s16)player_height;
        if (StagePlayer->status == STAT_SQUAT)
        {
            half_height = (s16)player_height / 2;
            adjusted_y = base_y - half_height;
        }
        else
        {
            adjusted_y = base_y - full_height;
        }
        vect.vy = adjusted_y;

        while (limit < __builtin_abs(vect.vx) ||
               limit < __builtin_abs(vect.vy) ||
               limit < __builtin_abs(vect.vz))
        {
            n <<= 1;
            vect.vx >>= 1;
            vect.vy >>= 1;
            vect.vz >>= 1;
        }

        svect.vx = vect.vx;
        svect.vy = vect.vy;
        svect.vz = vect.vz;
        if (GetAreaMapPassage(GlobalAreaMap, &position, &svect, n) != 0)
        {
            return SR_GONE;
        }
        return (*distance < searchsight[profile].clear_distance) ? SR_SEEN
                                                                 : SR_GLIMPSE;
    }
    return SR_UNSEEN;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ControlTraceLine(struct Humanoid *human);
 *     HUMAN.C:526, 33 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s5       struct Humanoid * human
 *     reg   $s4       struct TracePoint * point
 *     reg   $s1       struct TraceLine * trcl
 *     reg   $s2       long dx
 *     reg   $s0       long dz
 *     reg   $s6       long dist
 *     reg   $s3       short pad
 *     reg   $s2       long dx
 *     reg   $s0       long dz
 *     reg   $s0       short roty
 *     reg   $a1       short degree
 * END PSX.SYM */

short ControlTraceLine(Humanoid *human)
{
    TraceLine *trcl;
    TracePoint *point;

    s32 dx, dz;
    s32 dist;
    s16 cnt;
    trace_pad pad;
    u16 roty;
    s32 ang;
    short t;
    s16 diff;
    s16 degree;
    s32 absdeg;
    s32 d32;

    trcl = human->trace;
    pad = PADLup;
    if (trcl == 0)
    {
        return 0;
    }
    point = trcl->point + trcl->index;
    dx = point->x - human->locate->vx;
    dz = point->z - human->locate->vz;
    dist = SquareRoot0(dx * dx + dz * dz);
    if ((human->attribute & ATTR_WALL) != 0)
    {
        trcl->count = -30;
    }
    cnt = trcl->count;
    trcl->count = cnt + 1;
    if (cnt > 0)
    {
        roty = human->rotate->vy;
        ang = ratan2(-dx, -dz);
        t = ang - roty;
        diff = t;
        if (diff > ANGLE_HALF)
        {
            t = ANGLE_FULL - t;
        }
        else if (diff <= -ANGLE_HALF)
        {
            t += ANGLE_FULL;
        }
        d32 = t;
        degree = d32;
        if (human->turn <= d32)
        {
            pad |= PADLright;
        }
        else if (d32 <= -human->turn)
        {
            pad |= PADLleft;
        }
        absdeg = degree;
        if (absdeg < 0)
        {
            absdeg = -absdeg;
        }
        if (absdeg > 500)
        {
            pad &= (PADLleft | PADLright);
        }
    }
    if (dist <= point->range)
    {
        trcl->index++;
        if (trcl->point[trcl->index].pad == TRACE_POINT_END)
        {
            trcl->index = 0;
            return (s16)PAD_DIRECTION_BUTTONS;
        }
        pad |= trcl->point[trcl->index].pad;
    }
    return pad;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void KillHumanoid(struct Humanoid *human);
 *     HUMAN.C:78, 15 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 * END PSX.SYM */

void KillHumanoid(Humanoid *human)
{
    short i;

    if (human == 0)
    {
        return;
    }

    DeleteConflict(human->model->object[MODEL_PART_WAIST]);
    DisposeModelArchive(human->model);
    DisposeMotionManager(human->motion);
    DisposeWeapon(human);
    dispose_weapon_data_of_char_(human, ATTACK_CANCEL_ALL);
    vfree(human);
    for (i = 0; i < Humans; i++)
    {
        if (HumanGroup[i] == human)
        {
            break;
        }
    }
    if (i < Humans)
    {
        Humans--;
        HumanGroup[i] = HumanGroup[Humans];
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ControlAllHumanoid(void);
 *     HUMAN.C:97, 6 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 * END PSX.SYM */

short ControlAllHumanoid(void)
{
    Humanoid *human;
    s16 i;
    s32 result;

    VISIBLE_ENEMIES_ = 0;
    i = 0;
    result = Humans;
    if (result > 0)
        do
        {
            human = HumanGroup[i];
            if ((human->attribute & ATTR_SUSPEND) == 0)
            {
                if (human->type == BALMA)
                {
                    swap_balma_area_map_();
                    ControlHumanoid(human);
                    swap_balma_area_map_();
                }
                else
                {
                    ControlHumanoid(human);
                }
            }
            i++;
        } while (result = i < Humans);
    return result;
}

void draw_visible_characters_(void)
{
    s16 i;
    Humanoid *cs;

    for (i = 0; i < VISIBLE_ENEMIES_; i++)
    {
        cs = VISIBLE_CHARACTERS_ON_STAGE_[i];
        DrawTMDmode = DrawModeSave[i];
        DrawModelArchive(cs->model, -i);
        if (cs->weapon[WEAPON_SLOT_ACTIVE_0] != 0)
        {
            DrawOrnament(cs->weapon[WEAPON_SLOT_ACTIVE_0]);
        }
        if (cs->weapon[WEAPON_SLOT_ACTIVE_1] != 0)
        {
            DrawOrnament(cs->weapon[WEAPON_SLOT_ACTIVE_1]);
        }
        if (cs->illusion[WEAPON_HAND_0] != 0)
        {
            DrawAfterimage(cs->illusion[WEAPON_HAND_0], 1);
        }
        if (cs->illusion[WEAPON_HAND_1] != 0)
        {
            DrawAfterimage(cs->illusion[WEAPON_HAND_1], 1);
        }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Humanoid * GetHumanoid(short type);
 *     HUMAN.C:334, 9 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short type
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 * END PSX.SYM */

Humanoid *GetHumanoid(character_kind type)
{
    short i;

    for (i = 0; i < Humans; i++)
    {
        if (HumanGroup[i]->type == type)
        {
            return HumanGroup[i];
        }
    }
    return 0;
}

s32 is_humanoid_on_stage_(Humanoid *human)
{
    s32 i;

    for (i = 0; i < Humans; i++)
    {
        if (HumanGroup[i] == human)
            break;
    }
    return i != Humans;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void MoveHumanoid(struct Humanoid *human, short ordr, short side);
 *     HUMAN.C:349, 17 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *     param $a1       short ordr
 *     param $a2       short side
 * END PSX.SYM */

void MoveHumanoid(Humanoid *human, short ordr, short side)
{
    int sine, cosine;
    int order_value;
    short order_speed, side_speed;

    order_value = ordr;
    order_speed = ordr;
    side_speed = side;
    if (order_value != 0 || side != 0)
    {
        sine = -rsin(human->rotate->vy);
        cosine = -rcos(human->rotate->vy);
        /* Sign-extend only when the upper bits are clear; callers may pass wider values. */
        if ((order_value & MOTION_BYTE_UPPER_MASK) == MOTION_BYTE_SIGN_BIT)
        {
            order_speed = ordr - MOTION_BYTE_RANGE;
        }
        if ((side & MOTION_BYTE_UPPER_MASK) == MOTION_BYTE_SIGN_BIT)
        {
            side_speed = side - MOTION_BYTE_RANGE;
        }
        human->vector.vx = (short)(((short)sine * order_speed -
                                    (short)cosine * side_speed) >> FIXED_SHIFT);
        human->vector.vz = (short)(((short)cosine * order_speed +
                                    (short)sine * side_speed) >> FIXED_SHIFT);
    }
    else
    {
        human->vector.vz = 0;
        human->vector.vx = 0;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void GetMoveSpeed(struct SVECTOR *vect, short ry, short ordr, short side);
 *     HUMAN.C:370, 7 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct SVECTOR * vect
 *     param $a1       short ry
 *     param $a2       short ordr
 *     param $a3       short side
 * END PSX.SYM */

void GetMoveSpeed(SVECTOR *vect, short ry, short ordr, short side)
{
    int s, c;

    s = -rsin(ry);
    c = -rcos(ry);
    vect->vy = 0;
    vect->vx = (short)(((short)s * ordr - (short)c * side) >> FIXED_SHIFT);
    vect->vz = (short)(((short)c * ordr + (short)s * side) >> FIXED_SHIFT);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetDirection(long dx, long dz, short roty);
 *     HUMAN.C:381, 9 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       long dx
 *     param $a1       long dz
 *     param $a2       short roty
 * END PSX.SYM */

static __inline__ s16 FoldRelativeDirection(s32 difference)
{
    s16 signed_difference;
    s16 result;

    signed_difference = difference;
    result = difference;
    if (signed_difference > ANGLE_HALF)
    {
        result = ANGLE_FULL - difference;
    }
    else if (signed_difference <= -ANGLE_HALF)
    {
        result = difference + ANGLE_FULL;
    }
    return result;
}

s16 GetDirection(s32 dx, s32 dz, s32 roty)
{
    s32 difference;

    difference = ratan2(-dx, -dz) - roty;
    return FoldRelativeDirection(difference);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long GetTargetDistance(struct Humanoid *human, short *deg);
 *     HUMAN.C:394, 10 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *     param $a1       short * deg
 * END PSX.SYM */

long GetTargetDistance(Humanoid *human, short *deg)
{
    s32 dx, dz;
    s32 angle;
    s32 vy;
    s32 diff;
    s16 deg2;

    dx = human->target->coord.t[0] - human->locate->vx;
    dz = human->target->coord.t[2] - human->locate->vz;
    vy = (u16)human->rotate->vy;
    angle = ratan2(-dx, -dz);
    diff = angle - vy;
    deg2 = FoldRelativeDirection(diff);
    *deg = deg2;
    return SquareRoot0(dx * dx + dz * dz);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Humanoid * GetNearestHumanoid(struct Humanoid *human, short distance);
 *     HUMAN.C:408, 24 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *     param $a1       short distance
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 * END PSX.SYM */

Humanoid *GetNearestHumanoid(Humanoid *human, short distance)
{
    Humanoid *cur;
    Humanoid *best;
    s32 dx, dz;
    s32 dist;
    s32 best_dist;
    int i;

    best = 0;
    best_dist = 20000;
    if (human->map.height != 0)
    {
        return best;
    }
    for (i = 0; i < Humans; i++)
    {
        cur = HumanGroup[i];
        if (cur != human && cur->status != STAT_DEAD &&
            (cur->attribute & ATTR_SUSPEND) == 0)
        {
            dx = PAGE_CIVILIAN; /* parked in dx before its delta-x role */
            if ((cur->type & PAGE_MASK) != dx && cur->life >= 0)
            {
                dx = __builtin_abs(cur->locate->vx - human->locate->vx);
                dz = __builtin_abs(cur->locate->vz - human->locate->vz);
                dist = SquareRoot0(dx * dx + dz * dz);
                if (dist < distance && dist < best_dist)
                {
                    best_dist = dist;
                    best = cur;
                }
            }
        }
    }
    return best;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct TraceLine * SetupTraceLine(struct Humanoid *human, struct TracePoint *point);
 *     HUMAN.C:488, 18 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *     param $a1       struct TracePoint * point
 * END PSX.SYM */

TraceLine *SetupTraceLine(Humanoid *human, TracePoint *point)
{
    TraceLine *trcl;

    if (point == 0)
    {
        SystemOut(msg_no_trace_point);
    }
    trcl = (TraceLine *)valloc(8);
    trcl->count = 0;
    trcl->index = 0;
    trcl->point = point;
    while (point->pad != TRACE_POINT_END)
    {
        point++;
    }
    point->x = human->locate->vx;
    point->z = human->locate->vz;
    point->range = (s16)(human->locate->vy / 100);
    human->trace = trcl;
    return trcl;
}
