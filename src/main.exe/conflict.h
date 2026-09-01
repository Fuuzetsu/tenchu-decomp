#ifndef CONFLICT_H
#define CONFLICT_H

/* GetAreaMapLevel mode bits, recovered from the callee's independent tests. */
enum area_level_mode_flag
{
    AREA_LEVEL_DEFAULT = 0,
    AREA_LEVEL_STEP_DOWN = 0x01,
    AREA_LEVEL_RETURN_DELTA = 0x02,
    AREA_LEVEL_ALLOW_DEEP = 0x04,
    AREA_LEVEL_FIRST_HIT = 0x08,
    AREA_LEVEL_REUSE_CACHED = 0x10
};

/* ConflictObject slot/result sentinel. ModelType.id stores this value while
 * the model is not registered in the collision pool. */
#define CONFLICT_NONE (-1)

/* CONFLICT.C's shared floor query, using the original promoted mode ABI. */
extern long GetAreaMapLevel(AreaMapType *area, long x, long y, long z,
                            int mode);
extern long GetAreaMapVector(AreaMapType *area, MapVector *mvp,
                             VECTOR *pos, long wide, int mode);
extern VECTOR *GetAreaMapPassage(AreaMapType *area, VECTOR *pos,
                                 SVECTOR *vect, short n);
extern conflict_id InsertConflict(ModelType *model);
extern void DeleteConflict(ModelType *model);
extern void ComputeAllConflict(void);
extern conflict_id GetConflictResult(ModelType *model, conflict_id index);

#endif
