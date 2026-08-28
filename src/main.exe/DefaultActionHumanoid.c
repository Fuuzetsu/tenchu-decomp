#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DefaultActionHumanoid(struct Humanoid *human);
 *     HUMAN.C:155, 175 src lines, frame 120 bytes, saved-reg mask 0x80ff0000 (DEMO build -- see below)
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
 * The nested one-shot loops around the conflict pointer and object id place
 * sched1 loop-note fences between the target's pointer, id, and size loads.
 * The identical yy arms add a zero-code CFG fence without loop-weighting the
 * collision pointer and rotating its a1/a2 allocation.
 * The 14-deep one-shot tower around the object-vault turn is now
 * EXACTLY understood (regalloc.py --order on both shapes): each level
 * adds +1 weighted ref to the function-wide zz (18 -> 32), pushing its
 * global-alloc priority (floor_log2(refs)*refs/live_length) over the
 * floor_log2 cliff at 32 refs -- 10062 vs 9701 for the STAT_DEAD
 * constant's pseudo -- so zz wins $s0 and the whole callee-saved
 * assignment matches. 14 levels is the measured minimum for the +14;
 * 5/6-level abs-only towers stall at 43 differing instructions (the
 * s0/s1 pair swap) and the flat form cascades to ~114. Extra
 * source-level zz reads cannot substitute (cse folds them) and a
 * block-scoped local cannot take $s0 (it crosses no call). The
 * scaffold is a measured register-pressure dial, not a guess. Selector
 * rewrites also fail: zz-as-selector reaches the same 43-diff state but
 * adds a sll/sra narrowing pair before the call (multi-source defs stop
 * the constant folding that direction's enjoy), and ry-as-selector is
 * worse still (+20 bytes). The +14 must come from other regions'
 * factoring if it comes at all -- see PLAN's DAH endgame lead. Also
 * refuted: retargeting the reflection region's `direction` onto zz
 * (liveness-legal, +4 natural refs) perturbs two allocnos at once --
 * no tower depth 9-14 rebalances it (13 under by the s0/s1 class,
 * 14 over by a different pair).
 * ENDGAME CLOSED (2026-08-29): the cliff arithmetic caps every natural
 * refactor -- zz needs floor_log2(r)*r/live > 0.9701 (34 refs at <=167
 * live) and in-place reuse tops out around 28/169; the permuter's best
 * flat candidate repaired the allocation with a DEAD compute on the
 * starved pseudo, which fixes the registers but emits its own
 * instructions (score stalls at the dead code's size). A zero-code ref
 * amplifier is required, and in cc1 2.8.1 only note-based loop
 * weighting -- the do-while(0) family -- adds refs without emitting a
 * single byte. Whatever the 1998 source spelled (statement macros
 * expanding to one-shot wrappers being the period idiom), it reduced to
 * exactly this construct.
 * Retail narrows the recovered `long i` at both map-query calls; explicit
 * casts retain that local's original type and the shared API's original
 * promoted `int mode` without hiding either behind a false prototype.
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

    i = 1;
    map = &human->map;
    locate = human->locate;
    vector = &human->vector;
    object = *human->model->object;
    human->rotate->vy &= 0xfff;
    human->attribute = (u8)human->attribute;
    slocate = &human->slocate;

    if (map->vector == 0)
    {
        i = 0x11;
    }
    if (human->type != BALMA)
    {
        FieldArea = map->area;
        FieldIndex = map->index;
    }

    {
        VECTOR position;
        VECTOR *wide;
        MapVector *call_map;

        if (human->status == STAT_ATTACK)
        {
            goto use_conflict_position;
        }
        call_map = map;
        if (human->status == STAT_SQUAT)
        {
            goto use_locate_position;
        }
        wide = locate;
        if (map->height != 0)
        {
            goto probe_map;
        }
        {
            s32 abs_x;
            s32 abs_z;

            abs_x = vector->vx;
            if (abs_x < 0)
            {
                abs_x = -abs_x;
            }
            if (abs_x >= 0x51)
            {
                goto use_conflict_position;
            }
            abs_z = vector->vz;
            if (abs_z < 0)
            {
                abs_z = -abs_z;
            }
            if (abs_z < 0x51)
            {
                goto probe_map;
            }
        }

    use_conflict_position:
        position = ConflictObject[(*human->model->object)->id].position;
        position.vy = locate->vy;
        GetAreaMapVector(GlobalAreaMap, map, &position, human->width,
                         (short)i);
        goto map_probe_done;
    use_locate_position:
        wide = locate;
    probe_map:
        GetAreaMapVector(GlobalAreaMap, call_map, wide, human->width,
                         (short)i);
    map_probe_done:;
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
        if (vector->vy < 400)
        {
            vector->vy += 20;
        }
        if (map->attrib & MAP_DEATH)
        {
            if (map->height < 25000 && human->life != 0)
            {
                if (human == StagePlayer)
                {
                    Sound(human, 0x49);
                    SetCameraMode(CMODE_FALL);
                }
                else
                {
                    Sound(human, 8);
                }
                human->life = 0;
                if ((human->type & 0xf0) != PAGE_BOSS && human != StagePlayer)
                {
                    ReqLifeBar(human);
                }
            }
            goto ground_motion;
        }
    }
    else
    {
        if (vector->vy > 0 || map->level == (s32)0x80000000)
        {
            if ((map->attrib & MAP_DEATH) == 0)
            {
                human->attribute |= ATTR_NOFLOOR;
            }
            if (map->level != (s32)0x80000000)
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
    }

    /* One lw covering vector/direct/angleL/angleH; the high half is the
     * two wall-angle bytes. */
    if (*(s32 *)&map->vector & (s32)0xffff0000)
    {
        u16 attribute;

        attribute = human->attribute;
        human->attribute = attribute | ATTR_WALLANGLE;
        if (map->height < -450)
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
        if (zz == (s32)0x80000000)
        {
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
                GetAreaMapVector(GlobalAreaMap, &mv, &position, 300, 4);

                coefficient_x = RefrectMove[mv.vector][0];
                dx = position.vx - locate->vx;
                locate->vx += coefficient_x * ((dx >= 0) ? dx : -dx);

                coefficient_z = RefrectMove[mv.vector][1];
                dz = position.vz - locate->vz;
                locate->vz += coefficient_z * ((dz >= 0) ? dz : -dz);

                if (mv.level != zz && locate->vy < mv.level)
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
        }
        else
        {
            s32 angle_abs;

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
                    i = ((u16)human->width << 16) >> 18; /* width / 4; the u16 view is the retail lhu access width */
                    xx = RefrectMove[direction][0] * i;
                    zz = RefrectMove[direction][1] * i;
                }
                locate->vx += xx;
                locate->vz += zz;
            }
            else
            {
                xx = (rsin(ry) * human->width) >> 14;
                yy = rcos(ry);
                i = human->rotate->vy - ry;
                zz = (yy * human->width) >> 14;
                angle_abs = (i >= 0) ? i : -i;
                if (angle_abs >= 2000)
                {
                    i = (i > 0) ? i - 0x1000 : i + 0x1000;
                }
                angle_abs = (i >= 0) ? i : -i;
                if (angle_abs < 0x708 || human != StagePlayer || map->height != 0)
                {
                    if (map->angleH == 0 &&
                        (human->status == STAT_MOVE || human->status == STAT_CHASE))
                    {
                        {
                            SVECTOR *rotate;
                            s32 rotate_y;

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
                        }
                        MoveHumanoid(human, human->motion->motion->orderspd,
                                     human->motion->motion->sidespd);
                    }
                    xx >>= 1;
                    zz >>= 1;
                }
                locate->vx -= xx;
                locate->vz -= zz;
            }
        }
    }

    if (object->attribute & 0x8000)
    {
        while (1)
        {
            i = GetConflictResult(object, -1);
            if (i < 0)
            {
                break;
            }
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

                locate->vx = xx - ((ConflictDistance.vx >= 0)
                                       ? human->width
                                       : -human->width) /
                                      8;

                locate->vz -= ((ConflictDistance.vz >= 0)
                                   ? human->width
                                   : -human->width) /
                              8;

                do
                {
                    do
                    {
                        conflict = &ConflictObject[i];
                    } while (0);
                    object_id = object->id;
                } while (0);
                size_y = conflict->size.vy;
                if (object_id != 0)
                {
                    yy = conflict->position.vy;
                }
                else
                {
                    yy = conflict->position.vy;
                }
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
                                      (s16)human->locate->vy);
                    do
                    {
                        do
                        {
                            do
                            {
                                do
                                {
                                    do
                                    {
                                        do
                                        {
                                            do
                                            {
                                                do
                                                {
                                                    do
                                                    {
                                                        do
                                                        {
                                                            do
                                                            {
                                                                do
                                                                {
                                                                    do
                                                                    {
                                                                        do
                                                                        {
                                                                            direction_abs = zz >= 0 ? zz : -zz;
                                                                        } while (0);
                                                                    } while (0);
                                                                } while (0);
                                                            } while (0);
                                                        } while (0);
                                                    } while (0);
                                                } while (0);
                                            } while (0);
                                        } while (0);
                                    } while (0);
                                    direction = 0x1003;
                                } while (0);
                                if (direction_abs < 1100)
                                {
                                    direction = 0x1000;
                                }
                            } while (0);
                            SetNowMotion(human, direction, 1);
                        } while (0);
                        Sound(human, 6);
                    } while (0);
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
