#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DisposeModelArchive(struct ModelArchiveType *mad);
 *     3DCTRL.C:425, 9 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct ModelArchiveType * mad
 * END PSX.SYM */

extern void vfree(void *p);

void DisposeModelArchive(ModelArchiveType *mad)
{
    s32 i;

    if (mad != 0)
    {
        for (i = 0; i < mad->n; i++)
        {
            if (mad->object[i] != 0)
            {
                vfree(mad->object[i]);
            }
        }
        vfree(mad->object);
        vfree(mad);
    }
}
