#include "common.h"
#include "main.exe.h"
#include "images.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct GsIMAGE * GetImage(int index);
 *     IMAGES.C:52, 16 src lines, frame 40 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * The counted loop indexes the owning `Images` array directly. GCC
 * strength-reduces `&Images[i]` to the retail walking pointer; its `i = 0`
 * initializer after the optional error message also supplies the target's
 * shared delay-slot default.
 *
 * The final bounds check needed its condition WRITTEN INVERTED relative to
 * Ghidra's own `if (index < 0x3e) return Images+index; else {bad; return
 * Images;}` rendering: with Ghidra's literal polarity, cc1 put the no-call
 * `return Images+index;` arm as the branch TARGET and the AdtMessageBox arm
 * as the fallthrough (needing its own `j` to skip to the epilogue) — the
 * opposite of the target. Writing `if ((unsigned)index >= 0x3e) {bad; return Images;}
 * else return Images+index;` (condition inverted, bodies swapped to match)
 * reproduces the target's good-path-fallthrough/bad-path-branch-target
 * layout exactly.
 */
extern void AdtMessageBox(char *fmt, ...);
extern void vfree(void *p);
extern char path_image_images_arc[]; /* K:\\WORK\\CDIMAGE\\IMAGE\\images.arc */
extern char msg_bad_image_file[];    /* bad image file */
extern char msg_bad_image_index[];   /* bad image index */
/* Qualified because INFOVIEW.C has its own static fInitialize. */
extern u8 Images_fInitialize;
extern GsIMAGE Images[N_IMAGES];

GsIMAGE *GetImage(ImageArchiveId index)
{
    ArcFile *archive;
    u_long *adr;
    int i;

    if (Images_fInitialize == 0)
    {
        archive = (ArcFile *)FileRead(path_image_images_arc);
        if (archive->count < N_IMAGES)
        {
            AdtMessageBox(msg_bad_image_file);
        }
        for (i = 0; i < N_IMAGES; i++)
        {
            adr = get_tim_from_archive(archive, i);
            GetTIMInfo(adr, &Images[i]);
            LoadTIM(adr);
        }
        vfree(archive);
        Images_fInitialize = 1;
    }
    if ((unsigned)index >= N_IMAGES)
    {
        AdtMessageBox(msg_bad_image_index);
        return Images;
    }
    else
    {
        return Images + index;
    }
}
