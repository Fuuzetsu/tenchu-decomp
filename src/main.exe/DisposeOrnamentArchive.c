#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DisposeOrnamentArchive(struct OrnamentArchiveType *mad);
 *     WORLD.C:319, 11 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct OrnamentArchiveType * mad
 * END PSX.SYM */

extern void DisposeOrnament(OrnamentType *objp);
extern void vfree(void *p);

void DisposeOrnamentArchive(OrnamentArchiveType *mad)
{
    s32 i;

    if (mad != 0)
    {
        for (i = 0; i < mad->n; i++)
        {
            DisposeOrnament(mad->object[i]);
        }
        vfree(mad->object);
        vfree(mad->data);
        vfree(mad);
    }
}
