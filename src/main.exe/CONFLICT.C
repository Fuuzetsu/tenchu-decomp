#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "vmemory.h"

/*
 * The demo symbols place DisposeAreaMap immediately after LoadAreaMap. The
 * shipped executable moves it to the end of this unit; the definitions below
 * follow the retail order, with both orders recorded in the translation-unit
 * manifest.
 */

extern char msg_no_area_data[]; /* NO AREA DATA */
extern char msg_conflict_regist_failure[]; /* CONFLICT REGIST FAILURE */
extern long AreaMapLastY;
extern s16 direction[N_MAP_PROBE_DIRECTIONS][2];
extern VECTOR cv;


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long * LoadAreaMap(unsigned long *adr);
 *     CONFLICT.C:47, 23 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned long * adr
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct NodeIndexType *FieldIndex;
 *     extern struct AreaNodeType *FieldArea;
 * END PSX.SYM */

AreaMapType *LoadAreaMap(AreaMapType *adr)
{
    NodeIndexType *map;
    s16 j;
    long idx0;

    map = (NodeIndexType *)adr;
    if (adr == 0)
        SystemOut(msg_no_area_data);

    j = 0;
    idx0 = ((NodeIndexType *)adr)->index;
    if (idx0 != 0)
    {
        do
        {
            map[j].index += (long)adr;
            map[j].y += 2;
            if (map[j].n < 0)
            {
                ((IndexArrayType *)map[j].index)->index =
                    ((IndexArrayType *)map[j].index)->index + (long)adr;
            }
            j++;
        } while (map[j].index != 0);
        idx0 = ((NodeIndexType *)adr)->index;
    }
    GlobalAreaMap = adr;
    FieldIndex = (NodeIndexType *)adr;
    idx0 = ((NodeIndexType *)adr)->index;
    FieldArea = (AreaNodeType *)idx0;
    return adr;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long ComputeAreaLevel(struct AreaNodeType *node, long x, long z);
 *     CONFLICT.C:83, 20 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $t0       struct AreaNodeType * node
 *     param $a1       long x
 *     param $a2       long z
 *     reg   $a0       short yy
 * END PSX.SYM */

long ComputeAreaLevel(AreaNodeType *node, long x, long z)
{
    short dz, zspan;
    short dx, xspan;
    short yy;
    int mask;

    dz = z - (u16)node->z1;
    zspan = (u16)node->z2 - (u16)node->z1 + 1;
    dx = x - (u16)node->x1;
    xspan = (u16)node->x2 - (u16)node->x1 + 1;

    mask = 1 << (((dz << 2) / zspan) * 4 + ((dx << 2) / xspan));
    if (((u16)node->division & mask) != 0)
    {
        yy = node->y;

        switch (node->attribute & (MAP_SLOPE_X | MAP_SLOPE_Z))
        {
        case MAP_SLOPE_X:
            yy = yy + dx * node->dy / xspan;
            break;
        case MAP_SLOPE_Z:
            yy += dz * node->dy / zspan;
            break;
        }
        return yy;
    }
    return LEVEL_NONE;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long GetAreaMapLevel(unsigned long *area, long x, long y, long z, int mode);
 *     CONFLICT.C:107, 69 src lines, frame 64 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned long * area
 *     param $s5       long x
 *     param stack+8   long y
 *     param $s6       long z
 *     param stack+16  int mode
 *     stack sp+16     short mode
 *     reg   $s3       struct NodeIndexType * index
 *     reg   $s0       struct AreaNodeType * node
 *     reg   $fp       long y2
 *     reg   $s4       long yy
 *     reg   $v1       long sy
 *     reg   $s1       long n
 *     reg   $s7       long nn
 *
 * Globals it touches, as the original declared them:
 *     extern struct NodeIndexType *FieldIndex;
 *     extern short FieldAttrib;
 *     extern struct AreaNodeType *FieldArea;
 * END PSX.SYM */

/* mode bits: 1 = also accept floors up to 1500 units below y
 * (step-down tolerance); 2 = return the height difference (level - y)
 * instead of the level; 4 = keep floors deeper than 1000 below (else
 * "no floor"); 8 = first-hit sampling — take the first containing
 * node without the highest-below pick (DrawSnow); 0x10 = same-height
 * fast path reusing the cached FieldArea via AreaMapLastY. Returns
 * 0x80000000 for no floor; a base-material-2 node (the buoyant
 * surface) reports no floor, and MAP_RESULT_FINAL accepts the current
 * result without examining the rest of that leaf list. */
long GetAreaMapLevel(AreaMapType *area, long x, long y, long z,
                     enum area_level_mode_flag mode)
{
    long n;
    long *row;
    NodeIndexType *index;
    AreaNodeType *node;
    AreaNodeType *list;
    long yy;
    long nn;
    long y2;
    short mode16 = mode;
    long first_hit;
    long sy;
    long ret;
    short qx;
    short qz;

    index = FieldIndex;
    x = x / 10;
    FieldAttrib = MAP_ATTRIBUTE_UNRESOLVED;
    z = z / 10;
    y2 = y / 10;
    yy = LEVEL_NONE;
    /* Wrapper retained for code layout; its original source construct is unknown. */
    do
    {
        if (mode & AREA_LEVEL_STEP_DOWN)
            y2 -= 150;

        if (y2 == AreaMapLastY && (mode & AREA_LEVEL_REUSE_CACHED) &&
            FieldArea->x1 <= x && x <= FieldArea->x2 &&
            FieldArea->z1 <= z && z <= FieldArea->z2)
        {
            yy = ComputeAreaLevel(FieldArea, x, z);
        }
        AreaMapLastY = y2;

        if (index != (NodeIndexType *)area)
        {
        down:
            if (y2 < index->y)
            {
                index--;
                if (index != (NodeIndexType *)area)
                    goto down;
            }
        }
        if (index->index != 0)
        {
        up:
            if (index->y < y2)
            {
                index++;
                if (index->index != 0)
                    goto up;
            }
            if (index->index != 0)
            {
                row = &index->index;
                first_hit = mode16 & AREA_LEVEL_FIRST_HIT;
            loop:
                if (yy == (u32)LEVEL_NONE)
                {
                    if (NODE_INDEX_ROW_FIELD(row, x1) <= x &&
                        x <= NODE_INDEX_ROW_FIELD(row, x2) &&
                        NODE_INDEX_ROW_FIELD(row, z1) <= z &&
                        z <= NODE_INDEX_ROW_FIELD(row, z2))
                    {
                        nn = NODE_INDEX_ROW_FIELD(row, n);
                        list = (AreaNodeType *)*row;
                        n = 0;
                        if (nn < 0)
                        {
                            qx = (x - NODE_INDEX_ROW_FIELD(row, x1)) *
                                     AREA_INDEX_AXIS_SIZE /
                                 (NODE_INDEX_ROW_FIELD(row, x2) -
                                  NODE_INDEX_ROW_FIELD(row, x1));
                            qz = (z - NODE_INDEX_ROW_FIELD(row, z1)) *
                                     AREA_INDEX_AXIS_SIZE /
                                 (NODE_INDEX_ROW_FIELD(row, z2) -
                                  NODE_INDEX_ROW_FIELD(row, z1));
                            n = ((IndexArrayType *)*row)->array[qz][qx];
                            if (n == AREA_NODE_INDEX_NONE)
                                goto next;
                            list = (AreaNodeType *)((IndexArrayType *)*row)->index;
                            nn = -nn;
                        }
                        if (n < nn)
                        {
                            node = (AreaNodeType *)(n * (long)sizeof(AreaNodeType) +
                                                    (long)list);
                        inner:
                            if (z >= node->z1)
                            {
                                if (node->x1 <= x && x <= node->x2 && z <= node->z2)
                                {
                                    FieldIndex = index;
                                    FieldArea = node;
                                    if (first_hit)
                                    {
                                        if (node->division == AREA_DIVISION_ALL)
                                            yy = node->y;
                                        else
                                            yy = ComputeAreaLevel(node, x, z);
                                        FieldAttrib = FieldArea->attribute;
                                        goto next;
                                    }
                                    sy = ComputeAreaLevel(node, x, z);
                                    if ((yy == (u32)LEVEL_NONE || sy < yy) && y2 <= sy)
                                    {
                                        FieldAttrib = FieldArea->attribute;
                                        yy = sy;
                                        if (FieldAttrib & MAP_RESULT_FINAL)
                                            goto next;
                                    }
                                }
                                n++;
                                node++;
                                if (n < nn)
                                    goto inner;
                            }
                        }
                    }
                next:
                    row += NODE_INDEX_ROW_WORDS;
                    index++;
                    if (*row != 0)
                        goto loop;
                }
            }
        }
        if (yy == (u32)LEVEL_NONE || FieldAttrib & MAP_BUOYANT)
            return LEVEL_NONE;
        yy = yy * 10;
        y2 = yy - y;
        if (y2 < -1000 && (mode16 & AREA_LEVEL_ALLOW_DEEP) == 0)
        {
            return LEVEL_NONE;
        }
        ret = yy;
    } while (0);
    if (mode16 & AREA_LEVEL_RETURN_DELTA)
        ret = y2;
    return ret;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long GetAreaMapVector(unsigned long *area, struct MapVector *mvp, struct VECTOR *pos, long wide, int mode);
 *     CONFLICT.C:180, 39 src lines, frame 64 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param stack+0   unsigned long * area
 *     param $s0       struct MapVector * mvp
 *     param $s1       struct VECTOR * pos
 *     param $s7       long wide
 *     param stack+16  int mode
 *     reg   $s4       short mode
 *     reg   $s2       short i
 *     reg   $s1       short v
 *     reg   $a0       long level
 *     reg   $s6       long x
 *     reg   $s3       long y
 *     reg   $s5       long z
 *
 * Globals it touches, as the original declared them:
 *     extern short FieldAttrib;
 *     extern struct AreaNodeType *FieldArea;
 *     extern struct NodeIndexType *FieldIndex;
 * END PSX.SYM */

/* The 4-direction movement probe. The centre query fills level/attrib/
 * area/index (mode forwards to GetAreaMapLevel with the cache bit 0x10
 * stripped; the four neighbour queries keep it). Each compass
 * neighbour at `wide` distance then classifies into: vector — no
 * floor there, or a >500 drop (unless mode bit 4 or a HIT/PUSH
 * attribute) — the 4-bit wall-direction code RefrectVector maps to
 * deflection angles; angleL — the neighbour floor is HIGHER than the
 * centre's (a wall or step up); angleH — lower. With no floor at the
 * centre the result is fabricated: MAP_BUOYANT, all four masks set. */
long GetAreaMapVector(AreaMapType *area, MapVector *mvp, VECTOR *pos, long wide, int mode)
{
    long x, y, z;
    long level2;
    short i;
    short v;
    long initial_level;
    short m;
    long rawmode;
    long mode2;

    x = pos->vx;
    y = pos->vy;
    z = pos->vz;

    mvp->level = GetAreaMapLevel(area, x, y, z, (short)(mode & ~AREA_LEVEL_REUSE_CACHED));
    mvp->attrib = FieldAttrib;
    mvp->area = FieldArea;
    mvp->index = FieldIndex;
    rawmode = (mode2 = mode);
    initial_level = mvp->level;
    if (mvp->attrib == 0)
    {
        mode = rawmode;
    }
    if ((initial_level ^ (u32)LEVEL_NONE) == 0)
    {
        mvp->height = 0;
        if (!(mode & AREA_LEVEL_ALLOW_DEEP))
        {
            /* Retail keeps three identical branches here; their original distinctions are unknown. */
            if (wide != 0)
            {
                if (y != 0)
                {
                    mvp->attrib = MAP_BUOYANT;
                    mvp->angleH = MAP_PROBE_ALL;
                    mvp->angleL = MAP_PROBE_ALL;
                    mvp->vector = MAP_PROBE_ALL;
                    return initial_level;
                }
                else
                {
                    mvp->attrib = MAP_BUOYANT;
                    mvp->angleH = MAP_PROBE_ALL;
                    mvp->angleL = MAP_PROBE_ALL;
                    mvp->vector = MAP_PROBE_ALL;
                    return initial_level;
                }
            }
            else
            {
                mvp->attrib = MAP_BUOYANT;
                mvp->angleH = MAP_PROBE_ALL;
                mvp->angleL = MAP_PROBE_ALL;
                mvp->vector = MAP_PROBE_ALL;
                return initial_level;
            }
        }
    }
    else
    {
        mvp->height = mvp->level - pos->vy;
    }

    i = 0;
    v = 1;
    mvp->angleH = 0;
    mvp->angleL = 0;
    mvp->vector = 0;
    m = (short)mode2;
    while (i < N_MAP_PROBE_DIRECTIONS)
    {
        level2 = GetAreaMapLevel(area, x + direction[i][0] * wide, y, z + direction[i][1] * wide, m);
        if (level2 == (u32)LEVEL_NONE ||
            ((level2 - y < -500) && !(mode2 & AREA_LEVEL_ALLOW_DEEP) &&
             !(((u16)mvp->attrib | (u16)FieldAttrib) &
               (MAP_SLOPE_X | MAP_SLOPE_Z))))
        {
            mvp->vector |= v;
        }
        else
        {
            if (mvp->level < level2)
            {
                mvp->angleL |= v;
            }
            else if (level2 < mvp->level)
            {
                mvp->angleH |= v;
            }
            mode = rawmode;
        }
        v <<= 1;
        i++;
    }
    return mvp->level;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct VECTOR * GetAreaMapPassage(unsigned long *area, struct VECTOR *pos, struct SVECTOR *vect, short n);
 *     CONFLICT.C:223, 31 src lines, frame 72 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s4       unsigned long * area
 *     param $a1       struct VECTOR * pos
 *     param $s1       struct SVECTOR * vect
 *     param $s3       short n
 *     reg   $a0       struct AreaNodeType * node
 *     stack sp+24     long [2] x
 *     stack sp+32     long [2] z
 *     stack sp+40     long [2] y
 *
 * Globals it touches, as the original declared them:
 *     extern struct AreaNodeType *FieldArea;
 *     extern struct NodeIndexType *FieldIndex;
 * END PSX.SYM */

VECTOR *GetAreaMapPassage(AreaMapType *area, VECTOR *pos, SVECTOR *vect, short n)
{
    long x[2], z[2], y[2];
    long xmin, xmax, ymin, ymax;
    short initial;
    short count;
    AreaNodeType *node;

    cv = *pos;
    initial = n;
    if (n <= 0)
    {
        initial = 100;
    }
    count = initial;

    while ((y[0] = GetAreaMapLevel(area, cv.vx, cv.vy, cv.vz,
                                   AREA_LEVEL_DEFAULT)) != LEVEL_NONE)
    {
        node = FieldArea;
        x[0] = node->x1 * 10;
        x[1] = node->x2 * 10;
        z[0] = node->z1 * 10;
        z[1] = node->z2 * 10;
        if (FieldIndex != (NodeIndexType *)area)
        {
            ymax = FieldIndex[-1].y * 10;
        }
        else
        {
            ymax = -1000000;
        }
        xmin = x[0];
        xmax = x[1];
        ymin = y[0];
        y[1] = ymax;
    inner:
        cv.vx += vect->vx;
        cv.vy += vect->vy;
        cv.vz += vect->vz;
        count--;
        if (count == 0)
        {
            return 0;
        }
        if (xmin <= cv.vx && cv.vx <= xmax && ymin <= cv.vy && cv.vy <= y[1] && z[0] <= cv.vz && cv.vz <= z[1])
        {
            goto inner;
        }
    }
    cv.vx -= vect->vx;
    cv.vy -= vect->vy;
    cv.vz -= vect->vz;
    return &cv;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitConflict(void);
 *     CONFLICT.C:260, 19 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct VECTOR UnitVector2;
 *     extern struct SVECTOR UnitVector;
 *     extern struct ModelType *ConflictModel;
 *     extern struct SVECTOR ConflictDistance;
 *     extern short ConflictObjects;
 * END PSX.SYM */

void InitConflict(void)
{
    short i;

    for (i = 0; i < N_CONFLICT_OBJECTS; i++)
    {
        ConflictObject[i].model = 0;
        ConflictObject[i].common = (void *)CONFLICT_OWNER_NONE;
        ConflictObject[i].position = UnitVector2;
        ConflictObject[i].offset = UnitVector;
        ConflictObject[i].size = UnitVector;
        memset(ConflictObject[i].result, 0, sizeof(ConflictObject[i].result));
    }
    ConflictModel = 0;
    ConflictDistance = UnitVector;
    ConflictObjects = 0;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short InsertConflict(struct ModelType *model);
 *     CONFLICT.C:283, 23 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct ModelType * model
 *
 * Globals it touches, as the original declared them:
 *     extern short ConflictObjects;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct VECTOR UnitVector2;
 *     extern struct SVECTOR UnitVector;
 * END PSX.SYM */

conflict_id InsertConflict(ModelType *model)
{
    u16 cnt;
    int idx;
    int id;

    id = model->id;
    if (id != CONFLICT_NONE)
    {
        return id;
    }
    if (ConflictObjects >= N_CONFLICT_OBJECTS)
    {
        SystemOut(msg_conflict_regist_failure);
    }
    cnt = ConflictObjects;
    ConflictObjects = cnt + 1;
    idx = (short)cnt;
    ConflictObject[idx].model = model;
    ConflictObject[idx].common = (void *)CONFLICT_OWNER_NONE;
    ConflictObject[idx].position = UnitVector2;
    ConflictObject[idx].offset = UnitVector;
    ConflictObject[idx].size = UnitVector;
    memset(ConflictObject[idx].result, 0, sizeof(ConflictObject[idx].result));
    model->id = cnt;
    model->attribute = (model->attribute | MODEL_ATTR_COLLIDE) & ~MODEL_ATTR_CONFLICT;
    return idx;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DeleteConflict(struct ModelType *model);
 *     CONFLICT.C:310, 15 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $t3       struct ModelType * model
 *     reg   $t1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern short ConflictObjects;
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */

void DeleteConflict(ModelType *model)
{
    short i;
    short count;

    if (model->id != CONFLICT_NONE)
    {
        i = 0;
        while (i < ConflictObjects)
        {
            count = ConflictObjects - 1;
            if (ConflictObject[i].model == model)
            {
                ConflictObjects = count;
                ConflictObject[i] = ConflictObject[count];
                ConflictObject[i].model->id = i;
            }
            else
            {
                i++;
            }
        }
        model->id = CONFLICT_NONE;
        model->attribute = model->attribute & ~(MODEL_ATTR_CONFLICT | MODEL_ATTR_COLLIDE);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ComputeAllConflict(void);
 *     CONFLICT.C:329, 50 src lines, frame 72 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s1       struct ConflictObjectType * confop
 *     reg   $s0       struct ModelType * model
 *     stack sp+16     struct MATRIX mat
 *     reg   $s2       short i
 *     reg   $t0       short j
 *
 * Globals it touches, as the original declared them:
 *     extern short ConflictObjects;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct ModelType World;
 * END PSX.SYM */

void ComputeAllConflict(void)
{
    short i;
    short j;
    ModelType *model;
    ConflictObjectType *confop;
    MATRIX mat;
    int d;

    for (i = 0; i < ConflictObjects; i++)
    {
        confop = &ConflictObject[i];
        model = confop->model;
        if (model->attribute & MODEL_ATTR_COLLIDE)
        {
            memset(confop->result, 0, sizeof(confop->result));
            confop->offset.pad = 0;
            model->attribute &= ~MODEL_ATTR_CONFLICT;
            if (model->locate.super == &World.locate)
            {
                confop->position.vx = model->locate.coord.t[0] + confop->offset.vx;
                confop->position.vy = model->locate.coord.t[1] + confop->offset.vy;
                confop->position.vz = model->locate.coord.t[2] + confop->offset.vz;
            }
            else
            {
                GsGetLw(&model->locate, &mat);
                GsSetLsMatrix(&mat);
                RotTrans(&confop->offset, &confop->position, (long *)0);
            }
        }
    }

    for (i = 0; i < ConflictObjects; i++)
    {
        if (ConflictObject[i].model->attribute & MODEL_ATTR_COLLIDE)
        {
            for (j = i + 1; j < ConflictObjects; j++)
            {
                ConflictObjectType *other = &ConflictObject[j];

                if (other->model->attribute & MODEL_ATTR_COLLIDE)
                {
                    d = __builtin_abs(other->position.vy - ConflictObject[i].position.vy);
                    if (d <= ConflictObject[i].size.vy + other->size.vy)
                    {
                        d = __builtin_abs(other->position.vz - ConflictObject[i].position.vz);
                        if (d <= ConflictObject[i].size.vz + other->size.vz)
                        {
                            d = __builtin_abs(other->position.vx - ConflictObject[i].position.vx);
                            if (d <= ConflictObject[i].size.vx + other->size.vx)
                            {
                                ConflictObject[i].result[j] =
                                    other->size.pad |
                                    CONFLICT_LIVE;
                                ConflictObject[j].result[i] =
                                    ConflictObject[i].size.pad |
                                    CONFLICT_LIVE;
                                ConflictObject[i].model->attribute =
                                    ConflictObject[i].model->attribute | MODEL_ATTR_CONFLICT;
                                other->model->attribute = other->model->attribute | MODEL_ATTR_CONFLICT;
                                ConflictObject[i].offset.pad++;
                                other->offset.pad++;
                            }
                        }
                    }
                }
            }
        }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetConflictResult(struct ModelType *model, short index);
 *     CONFLICT.C:382, 25 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct ModelType * model
 *     param $a2       short index
 *     reg   $t0       short i
 *     reg   $t1       short idx
 *
 * Globals it touches, as the original declared them:
 *     extern short ConflictObjects;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct SVECTOR ConflictDistance;
 *     extern struct ModelType *ConflictModel;
 * END PSX.SYM */

conflict_id GetConflictResult(ModelType *model, conflict_id index)
{
    conflict_id idx;
    int id;
    short i;
    int k;

    id = model->id;
    idx = model->id;
    if (id != CONFLICT_NONE)
    {
        if ((model->attribute & MODEL_ATTR_COLLIDE) == 0)
        {
            return CONFLICT_NONE;
        }
        i = 0;
        if (index < 0)
        {
            index = 0;
            if (index < ConflictObjects)
            {
                for (; index < ConflictObjects; index++)
                {
                    if (ConflictObject[id].result[index] != 0)
                    {
                        i++;
                        if (i > ConflictObject[id].offset.pad)
                        {
                            return CONFLICT_NONE;
                        }
                        if ((ConflictObject[id].result[index] & CONFLICT_CONSUMED) == 0)
                        {
                            break;
                        }
                    }
                }
            }
            else
            {
                return CONFLICT_NONE;
            }
        }
        k = index;
        if (k < ConflictObjects)
        {
            if (ConflictObject[idx].result[k] != 0)
            {
                ConflictObject[idx].result[k] |= CONFLICT_CONSUMED;
                ConflictModel = ConflictObject[k].model;
                ConflictDistance.vx = (short)ConflictObject[k].position.vx - (short)ConflictObject[idx].position.vx;
                ConflictDistance.vy = (short)ConflictObject[k].position.vy - (short)ConflictObject[idx].position.vy;
                ConflictDistance.vz = (short)ConflictObject[k].position.vz - (short)ConflictObject[idx].position.vz;
                return index;
            }
        }
        return CONFLICT_NONE;
    }
    return CONFLICT_NONE;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DisposeAreaMap(unsigned long *area);
 *     CONFLICT.C:74, 4 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned long * area
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

void DisposeAreaMap(AreaMapType *area)
{
    if (area == 0)
    {
        if (GlobalAreaMap != 0)
        {
            area = GlobalAreaMap;
            GlobalAreaMap = 0;
        }
    }
    vfree(area);
}
