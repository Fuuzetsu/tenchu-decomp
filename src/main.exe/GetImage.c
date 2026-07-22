#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct GsIMAGE * GetImage(int index);
 *     IMAGES.C:52, 16 src lines, frame 40 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     param $a0       int index
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsIMAGE Images[52];
 * END PSX.SYM */

/*
 * GetImage (0x8004f27c, 0x100 bytes) — lazily loads the shared 62-entry
 * Images[] pool (same load sequence as InitializeImage.c: FileRead the
 * images.arc archive, sanity-check its entry count, then get_tim_from_archive
 * + GetTIMInfo + LoadTIM per slot, vfree the archive buffer) guarded by a
 * one-shot flag, then returns &Images[index] with its own bounds check.
 *
 * The `load_images_archive__override__prt_*` labels below are splat
 * mid-function symbol artifacts, NOT a jump table: piece 1 falls straight
 * through into piece 2 (no branch targets either boundary) and piece 2 into
 * piece 3 the same way — one ordinary function (cookbook "Split functions":
 * SetupStageSequence/FileOption precedent). Written as plain C, no _jtbl.
 *
 * The `i = 0;`/`image = Images;` pair after the whole `if (count < 0x3e)
 * AdtMessageBox(...)` sits in Ghidra's own rendering as ONE statement each,
 * positioned after the inner if with no restructuring — reorg duplicates
 * `i = 0;` into the guard branch's delay slot (runs unconditionally on both
 * arms) AND leaves the original copy after the AdtMessageBox call for the
 * fallthrough arm (harmless double-execution), exactly the InitializeImage.c
 * "shared default via delay slot" mechanism.
 *
 * The final bounds check needed its condition WRITTEN INVERTED relative to
 * Ghidra's own `if (index < 0x3e) return Images+index; else {bad; return
 * Images;}` rendering: with Ghidra's literal polarity, cc1 put the no-call
 * `return Images+index;` arm as the branch TARGET and the AdtMessageBox arm
 * as the fallthrough (needing its own `j` to skip to the epilogue) — the
 * opposite of the target. Writing `if (index >= 0x3e) {bad; return Images;}
 * else return Images+index;` (condition inverted, bodies swapped to match)
 * reproduces the target's good-path-fallthrough/bad-path-branch-target
 * layout exactly.
 */
extern void AdtMessageBox(char *fmt, ...);
extern void vfree(void *p);
extern char D_8001287C[]; /* "K:\\WORK\\CDIMAGE\\IMAGE\\images.arc" */
extern char D_800128A0[]; /* "bad image file" */
extern char D_800128B0[]; /* "bad image index" */
extern char D_80097C90;   /* one-shot "images loaded" flag */
extern GsIMAGE Images[62];

GsIMAGE *GetImage(int index)
{
    u_long *pt;
    u_long *adr;
    int i;
    GsIMAGE *image;

    if (D_80097C90 == 0) {
        pt = FileRead(D_8001287C);
        if ((short)*pt < 0x3e) {
            AdtMessageBox(D_800128A0);
        }
        i = 0;
        image = Images;
        do {
            adr = get_tim_from_archive(pt, i);
            GetTIMInfo(adr, image);
            LoadTIM(adr);
            i = i + 1;
            image = image + 1;
        } while (i < 0x3e);
        vfree(pt);
        D_80097C90 = 1;
    }
    if ((unsigned)index >= 0x3e) {
        AdtMessageBox(D_800128B0);
        return Images;
    } else {
        return Images + index;
    }
}
