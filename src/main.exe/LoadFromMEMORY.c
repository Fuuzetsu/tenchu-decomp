#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long * LoadFromMEMORY(unsigned char *filename);
 *     FILEIO.C:247, 45 src lines, frame 64 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     param $s4       unsigned char * filename
 *     reg   $s1       unsigned long * vmp
 *     reg   $s2       unsigned long * data
 *     stack sp+16     unsigned char [24] name
 *     reg   $s0       short i
 *     reg   $a2       short j
 *     reg   $s4       unsigned char * filename
 *     reg   $s2       int fd
 *     reg   $s3       int size
 *     reg   $s1       unsigned long * data
 * END PSX.SYM */

/*
 * LoadFromMEMORY (0x8001962c) — memory-card/MEMORY loading stub; disabled in
 * the shipped build, so it just reports the "disabled" message and returns
 * null (filename is unused). D_8001113C is the literal string
 * "*** memory load is disabled now ***" living in this TU's rodata blob
 * (unsplit data segment; the address already has an auto symbol, no yaml
 * carve needed — same pattern as D_800121CC/D_80097CB4 elsewhere).
 */
extern void AdtMessageBox(char *fmt, ...);
extern char D_8001113C[];

void *LoadFromMEMORY(u8 *filename)
{
    AdtMessageBox(D_8001113C);
    return 0;
}
