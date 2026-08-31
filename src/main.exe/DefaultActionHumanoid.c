#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"
#include "sound.h"

/* The release-emptied debug print, the idiom the demo build shows live: a
 * debug build defines it as do { FntPrint x; } while (0) (the demo binary
 * carries this TU's prints compiled in; retail still links FntPrint). The
 * name is a stand-in -- the original macro name is unrecoverable. The empty
 * expansion's loop notes are load-bearing at three sites in the conflict
 * arm; see the register-allocation notes below. */
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

/*
 * Register-allocation constraints:
 *  - reflect_dz is a short, hot, call-crossing carrier that outranks and
 *    conflicts with i, so it takes $s0 and moves i to $s1. The earlier zz
 *    values then share $s0 because each dies at its reflect_dz copy.
 *  - Keep the three copies in separate control-flow blocks, with the turn-path
 *    copy before if (i > 0), and keep the mask and shift as two statements.
 *    Flow counts those references before the copies, mask, and negation fold
 *    away; moving or combining them loses the required conflict or weight.
 *    Reusing dead PSX.SYM local ry also matches, but reflect_dz states the value
 *    being preserved and has more allocation headroom.
 *  - There is no honest carrier in the conflict loop: GetDirection's result
 *    dies before SetNowMotion, while the vector save/restore crosses no call.
 *  - Retail's added damage arm makes i call-crossing, creating the contest that
 *    the demo build did not have. GCC 2.8.0-psx, 2.8.1-psx, and gs107 agree on
 *    this allocation; 2.7.2 misses even the instruction count.
 *
 * Scheduling constraints:
 *  - The three empty DBG one-shot loops in the conflict arm are release forms
 *    of debug-print sites. Their loop notes form sched1 region boundaries and
 *    preserve the target's conflict-record load order. Each site, and the
 *    size-before-position source order in the third region, is required.
 *  - Qualifiers are not substitutes: volatile on the short field changes the
 *    load sequence; a volatile word orders memory but not the address ALU;
 *    const and register are inert.
 *  - This explanation is consistent with the demo's live map-probe prints and
 *    its 31 FntPrint calls. Retail still links diagnostic printing, so deleted
 *    per-frame DBG calls are a natural source for the otherwise empty fences.
 *  - The recovered long i is explicitly narrowed at both map queries to keep
 *    the shared API's promoted int mode visible.
 */
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
    human->rotate->vy &= 0xfff;
    /* The cast (not &= 0xff) makes the reload an lbu: byte-required
     * (verified against the .s). */
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

    /* The demo build's own dump of the probe result, recovered verbatim
     * from the demo binary (strings at 0x800106d4/0x800106f4, arguments
     * matched field-for-field against its call sites); compiled out of
     * retail, where this whole statement folds to nothing. */
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

    if (map->attrib & 2)
    {
        human->attribute |= 0x200;
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
        SetNowMotion(human, MOT_DEAD, 1);
    }

    /* One lw covering vector/direct/angleL/angleH; the high half is the
     * two wall-angle bytes. */
    if (*(s32 *)&map->vector & (s32)0xffff0000)
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
            if (ry == -1)
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
                    i = (i > 0) ? i - 0x1000 : i + 0x1000;
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
        while ((i = GetConflictResult(object, -1)) >= 0)
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
            if ((ConflictObject[i].size.pad & CONFLICT_SOFT) == 0 &&
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

                /* Three DBG sites the bytes require (their empty release
                 * expansions are sched1 region fences -- see the header).
                 * Unlike the map-probe pair above, the real print text
                 * postdates the demo and is unrecoverable; the argument
                 * below is a placeholder, not a recovered string. */
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
                    if (conflict->size.pad & CONFLICT_STAND)
                    {
                        vector->vy = 0;
                        map->level = top;
                        locate->vy = top;
                        map->height = 0;
                        /* Standing on the object: wipe the ground-material bits. */
                        map->attrib &= 0xff80;
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
                    SetNowMotion(human, direction, 1);
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
