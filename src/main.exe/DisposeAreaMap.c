#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DisposeAreaMap(unsigned long *area);
 *     CONFLICT.C:74, 4 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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
 *     param $a0       unsigned long * area
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

/*
 * DisposeAreaMap (0x8001ab28) — free an area-map object; if none was given,
 * fall back to the cached GlobalAreaMap (clearing the cache) instead. Unlike
 * Ghidra's flattened rendering (which loads GlobalAreaMap unconditionally at
 * entry), the asm only loads it inside the `area == 0` path — a nested if,
 * not a hoisted read (trust the asm's branch shape over Ghidra's SSA-style
 * early temp).
 */
extern void vfree(void *p);
extern void *GlobalAreaMap;

void DisposeAreaMap(void *area)
{
    if (area == 0) {
        void *tmp = GlobalAreaMap;
        if (tmp != 0) {
            area = tmp;
            GlobalAreaMap = 0;
        }
    }
    vfree(area);
}
