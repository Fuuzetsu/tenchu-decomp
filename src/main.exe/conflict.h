#ifndef CONFLICT_H
#define CONFLICT_H

/* CONFLICT.C's shared floor query, using the original promoted mode ABI. */
extern long GetAreaMapLevel(unsigned long *area, long x, long y, long z,
                            int mode);
extern long GetAreaMapVector(unsigned long *area, MapVector *mvp,
                             VECTOR *pos, long wide, int mode);
extern VECTOR *GetAreaMapPassage(unsigned long *area, VECTOR *pos,
                                 SVECTOR *vect, short n);
extern short InsertConflict(ModelType *model);
extern void DeleteConflict(ModelType *model);
extern void ComputeAllConflict(void);
extern short GetConflictResult(ModelType *model, short index);

#endif
