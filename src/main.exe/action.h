#ifndef TENCHU_ACTION_H
#define TENCHU_ACTION_H

MotionPackType *LoadMotion(unsigned long *data);
MotionRegistType *SetupMotionRegist(MotionRegistType *registration);
MotionManager *SetupMotionManager(ModelArchiveType *model,
                                  MotionRegistType *registration);
void DisposeMotionManager(MotionManager *motion);
short GetMotionID(MotionManager *motion, motion_id id);
s16 UpdateMotion(MotionManager *motion, motion_id id);
short PlayMotion(MotionManager *motion, short mode);

#endif
