#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * DisposeModelArchive (0x800185bc) — free a model archive: dispose each
 * non-null sub-model in the `object` table (ModelArchiveType.n/object,
 * item.h), then the table itself, then the archive. Same null-check-then-free
 * shape as DisposeBG/DisposeAfterimage, plus a loop over the sub-object
 * table.
 */
extern void vfree(void *p);

void DisposeModelArchive(ModelArchiveType *mad)
{
    s32 i;

    if (mad != 0) {
        for (i = 0; i < mad->n; i++) {
            if (mad->object[i] != 0) {
                vfree(mad->object[i]);
            }
        }
        vfree(mad->object);
        vfree(mad);
    }
}
