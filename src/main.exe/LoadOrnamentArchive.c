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
    ModelType *super;
    u32 tagMask;
    int parent;
    int count;

    if (adr == 0)
    {
        SystemOut(msg_no_model_archive_data_2);
    }
    mad = (OrnamentArchiveType *)valloc(sizeof(OrnamentArchiveType));
    mad->data = adr;
    adr++;
    mad->n = *(u16 *)adr;
    adr++;
    i = 0;
    tagMask = 0xA0000000;
    mad->object = (OrnamentType **)valloc(mad->n * 4);
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
    mad->object[idx] = (OrnamentType *)((u32)objp | tagMask);
}
    goto loop1;
loop1_end:

    if (prnt == 0)
    {
        prnt = &World;
    }
    GsInitCoordinate2((GsCOORDINATE2 *)prnt, (GsCOORDINATE2 *)mad);
    mad->locate.coord.t[0] = 0;
    mad->locate.coord.t[1] = 0;
    mad->locate.coord.t[2] = 0;
    mad->rotate.vx = 0;
    mad->rotate.vy = 0;
    mad->rotate.vz = 0;
    UpdateCoordinate((ModelType *)mad);
    i = 0;
    mad->id = -1;
    mad->attribute = 0;
loop2:
    if (!(i < (count = mad->n)))
        goto loop2_end;
    objp = mad->object[i];
    super = (ModelType *)mad;
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
    GsInitCoordinate2((GsCOORDINATE2 *)super, &objp->locate);
    objp->locate.coord.t[0] = prntp[i].dx;
    objp->locate.coord.t[1] = prntp[i].dy;
    objp->locate.coord.t[2] = prntp[i].dz;
    UpdateOrnament(objp, 0);
    i++;
    objp->object.attribute |= 0x400; /* libgs GsDOBJ2 bit 10 — consumed
                              inside the linked libgs sorter, set
                              on every loaded world object */
    goto loop2;

parent_found:
    super = (ModelType *)mad->object[j];
    goto coordinate_init;
loop2_end:

    mad->rotate.pad = (short)mad->object[0]->locate.coord.t[1];
    return mad;
}
