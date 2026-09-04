#include "common.h"
#include "main.exe.h"
#include "action.h"
#include "item.h"
#include "model.h"
#include "vmemory.h"

/*
 * Demo ACTION.C orders these routines as LoadMotion, SearchMotion,
 * SetupMotionRegist, SetupMotionManager, DisposeMotionManager, GetMotionID,
 * UpdateMotion, PlayMotion, HoldMotion, SweepMotion, ActiveMotion,
 * SetupSpline, UpdateSplineControl, and GetSpline. Both the trial and retail
 * builds use the definition order below; both orders are recorded in the
 * translation-unit manifest.
 */

extern char msg_no_motion_data[]; /* NO MOTION DATA */

void SetupSpline(MotionManager *mmp);
void GetSpline(SVECTOR *vect, SplineControlType *spc, short cnt);

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct MotionDataType * SearchMotion(short id);
 *     ACTION.C:100, 22 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a1       short id
 *     reg   $a3       struct MotionPackType * mpd
 *     reg   $a2       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionPackType *CommonMotion;
 *     extern struct MotionPackType *PlayerMotion;
 *     extern struct MotionPackType *StageMotion;
 * END PSX.SYM */

MotionDataType *SearchMotion(short id)
{
    MotionPackType *mpd;
    short i;

    mpd = CommonMotion;
    if (mpd != 0)
    {
        for (i = 0; i < mpd->n; i++)
        {
            if (mpd->motion[i]->id == id)
            {
                return mpd->motion[i];
            }
        }
    }
    mpd = PlayerMotion;
    if (mpd != 0)
    {
        for (i = 0; i < mpd->n; i++)
        {
            if (mpd->motion[i]->id == id)
            {
                return mpd->motion[i];
            }
        }
    }
    mpd = StageMotion;
    if (mpd != 0)
    {
        for (i = 0; i < mpd->n; i++)
        {
            if (mpd->motion[i]->id == id)
            {
                return mpd->motion[i];
            }
        }
    }
    return 0;
}
/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short UpdateMotion(struct MotionManager *mmp, short mid);
 *     ACTION.C:182, 34 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct MotionManager * mmp
 *     param $t0       short mid
 *     reg   $a0       struct MotionRegistType * mrp
 *     reg   $a2       short i
 *     reg   $a1       short j
 *     reg   $a3       short * xyz
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionRegistType MOTcommon[41];
 * END PSX.SYM */

