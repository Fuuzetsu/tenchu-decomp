#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"
#include "sound.h"

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
         ((vector->vx >= 0 ? vector->vx : -vector->vx) > 0x50 ||
          (vector->vz >= 0 ? vector->vz : -vector->vz) > 0x50)))
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
            goto ground_motion;
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
    ground_motion:
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
            position.vy = locate->vy - 500;
            GetAreaMapVector(GlobalAreaMap, &mv, &position, 300,
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
                                 : locate->vy + 20;
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
                angle_abs = (i >= 0) ? i : -i;
                if (angle_abs >= 2000)
                {
                    i = (i > 0) ? i - ANGLE_FULL : i + ANGLE_FULL;
                }
                angle_abs = (i >= 0) ? i : -i;
                if (angle_abs < 1800 || human != StagePlayer || map->height != 0)
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
                            rotate_y -= 0x20;
                        }
                        else
                        {
                            rotate_y += 0x20;
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
                        vector->vy = 10;
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
                    if (direction_abs < 1100)
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
                        human->trace->count = -20;
                    }
                }
            }
        }
    }
    return human->attribute;
}
