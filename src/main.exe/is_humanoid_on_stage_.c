#include "common.h"
#include "main.exe.h"
#include "item.h"

s32 is_humanoid_on_stage_(Humanoid *human)
{
    s32 i;

    for (i = 0; i < Humans; i++)
    {
        if (HumanGroup[i] == human)
            break;
    }
    return i != Humans;
}
