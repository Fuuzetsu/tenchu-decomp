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
 * The four small do{}while(0) fences on zz statements are ONE register-
 * pressure dial split across sites (regalloc.py --order on every shape):
 * cc1 weights a pseudo's refs by loop depth, +1 per enclosed ref per level,
 * linearly in depth, defs and uses alike (`zz >>= 1` encloses 2). zz needs
 * +14 weighted refs (18 -> 32) so its global-alloc priority
 * (floor_log2(refs)*refs/live_length) crosses the floor_log2 cliff at 32
 * refs -- 10062 vs 9701 -- and beats i's pseudo to $s0, which the whole
 * callee-saved assignment hangs on. The depths here (map->level @2, >>=1
 * @3, the conflict locate->vz store @3, the abs @3) sum to exactly +14 =
 * 2+6+3+3. Site choice is forced, all measured: fencing any statement
 * mentioning i raises the rival as fast as zz (GetDirection's args are +2 i
 * per level); any mentioning xx trips xx's floor_log2 cliff at 16 refs (one
 * boosted ref lifts it over locate/human and reshuffles s2-s4); and
 * `zz = locate->vz` (the conflict read-back) is fence-toxic for a third
 * reason -- the target schedules that load into the ConflictDistance.vx
 * load-delay shadow, and a fence barrier there pins it early and buys a
 * +4-byte nop. Refuted substitutes: the flat form cascades to ~114 diffs; a
 * single-site tower on the abs needs 14 levels (1 enclosed ref; 5/6 levels
 * stall at 43); extra source-level zz reads, `zz = zz;`, and a
 * dead-boundary copy (`ry = zz;`) all add 0 refs (deleted before .lreg
 * counts); a block-scoped local cannot take $s0 (crosses no call);
 * `register` is a no-op at -O2; zz-as-selector and the ry-merge land
 * 43-diffs-or-worse and the merge deletes a PSX.SYM-attested local.
 * Splitting the rival `i` (mode/angle/scan/body roles) is impossible on
 * two independent measured axes: only its loop tail crosses calls, so
 * every other piece colors caller-saved (t0/a0/a3 measured) while its
 * byte sites demand $s1 — the single-pseudo `i` inherits $s1 purely
 * from its tail; and every tail-containing piece (5298-8640) outranks
 * natural zz (18/160 -> 4500) and shares $s0 freely with ry (a set-dest
 * and an input dying at the same insn never conflict), so it takes $s0
 * before zz allocates. Every partition measured 114-124 diffs or a
 * length mismatch. The four fences are irreplaceable.
 * PROVENANCE (2026-08-30, demo-binary witness + 83-config sweep): the
 * JP demo's DAH (0x80024de0, HUMAN.C:155) is instruction-identical to
 * retail in every shared region and compiles the surviving fenced
 * statements fence-free with zz already in $s0 — because the demo's
 * `i` never crosses a call and colors caller-saved $a2. Retail's added
 * damage arm makes i live across GetDirection/SetNowMotion/Sound,
 * promoting it into the callee-saved file ($s1; the whole map shifts
 * down one, giving retail its ninth save, $s8) — that promotion CREATES
 * the i-vs-zz rivalry these fences resolve. Two of the four fenced
 * statements do not even exist in demo-era source, so the fences are
 * not release-emptied debug-macro fossils (the demo's real debug idiom
 * was bare FntPrint dumps, one block of which sat after the map probe
 * — deleted in retail with no residue). Wrong-flags and wrong-compiler
 * are excluded by measurement: 83 configurations across cc1 2.6/2.7.2/
 * 2.8.0/2.8.1/gs107 never bring flat DAH under 118 differing bytes,
 * every length-preserving flag is provably inert here, and 2.7.2-class
 * compilers cannot reproduce even the fenced bytes at any setting.
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
    /* The cast (not &= 0xff) makes the reload an lbu: byte-required
     * (verified against the .s). */
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
        VECTOR *probe;
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
        probe = locate;
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
            if (abs_x > 0x50)
            {
                goto use_conflict_position;
            }
            abs_z = vector->vz;
            if (abs_z < 0)
            {
                abs_z = -abs_z;
            }
            if (abs_z <= 0x50)
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
        probe = locate; /* duplicated on both paths: the $L7 block is in the bytes */
    probe_map:
        GetAreaMapVector(GlobalAreaMap, call_map, probe, human->width,
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
                    Sound(human, 0x49);
                    SetCameraMode(CMODE_FALL);
                }
                else
                {
                    Sound(human, 8);
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
    else
    {
        if (vector->vy > 0 || map->level == LEVEL_NONE)
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
        /* zz weight fence -- see header */
        do
        {
            do
            {
                zz = map->level;
            } while (0);
        } while (0);
        if (zz == LEVEL_NONE)
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
                    i = ((u16)human->width << 16) >> 18; /* width / 4; the u16 view is the retail lhu access width, and the sll16/sra18 pair (not >> 2) is in the bytes */
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
                if (angle_abs < 1800 || human != StagePlayer || map->height != 0)
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
                    /* zz weight fence -- see header */
                    do
                    {
                        do
                        {
                            do
                            {
                                zz >>= 1;
                            } while (0);
                        } while (0);
                    } while (0);
                }
                locate->vx -= xx;
                locate->vz -= zz;
            }
        }
    }

    if (object->attribute & MODEL_ATTR_CONFLICT)
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

                locate->vx -= ((ConflictDistance.vx >= 0)
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
                    /* zz weight fence -- see header */
                    do
                    {
                        do
                        {
                            do
                            {
                                locate->vz = zz;
                            } while (0);
                        } while (0);
                    } while (0);
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
                    /* zz weight fence -- see header */
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
                    direction = MOT_DAMAGE_BACK_LIGHT;
                    if (direction_abs < 1100)
                    {
                        direction = MOT_DAMAGE;
                    }
                    SetNowMotion(human, direction, 1);
                    Sound(human, 6);
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
