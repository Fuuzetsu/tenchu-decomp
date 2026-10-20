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
