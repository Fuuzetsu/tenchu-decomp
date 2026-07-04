#include "common.h"
#include "main.exe.h"

/* DisposeModel (0x800184f8) — free a model object if non-null. Byte-identical
 * to DisposeOrnament (cloned via tools/clonematch.py). */

extern void vfree(void *p);

void DisposeModel(void *objp)
{
    if (objp != 0)
    {
        vfree(objp);
    }
}
