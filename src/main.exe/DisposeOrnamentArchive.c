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
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct OrnamentArchiveType * mad
 * END PSX.SYM */

/*
 * DisposeOrnamentArchive (0x8003cd5c) — free an ornament archive: dispose
 * each sub-ornament in the `object` table (DisposeOrnament handles its own
 * null check, so the loop calls it unconditionally — unlike
 * DisposeModelArchive's inline null-check-then-vfree), then the object
 * table, the `data` buffer, and the archive itself. The shared PSX.SYM
 * record supplies `n`@0x5C, `object`@0x60, and `data`@0x64.
 */
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
