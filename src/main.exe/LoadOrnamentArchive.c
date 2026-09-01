#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct OrnamentArchiveType * LoadOrnamentArchive(unsigned long *adr, struct ModelType *prnt);
 *     WORLD.C:259, 57 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       unsigned long * adr
 *     param $s4       struct ModelType * prnt
 *     reg   $a3       struct ModelType * dim
 *     reg   $s1       struct OrnamentArchiveType * mad
 *     reg   $s3       struct ParentingType * prntp
 *     reg   $s5       unsigned char * tmdp
 *     reg   $s2       short i
 *     reg   $v1       short j
 *     reg   $s0       struct OrnamentType * objp
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType World;
 * END PSX.SYM */

/*
 * STATUS: MATCHING — pure C, all 568 bytes / 142 instructions exact.
 * Reusing the PSX.SYM `i` for both archive loops and `j` for the nested
 * parent search produces the target loop and found-path layout. The offset
 * consumer's unsigned prntp cancellation supplies the two allocation reads
 * that make `prntp` outrank `prnt`, giving the target s3/s4 assignment with
 * no one-shot wrappers. Assigning `count` in loop2's comparison and
 * initializing `j` before the parent copy preserve the two target
 * instruction-order pairs.
 */

extern void *valloc(u32 size);
extern void UpdateOrnament(OrnamentType *objp, short ry);
extern OrnamentType *LoadOrnament(u_long *adr);
extern char msg_no_model_archive_data_2[]; /* NO MODEL ARCHIVE DATA */

OrnamentArchiveType *LoadOrnamentArchive(u_long *adr, ModelType *prnt)
{
    OrnamentArchiveType *mad;
    ParentingType *prntp;
    u8 *tmdp;
    short i;
    short j;
    OrnamentType *objp;
    GsCOORDINATE2 *super;
    u32 uncachedSegment;
    int parent;
    int count;

    if (adr == 0)
    {
        SystemOut(msg_no_model_archive_data_2);
    }
    mad = (OrnamentArchiveType *)valloc(sizeof(OrnamentArchiveType));
    mad->data = adr;
    adr = MODEL_ARCHIVE_CURSOR_ADVANCE(adr, signature, count);
    mad->n = MODEL_ARCHIVE_UNSIGNED_COUNT(adr);
    adr = MODEL_ARCHIVE_CURSOR_ADVANCE(adr, count, parenting);
    i = 0;
    uncachedSegment = PSX_KSEG1_BASE;
    mad->object =
        (OrnamentType **)valloc(mad->n * sizeof(OrnamentType *));
    prntp = (ParentingType *)adr;
    tmdp = (u8 *)adr;

loop1:
{
    int idx = i;
    s32 offset;
    if (idx >= mad->n)
        goto loop1_end;
    /* Allocation carrier for both former offset-load wrappers. */
    /* allocation staging: folded after flow -- not recovered arithmetic */
    offset = prntp[idx].index + (u32)prntp - (u32)prntp;
    i++;
    objp = LoadOrnament((u_long *)(tmdp + offset));
    mad->object[idx] = (OrnamentType *)((u32)objp | uncachedSegment);
}
    goto loop1;
loop1_end:

    if (prnt == 0)
    {
        prnt = &World;
    }
    GsInitCoordinate2(&prnt->locate, &mad->locate);
    mad->locate.coord.t[0] = 0;
    mad->locate.coord.t[1] = 0;
    mad->locate.coord.t[2] = 0;
    mad->rotate.vx = 0;
    mad->rotate.vy = 0;
    mad->rotate.vz = 0;
    UpdateCoordinate((ModelType *)mad);
    i = 0;
    mad->id = CONFLICT_NONE;
    mad->attribute = 0;
loop2:
    if (!(i < (count = mad->n)))
        goto loop2_end;
    objp = mad->object[i];
    super = &mad->locate;
    if (prntp[i].np >= 0 && count > 0)
    {
        j = 0;
        parent = prntp[i].np;
    parent_loop:
        if (parent == prntp[j].nc)
            goto parent_found;
        j++;
        if (j < count)
            goto parent_loop;
    }
coordinate_init:
    GsInitCoordinate2(super, &objp->locate);
    objp->locate.coord.t[0] = prntp[i].dx;
    objp->locate.coord.t[1] = prntp[i].dy;
    objp->locate.coord.t[2] = prntp[i].dz;
    UpdateOrnament(objp, 0);
    i++;
    objp->object.attribute |= GS_DOBJ_DIVISION_DEPTH_BITS(2);
    goto loop2;

parent_found:
    super = &mad->object[j]->locate;
    goto coordinate_init;
loop2_end:

    mad->rotate.pad = (short)mad->object[0]->locate.coord.t[1];
    return mad;
}
