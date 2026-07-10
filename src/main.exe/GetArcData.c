#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long * GetArcData(int index);
 *     IMAGES.C:158, 25 src lines, frame 24 bytes, saved-reg mask 0x80010000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $a0       int index
 * END PSX.SYM */

/*
 * GetArcData (0x8004f37c, 0xd0 bytes) — lazily loads "models.arc" via
 * FileRead into the gp-relative global MODEL_ARCHIVE_PTR (defined in this
 * TU, hence gp-addressed — see the cookbook's gp section), one-time-converts
 * its table of `count` relative offsets (each stored where it will end up
 * holding an absolute pointer — same slot reused for both, like
 * ProcItemDrop's shared-constant idiom but here for a whole table) into
 * absolute pointers via `entry[i] += (char *)arc + 4` (the "+4" skips the
 * {count,loaded} header word), marks `loaded` so the conversion only runs
 * once, then returns `entry[index]` after validating `index` against
 * `count`.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - The conversion must go through a temp: `t = entry[i] + 4; entry[i] =
 *    (s32)arc + t;`. Writing it as one expression (either operand order)
 *    lets fold-const's `associate` combine the invariant `arc + 4` into one
 *    loop-hoisted register, an extra callee-saved reg the target doesn't
 *    have — splitting the statement keeps `arc` and the per-iteration `+4`
 *    in separate sub-expressions so nothing invariant-with-a-constant is
 *    left for loop.c to hoist.
 *  - `arc->count > zero` (a fresh `s32 zero = 0;` local) instead of the
 *    equivalent `> 0` literal fixes a pure a0/a1 register SWAP between `arc`
 *    and the loop counter `i` (12 bytes, permuter-found, no length/
 *    instruction change) — adding one more pseudo shifts global-alloc's
 *    pseudo-number tie-break enough to flip which of the two competing
 *    variables lands in which hard reg. Cheaper than a full permuter run
 *    once you suspect this class of tie: try naming a literal operand
 *    through a same-valued local first.
 */

typedef struct
{
    s16 count;    /* 0x0 */
    s16 loaded;   /* 0x2 (0 until the offset->pointer conversion has run) */
    s32 entry[1]; /* 0x4 (relative offset before conversion, absolute
                     pointer after — same 4 bytes hold both) */
} ArcFile;

extern ArcFile *MODEL_ARCHIVE_PTR;
extern u_long *FileRead(char *path);
extern void AdtMessageBox(char *fmt, ...);
extern char D_800128D8[]; /* "K:\WORK\CDIMAGE\IMAGE\models.arc" */
extern char D_800128C0[]; /* "bad archive index %d" */

void *GetArcData(s32 index)
{
    s32 i;
    ArcFile *arc;
    s32 t;
    s32 zero = 0;

    if (MODEL_ARCHIVE_PTR == 0)
    {
        MODEL_ARCHIVE_PTR = (ArcFile *) FileRead(D_800128D8);
    }
    arc = MODEL_ARCHIVE_PTR;
    if (arc->loaded == 0)
    {
        i = 0;
        if (arc->count > zero)
        {
            do
            {
                t = arc->entry[i] + 4;
                arc->entry[i] = (s32) arc + t;
                i++;
            } while (i < arc->count);
        }
        arc->loaded = 1;
    }
    if (index < 0 || arc->count <= index)
    {
        AdtMessageBox(D_800128C0, index);
        return 0;
    }
    return (void *) arc->entry[index];
}
