#ifndef CONFLICT_H
#define CONFLICT_H

/* GetAreaMapLevel mode bits, recovered from the callee's independent tests. */
#define AREA_LEVEL_STEP_DOWN 0x01
#define AREA_LEVEL_RETURN_DELTA 0x02
#define AREA_LEVEL_ALLOW_DEEP 0x04
#define AREA_LEVEL_FIRST_HIT 0x08
#define AREA_LEVEL_REUSE_CACHED 0x10

/* CONFLICT.C's shared floor query, using the original promoted mode ABI. */
extern long GetAreaMapLevel(AreaMapType *area, long x, long y, long z,
                            int mode);
extern long GetAreaMapVector(AreaMapType *area, MapVector *mvp,
                             VECTOR *pos, long wide, int mode);
extern VECTOR *GetAreaMapPassage(AreaMapType *area, VECTOR *pos,
                                 SVECTOR *vect, short n);
extern short InsertConflict(ModelType *model);
extern void DeleteConflict(ModelType *model);
extern void ComputeAllConflict(void);
extern short GetConflictResult(ModelType *model, short index);

#endif
