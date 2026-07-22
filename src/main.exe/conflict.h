#ifndef CONFLICT_H
#define CONFLICT_H

/* CONFLICT.C's shared floor query, using the original promoted mode ABI. */
extern long GetAreaMapLevel(unsigned long *area, long x, long y, long z,
                            int mode);
extern short InsertConflict(ModelType *model);
extern void DeleteConflict(ModelType *model);
extern void ComputeAllConflict(void);

#endif
