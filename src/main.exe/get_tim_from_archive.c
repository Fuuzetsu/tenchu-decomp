#include "common.h"
#include "main.exe.h"

/*
 * get_tim_from_archive (0x8004f1d8, 0xa4 bytes) - generic archive-entry
 * accessor, near-twin of GetArcData.c: one-time-converts an ArcFile's
 * table of relative offsets into absolute pointers (same conversion
 * idiom - see GetArcData.c's matching notes for the temp-through-`t`
 * requirement), then validates and returns `entry[idx]`. Unlike
 * GetArcData this takes the archive pointer directly as a PARAMETER (no
 * lazy FileRead/gp singleton caching), so `archive`/`idx` stay in
 * $a0/$a1 throughout with no register promotion.
 *
 * NOTE on the split: splat reports this as 2 pieces (a `__override__prt_`
 * glabel at 0x8004f254), but it is NOT a jump table - the raw .s has no
 * branch targeting 0x8004f254 itself; piece 1 falls straight through into
 * piece 2's first instruction (the `lui`/`jal`/`addiu` address-materialize
 * spans the piece boundary), and the one forward branch out of piece 1
 * lands mid-piece-2 at an ordinary internal label (.L8004F264). Per the
 * cookbook's "not every 2-piece report is a real jump table" rule, this
 * is written as ONE plain C function, no _jtbl array.
 */
typedef struct
{
    s16 count;    /* 0x0 */
    s16 loaded;   /* 0x2 (0 until the offset->pointer conversion has run) */
    s32 entry[1]; /* 0x4 (relative offset before conversion, absolute
                     pointer after - same 4 bytes hold both) */
} ArcFile;

extern void AdtMessageBox(char *fmt, ...);
extern char D_800128C0[]; /* "bad archive index %d" */

u_long *get_tim_from_archive(u_long *archive, int idx)
{
    ArcFile *arc;
    s32 i;
    s32 t;

    arc = (ArcFile *)archive;
    if (arc->loaded == 0)
    {
        i = 0;
        if (arc->count > 0)
        {
            do
            {
                t = arc->entry[i] + 4;
                arc->entry[i] = (s32)arc + t;
                i++;
            } while (i < arc->count);
        }
        arc->loaded = 1;
    }
    if (idx < 0 || arc->count <= idx)
    {
        AdtMessageBox(D_800128C0, idx);
        return 0;
    }
    return (u_long *)arc->entry[idx];
}
