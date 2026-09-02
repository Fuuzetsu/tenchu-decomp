#include "common.h"
#include "main.exe.h"
#include "tmdfile.h"

/* Relocate a linked-TMD's vertex/normal/primitive offsets into absolute
 * pointers, once (bit 0 of the header marks it already mapped). */
void GsMapModelingData(unsigned long *model)
{
    TmdObjectRecord *object;
    int count;
    int i;

    if (*model & TMD_FLAG_MAPPED)
        return;

    *model |= TMD_FLAG_MAPPED;
    ++model;
    i = 0;
    count = *model++;
    if (count > 0)
    {
        do
        {
            object = (TmdObjectRecord *)(model + i * TMD_OBJECT_WORDS);
            object->sdk.vertop =
                (unsigned long *)((unsigned long)object->sdk.vertop +
                                  (unsigned long)model);
            object->sdk.nortop =
                (unsigned long *)((unsigned long)object->sdk.nortop +
                                  (unsigned long)model);
            object->sdk.primtop =
                (unsigned long *)((unsigned long)object->sdk.primtop +
                                  (unsigned long)model);
            ++i;
        } while (i < count);
    }
}
