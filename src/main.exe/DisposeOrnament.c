#include "common.h"
#include "main.exe.h"

/* DisposeOrnament (0x80018718) — byte-identical clone of DisposeModel (tools/clonematch.py). */

extern void vfree(void *p);

void DisposeOrnament(void *objp)
{
    if (objp != 0)
    {
        vfree(objp);
    }
}