motion_update_result UpdateMotion(MotionManager *mmp, motion_id mid)
{
    MotionRegistType *mrp;
    MotionDataType *md;
    s16 i;
    s16 j;
    s16 *xyz;
    s32 t;
    s16 sweep;

    if (mid == mmp->mid)
        return MOTION_UPDATE_UNCHANGED;

    mrp = mmp->motreg;
    i = 0;
    while (mrp[i].mid != mid)
    {
        if (mrp[i].mid == MOTION_ID_NONE)
            break;
        i++;
    }
    if (mrp[i].motion == 0)
    {
        mrp = MOTcommon;
        i = 0;
        while (mrp[i].mid != mid)
        {
            if (mrp[i].mid == MOTION_ID_NONE)
                break;
            i++;
        }
        if (mrp[i].motion == 0)
            return MOTION_UPDATE_NOT_FOUND;
    }

    md = mrp[i].motion;
    mmp->mid = mid;
    mmp->motion = md;
    sweep = md->sweep;
    mmp->count = sweep;
    if (sweep & MOTION_BYTE_SIGN_BIT)
        mmp->count = sweep - MOTION_BYTE_RANGE;
    mmp->loop = 0;
    i = (mmp->motion->n < mmp->model->n) ? mmp->motion->n : mmp->model->n;
    mmp->n = i;
    SetupSpline(mmp);

    for (i = 0; i < mmp->model->n; i++)
    {
        xyz = &mmp->model->object[i]->rotate.vx;
        for (j = 0; j < 3; j++)
        {
            t = xyz[j];
            if (((t < 0) ? -t : t) > ANGLE_HALF)
                xyz[j] = (xyz[j] < 0) ? t + ANGLE_FULL : t - ANGLE_FULL;
            xyz[j] = xyz[j] % ANGLE_FULL;
        }
    }
    return MOTION_UPDATE_CHANGED;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short HoldMotion(struct MotionManager *mmp);
 *     ACTION.C:242, 26 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct MotionManager * mmp
 *     reg   $s2       struct MotionDataType * mot
 *     reg   $a1       struct ModelType * object
 *     reg   $s0       short i
 * END PSX.SYM */

short HoldMotion(MotionManager *mmp)
{
    MotionDataType *mot;
    ModelType *object;
    short i;

    mot = mmp->motion;
    if (mmp->mask & MOTION_MASK_ROOT)
    {
        object = *mmp->model->object;
        object->locate.coord.t[0] = (s32)mot->locate->x;
        object->locate.coord.t[2] = (s32)mot->locate->z;
        object->locate.coord.t[1] =
            ((s32)mmp->model->rotate.pad *
             (s32)mot->locate->y) >>
            12;
    }
    for (i = 0; i < mmp->n; i++)
    {
        if (MOTION_PART_ENABLED(mmp->mask, i))
        {
            object = mmp->model->object[i];
            object->rotate.vx = mot->rotate[i]->x;
            object->rotate.vy = mot->rotate[i]->y;
            object->rotate.vz = mot->rotate[i]->z;
            UpdateCoordinate(object);
        }
    }
    mmp->loop = MOTION_LOOP_FROZEN;
    mmp->count = 0;
    return 0;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SweepMotion(struct MotionManager *mmp);
 *     ACTION.C:272, 31 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct MotionManager * mmp
 *     reg   $s2       struct MotionDataType * mot
 *     reg   $a2       struct ModelType * object
 *     reg   $s1       short i
 * END PSX.SYM */

short SweepMotion(MotionManager *mmp)
{
    MotionDataType *mot;
    ModelType *object;
    short i;
    short count;

    count = -mmp->count++;
    mot = mmp->motion;

    if (mmp->mask & MOTION_MASK_ROOT)
    {
        object = *mmp->model->object;
        object->locate.coord.t[0] +=
            (mot->locate->x - object->locate.coord.t[0]) / count;
        object->locate.coord.t[2] +=
            (mot->locate->z - object->locate.coord.t[2]) / count;
        object->locate.coord.t[1] +=
            (((s32)mmp->model->rotate.pad * mot->locate->y >>
               FIXED_SHIFT) -
             object->locate.coord.t[1]) /
            count;
        object->rotate.vx +=
            (mot->rotate[MODEL_PART_WAIST]->x - object->rotate.vx) /
            count;
        object->rotate.vy +=
            (mot->rotate[MODEL_PART_WAIST]->y - object->rotate.vy) /
            count;
        object->rotate.vz +=
            (mot->rotate[MODEL_PART_WAIST]->z - object->rotate.vz) /
            count;
        UpdateCoordinate(object);
    }

    for (i = MODEL_PART_TORSO; i < mmp->n; i++)
    {
        if (MOTION_PART_ENABLED(mmp->mask, i))
        {
            object = mmp->model->object[i];
            object->rotate.vx +=
                (mot->rotate[i]->x - object->rotate.vx) / count;
            object->rotate.vy +=
                (mot->rotate[i]->y - object->rotate.vy) / count;
            object->rotate.vz +=
                (mot->rotate[i]->z - object->rotate.vz) / count;
            UpdateCoordinate(object);
        }
    }

    return mmp->count;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ActiveMotion(struct MotionManager *mmp);
 *     ACTION.C:307, 31 src lines, frame 48 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct MotionManager * mmp
 *     reg   $s0       short i
 *     reg   $s3       short count
 *     reg   $s1       struct ModelType * object
 *     stack sp+16     struct SVECTOR vect
 * END PSX.SYM */

short ActiveMotion(MotionManager *mmp)
{
    short i;
    short count;
    short frame;
    ModelType *object;
    SVECTOR vect;

    if (mmp->motion->time == 0)
    {
        count = HoldMotion(mmp);
        return count;
    }
    frame = mmp->count;
    mmp->count = frame + 1;
    count = frame;
    if (mmp->mask & MOTION_MASK_ROOT)
    {
        object = *mmp->model->object;
        i = frame;
        GetSpline(&vect, mmp->control, i);
        object->locate.coord.t[0] = (s32)vect.vx;
        object->locate.coord.t[2] = (s32)vect.vz;
        object->locate.coord.t[1] =
            (s32)mmp->model->rotate.pad * (s32)vect.vy >> FIXED_SHIFT;
        GetSpline(&object->rotate, mmp->control + 1, i);
        UpdateCoordinate(object);
    }
    for (i = 1; i < mmp->n; i++)
    {
        if (MOTION_PART_ENABLED(mmp->mask, i))
        {
            object = mmp->model->object[i];
            GetSpline(&object->rotate, mmp->control + (i + 1), count);
            UpdateCoordinate(object);
        }
    }
    count = mmp->count;
    if (mmp->motion->time < count)
    {
        mmp->count = 0;
        return 0;
    }
    return count;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void UpdateSplineControl(struct SplineControlType *spc);
 *     ACTION.C:365, 19 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $t0       struct SplineControlType * spc
 *     reg   $a2       struct MotionElementType * key0p
 *     reg   $a3       struct MotionElementType * key1n
 * END PSX.SYM */

void UpdateSplineControl(SplineControlType *spc)
{
    MotionElementType *key0p;
    MotionElementType *key1n;
    s32 dt;
    s32 slope1;
    s32 slope2;

    key0p = spc->key0;
    if (spc->key0->time != 0)
    {
        key0p--;
    }
    key1n = spc->key1;
    if (spc->key1->time < spc->dd0.pad)
    {
        key1n++;
    }
    {
        s16 diff;

        diff = spc->key1->time - spc->key0->time;
        dt = (s8)diff << 8;
    }
    slope1 = (s16)(dt / (spc->key1->time - key0p->time));
    slope2 = (s16)(dt / (key1n->time - spc->key0->time));
    spc->dd0.vx = (s16)(slope1 * (spc->key1->x - key0p->x) >> 8);
    spc->dd0.vy = (s16)(slope1 * (spc->key1->y - key0p->y) >> 8);
    spc->dd0.vz = (s16)(slope1 * (spc->key1->z - key0p->z) >> 8);
    spc->ds1.vx = (s16)(slope2 * (key1n->x - spc->key0->x) >> 8);
    spc->ds1.vy = (s16)(slope2 * (key1n->y - spc->key0->y) >> 8);
    spc->ds1.vz = (s16)(slope2 * (key1n->z - spc->key0->z) >> 8);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void GetSpline(struct SVECTOR *vect, struct SplineControlType *spc, short cnt);
 *     ACTION.C:388, 34 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct SVECTOR * vect
 *     param $s0       struct SplineControlType * spc
 *     param $s1       short cnt
 * END PSX.SYM */

void GetSpline(SVECTOR *vect, SplineControlType *spc, short cnt)
{
    MotionElementType *key;
    MotionElementType *next;

    key = spc->key1;
    if (key->time < cnt)
    {
        do
        {
            next = key + 1;
            spc->key1 = next;
            key = next;
        } while (next->time < cnt);
        spc->key0 = next - 1;
        UpdateSplineControl(spc);
    }
    else
    {
        key = spc->key0;
        if (key->time > cnt)
        {
            do
            {
                next = key - 1;
                spc->key0 = next;
                key = next;
            } while (cnt < next->time);
            spc->key1 = next + 1;
            UpdateSplineControl(spc);
        }
    }
    SplineFrac = (s16)(((cnt - spc->key0->time) * SPLINE_FRACTION_SCALE) /
                       (spc->key1->time - spc->key0->time));
    if ((s32)SplineFracOld != (s32)SplineFrac)
    {
        SplineFracOld = SplineFrac;
        SplineRow = &HermiteTable[SplineFrac];
    }
    eval_spline_gte_(vect, spc, SplineRow);
}

static __inline__ void *RelocateMotionPointer(void *base, void *relative)
{
    return (void *)((u32)relative + (u32)base);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct MotionPackType * LoadMotion(unsigned long *data);
 *     ACTION.C:77, 19 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned long * data
 *     reg   $s0       struct MotionPackType * mpd
 *     reg   $a1       struct MotionDataType * mmp
 *     reg   $a3       short i
 *     reg   $a2       short j
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionPackType *MotionPack;
 * END PSX.SYM */

MotionPackType *LoadMotion(unsigned long *data)
{
    MotionPackType *mpd;
    MotionDataType *mmp;
    short i;
    short j;

    mpd = (MotionPackType *)data;
    if (mpd == 0)
    {
        SystemOut(msg_no_motion_data);
    }
    for (i = 0; i < mpd->n; i++)
    {
        mpd->motion[i] = RelocateMotionPointer(mpd, mpd->motion[i]);
        mmp = mpd->motion[i];
        if (mmp->n != 0)
        {
            mmp->locate = RelocateMotionPointer(mmp, mmp->locate);
            for (j = 0; j < mmp->n; j++)
            {
                mmp->rotate[j] =
                    RelocateMotionPointer(mmp, mmp->rotate[j]);
            }
        }
    }
    MotionPack = mpd;
    return mpd;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct MotionRegistType * SetupMotionRegist(struct MotionRegistType *mrp);
 *     ACTION.C:126, 9 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct MotionRegistType * mrp
 * END PSX.SYM */

MotionRegistType *SetupMotionRegist(MotionRegistType *mrp)
{
    short i;

    i = 0;
    while (mrp[i].mid != MOTION_ID_NONE)
    {
        mrp[i].motion = SearchMotion(mrp[i].id);
        i++;
    }
    return mrp;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct MotionManager * SetupMotionManager(struct ModelArchiveType *mad, struct MotionRegistType *mot);
 *     ACTION.C:139, 17 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct ModelArchiveType * mad
 *     param $a1       struct MotionRegistType * mot
 * END PSX.SYM */

MotionManager *SetupMotionManager(ModelArchiveType *mad, MotionRegistType *mot)
{
    MotionManager *manager;

    manager = (MotionManager *)valloc(sizeof(MotionManager));
    manager->mid = MOTION_ID_NONE;
    manager->mask = MOTION_MASK_EVERY_PART;
    manager->loop = 0;
    manager->count = 0;
    manager->mode = MOTION_MODE_DEFAULT;
    if (mad != 0)
    {
        manager->n = mad->n;
    }
    else
    {
        manager->n = 2;
    }
    manager->motion = 0;
    manager->model = mad;
    manager->motreg = mot;
    manager->control =
        valloc((manager->n + 1) * sizeof(SplineControlType));
    return manager;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DisposeMotionManager(struct MotionManager *mmp);
 *     ACTION.C:160, 5 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct MotionManager * mmp
 * END PSX.SYM */

void DisposeMotionManager(MotionManager *mmp)
{
    if (mmp != 0)
    {
        vfree(mmp->control);
        vfree(mmp);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetMotionID(struct MotionManager *mmp, short mid);
 *     ACTION.C:169, 9 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct MotionManager * mmp
 *     param $a1       short mid
 * END PSX.SYM */

short GetMotionID(MotionManager *mmp, motion_id mid)
{
    MotionRegistType *registrations;
    s16 i;

    registrations = mmp->motreg;
    i = 0;
    while (registrations[i].mid != MOTION_ID_NONE)
    {
        if (registrations[i].mid == mid)
        {
            break;
        }
        i++;
    }
    return registrations[i].id;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short PlayMotion(struct MotionManager *mmp, short mode);
 *     ACTION.C:220, 17 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct MotionManager * mmp
 *     param $a1       short mode
 * END PSX.SYM */

short PlayMotion(MotionManager *mmp, short mode)
{
    short result;

    if (mmp->loop < 0)
    {
        return 0;
    }
    if (mode != 0)
    {
        if (mmp->count < 0)
        {
            SweepMotion(mmp);
        }
        else
        {
            result = ActiveMotion(mmp);
            if (result == 0)
            {
                result = mmp->loop;
                mmp->loop = result + 1;
            }
        }
    }
    else
    {
        result = mmp->count + 1;
        mmp->count = result;
        if (result > mmp->motion->time)
        {
            result = mmp->loop;
            mmp->count = 0;
            mmp->loop = result + 1;
        }
    }
    return mmp->count;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupSpline(struct MotionManager *mmp);
 *     ACTION.C:344, 17 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct MotionManager * mmp
 * END PSX.SYM */

void SetupSpline(MotionManager *mmp)
{
    short time;
    short i;
    SplineControlType *spc;

    time = mmp->motion->time;
    spc = mmp->control;
    spc->key0 = mmp->motion->locate;
    spc->dd0.pad = time;
    if (time != 0)
    {
        spc->key1 = spc->key0 + 1;
        UpdateSplineControl(spc);
    }
    for (i = 0; i < mmp->n; i++)
    {
        spc = &mmp->control[i + 1];
        spc->key0 = mmp->motion->rotate[i];
        spc->dd0.pad = time;
        if (time != 0)
        {
            spc->key1 = spc->key0 + 1;
            UpdateSplineControl(spc);
        }
    }
}
