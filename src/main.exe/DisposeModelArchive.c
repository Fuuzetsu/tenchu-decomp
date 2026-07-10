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
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct ModelArchiveType * mad
 * END PSX.SYM */

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
