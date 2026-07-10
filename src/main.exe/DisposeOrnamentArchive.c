#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * DisposeOrnamentArchive (0x8003cd5c) — free an ornament archive: dispose
 * each sub-ornament in the `object` table (DisposeOrnament handles its own
 * null check, so the loop calls it unconditionally — unlike
 * DisposeModelArchive's inline null-check-then-vfree), then the object
 * table, the `data` buffer, and the archive itself. Local truncated struct
 * (same convention as DisposeBG/DisposeAfterimage); `n`@0x5C/`object`@0x60/
 * `data`@0x64 cross-checked against Ghidra's own independently-built
 * struct (reference/ghidra_types.h:5342) and tools/access.py's offsets.
 */
typedef struct
{
    u8 pad0[0x5C];
    s16 n;                    /* 0x5C */
    OrnamentType **object;    /* 0x60 */
    void *data;                /* 0x64 */
} OrnamentArchiveType;

extern void DisposeOrnament(OrnamentType *objp);
extern void vfree(void *p);

void DisposeOrnamentArchive(OrnamentArchiveType *mad)
{
    s32 i;

    if (mad != 0) {
        for (i = 0; i < mad->n; i++) {
            DisposeOrnament(mad->object[i]);
        }
        vfree(mad->object);
        vfree(mad->data);
        vfree(mad);
    }
}
