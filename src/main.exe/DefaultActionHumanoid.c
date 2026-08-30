#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

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
 * Register-allocation notes, verified against cc1 2.8.1's own -dl/-dg/-dS
 * dumps and its source (flow.c, global.c, sched.c) this session.
 *
 * WHY reflect_dz EXISTS (the `reflect_dz = zz` on all three reflect
 * paths, the `&= ~1` before the halving, and the final negate-and-add):
 * $s0 must go to a value that
 * beats `i` in global-alloc priority (floor_log2(refs)*refs/live_length,
 * refs weighted by loop depth at flow time) AND that conflicts with i's
 * live ranges, or find_reg simply gives $s0 to both. Written with zz
 * doing everything (halved in place, subtracted at the tail), zz counts
 * 18 weighted refs over 160 live insns (4500) against i's ~9100-9950: i
 * takes $s0 and every callee-saved assignment shifts off the retail
 * bytes. Splitting the applied displacement into reflect_dz builds a
 * short, hot, call-crossing carrier instead: three path copies + mask
 * + shift + negate + the tail use = 10 refs over 24 insns (12500),
 * with the turn-block copy above the `if (i > 0)` test so reflect_dz
 * overlaps (and thus conflicts with) i, and before MoveHumanoid so it
 * crosses a call. reflect_dz takes $s0 first, i lands on $s1, and zz
 * - dying at each `reflect_dz = zz` - shares $s0 afterward, so the
 * copies coalesce to nothing. The mask folds into the sra
 * (simplify_shift_const) and the negate folds into the add
 * (A + (-B) -> A - B), both AFTER flow counts them: no instruction is
 * generated for either. Load-bearing details, all measured: the three
 * copies must sit in separate blocks (a same-block copy combines into
 * the `&=` and stops crossing the call); the turn-block copy must
 * precede the `if (i > 0)` (below it, nothing conflicts with i and
 * find_reg gives $s0 to BOTH); and the mask+shift must be two
 * statements (the fused form loses a counted ref). Re-using the dead
 * PSX.SYM ry as the carrier also measures byte-identical (thinner
 * margin, 8 refs); reflect_dz is the joint Claude+Codex pick for its
 * narration and headroom. The conflict loop has no equivalent
 * partition: GetDirection's result must die before SetNowMotion and
 * the vz save-restore crosses nothing, so no loop-range piece can
 * cross a call honestly - measured and argued from both sides. This is not a compiler quirk of ours: GCC
 * 2.8.0-psx, 2.8.1-psx and the gs107 build emit byte-identical code
 * for this function, 2.7.2-class codegen cannot reproduce even its
 * instruction count, and every era-plausible flag measures inert on
 * this allocation. (Historical note: before the fission this file
 * carried a seven-level do{}while(0) nest that boosted zz itself to 32
 * weighted refs - see git history for that mechanism's full story.)
 *
 * WHY RETAIL NEEDS IT AT ALL: the JP demo's DefaultActionHumanoid
 * (0x80024de0; Oct 1997, so necessarily a pre-2.8.0-era compiler) has no
 * damage arm in the conflict loop. Its i never lives across a call,
 * colors caller-saved $a2, and its zz takes $s0 uncontested. Retail's
 * added damage arm (GetDirection/SetNowMotion/Sound) makes i live across
 * calls, promoting it into the callee-saved file — that promotion
 * creates the i-vs-zz contest the ry fission settles. The demo's debug idiom
 * was bare FntPrint dumps (still compiled into the demo binary), deleted
 * in retail without residue.
 *
 * THE THREE DBG SITES IN THE CONFLICT ARM (empty do{}while(0) once
 * expanded for release): pure scheduling, no
 * weight (nothing inside them, so nothing is ref-boosted). A loop-note
 * pair bounds a sched1 region even when empty, and these keep the four
 * conflict-record loads in retail's order: written flat, sched's
 * backward pass places one of the record loads next to
 * ConflictObject[object_id]'s load (sched.c's potential_hazard prefers a
 * memory op right after a memory op) and no plain statement order
 * reaches the retail sequence address / lh id / lh size / lw position /
 * index chain / lw position — measured, and visible in the -dS trace.
 * Each of the three is individually load-bearing, and the size-then-
 * position load order inside the third region is too (all measured).
 * Some region edge is unavoidable in flat C: yy carries both the
 * position load and the later turn-arm value (both $a0 in the bytes;
 * one PSX.SYM local), so it has two assignments, REG_N_SETS != 1
 * denies its load sched1's birthing bump, and nothing else holds the
 * backward pass off it. Keywords cannot substitute (all measured):
 * volatile on the short field is byte-visible — it de-fuses lh into
 * lhu + sll/sra, so the retail bytes rule volatile out on their own;
 * a volatile s32 read orders only memory ops, not the address ALU
 * chain that must stay below the record loads; const and register are
 * inert. The likely original spelling is no spelling at all: an empty
 * do{}while(0) is exactly what the period's standard debug-print
 * macro (#define DBG(x) do { } while (0)) leaves in a release build,
 * and the debug-side wrapper do { FntPrint x; } while (0) is measured
 * byte-invisible around a live call. The demo binary carries this
 * function's actual debug prints: an if/else pair straight after the
 * map probe dumping the probe result --
 *     "l(ia) h%d v%x ah%x al%x %04x\n"   (map->level == LEVEL_NONE)
 *     "l%d h%d v%x ah%x al%x %04x\n"     (level, height, vector,
 *                                          angleH, angleL, attrib)
 * (strings at 0x800106d4/0x800106f4; 31 FntPrint sites across the
 * demo build). That pair now stands in the code below under the DBG
 * macro, measured byte-inert. Retail kept the machinery -- FntPrint
 * is still linked and called (the ADT debug menu), and
 * debug_printf_/debug_msg_open_ are RETAIL-era additions -- so
 * per-frame dumps compiled out while diagnostics stayed. The three
 * load-bearing empty sites sit exactly where a dev debugging the new
 * damage arm would dump the collision record. Three deleted debug prints explain the barriers
 * without anyone typing a bare one-shot loop.
 *
 * Retail narrows the recovered `long i` at both map-query calls; the
 * explicit casts keep the shared API's promoted `int mode` visible.
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
