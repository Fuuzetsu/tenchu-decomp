#ifndef TENCHU_ACTION_H
#define TENCHU_ACTION_H

typedef s16 motion_update_result;
enum motion_update_result_value
{
    MOTION_UPDATE_UNCHANGED = -1,
    MOTION_UPDATE_NOT_FOUND = 0,
    MOTION_UPDATE_CHANGED = 1
};

MotionPackType *LoadMotion(unsigned long *data);
MotionRegistType *SetupMotionRegist(MotionRegistType *registration);
MotionManager *SetupMotionManager(ModelArchiveType *model,
                                  MotionRegistType *registration);
void DisposeMotionManager(MotionManager *motion);
short GetMotionID(MotionManager *motion, motion_id id);
motion_update_result UpdateMotion(MotionManager *motion, motion_id id);
short PlayMotion(MotionManager *motion, short mode);
void eval_spline_gte_(SVECTOR *out, SplineControlType *control,
                      SVECTOR *basis);

#endif
