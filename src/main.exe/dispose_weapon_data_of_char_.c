#include "common.h"
#include "main.exe.h"
#include "item.h"

extern Humanoid *Me_MOTION_C;
void dispose_weapon_data_of_char_(Humanoid *h, int mode)
{
    Me_MOTION_C = h;
    dtM = h->motion;
    AttackCancelControl(mode);
}
