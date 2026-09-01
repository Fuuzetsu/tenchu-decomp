#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct ModelArchiveType * LoadModelArchive(unsigned long *adr, struct ModelType *prnt);
 *     3DCTRL.C:335, 54 src lines, frame 56 bytes, saved-reg mask 0x80ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       unsigned long * adr
 *     param $s6       struct ModelType * prnt
 *     reg   $a3       struct ModelType * dim
 *     reg   $s2       struct ModelArchiveType * mad
 *     reg   $s5       struct ParentingType * prntp
 *     reg   $s7       unsigned char * tmdp
 *     reg   $s3       short i
 *     reg   $v1       short j
 *     reg   $s0       struct ModelType * objp
 *     reg   $s0       struct ModelType * dim
 *     reg   $s2       struct ModelType * dim
 *     reg   $s0       struct ModelType * objp
 *     reg   $s0       struct ModelType * dim
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType World;
 * END PSX.SYM */

#include "item.h"
#include "tmdfile.h"

/*
 * STATUS: MATCHING — pure C, all 760 bytes / 190 instructions exact.
 *
 * The second pass follows LoadOrnamentArchive's proven parent-search shape.
 * Giving its parent pointer a separate `super` identity lets it die in a3 at
 * GsInitCoordinate2 while `objp` remains in s0. The `mad->n`
 * memory view and signed `mad->n` field have the same verified 0x64 address
 * but distinct C identities, preserving the target's adjacent lhu/lh loads
 * instead of cc1 folding them together. `limit` then keeps j in v1 through
 * the inner loop.
 */
extern void *valloc(u32 size);
extern char msg_no_model_archive_data[]; /* NO MODEL ARCHIVE DATA */

ModelArchiveType *LoadModelArchive(u_long *adr, ModelType *prnt)
{
    ModelArchiveType *mad;
    ParentingType *prntp;
    u8 *tmdp;
    short i;
    short j;
    short limit;
    u16 count;
    ModelType *dim;
    ModelType *objp;
    GsCOORDINATE2 *super;
    TMDFile *dtmd;
    int parent;

    if (adr == 0)
    {
        SystemOut(msg_no_model_archive_data);
    }
    mad = (ModelArchiveType *)valloc(sizeof(ModelArchiveType));
    adr = MODEL_ARCHIVE_CURSOR_ADVANCE(adr, signature, count);
    mad->n = MODEL_ARCHIVE_SIGNED_COUNT(adr);
    adr = MODEL_ARCHIVE_CURSOR_ADVANCE(adr, count, parenting);
    i = 0;
    mad->object = (ModelType **)valloc(mad->n * sizeof(ModelType *));
    prntp = (ParentingType *)adr;
    tmdp = (u8 *)prntp;
    if (mad->n > 0)
    {
        do
        {
            dtmd = (TMDFile *)(tmdp + prntp[i].index);
            dim = (ModelType *)valloc(sizeof(ModelType));
            if (dtmd != 0)
            {
                GsMapModelingData((u_long *)&dtmd->data);
                GsLinkObject4((u_long)dtmd->data.objects, &dim->object, 0);
            }
            dim->object.coord2 = &dim->locate;
            dim->object.attribute = 0;
            GsInitCoordinate2(&World.locate, &dim->locate);
            dim->locate.coord.t[0] = 0;
            dim->locate.coord.t[1] = 0;
            dim->locate.coord.t[2] = 0;
            dim->rotate.vx = 0;
            dim->rotate.vy = 0;
            dim->rotate.vz = 0;
            dim->clip.vx = 0;
            dim->clip.vy = 0;
            dim->clip.vz = 0;
            RotMatrixYXZ(&dim->rotate, &dim->locate.coord);
            dim->id = CONFLICT_NONE;
            dim->locate.flg = 0;
            dim->attribute = 0;
            mad->object[i] = dim;
            i++;
        } while (i < mad->n);
    }
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
    mad->clip.vx = 0;
    mad->clip.vy = 0;
    mad->clip.vz = 0;
    RotMatrixYXZ(&mad->rotate, &mad->locate.coord);
    i = 0;
    mad->locate.flg = 0;
    mad->id = CONFLICT_NONE;
    mad->attribute = 0;
    count = mad->n;
    if (mad->n > 0)
    {
        do
        {
            objp = mad->object[i];
            super = &mad->locate;
            if (prntp[i].np >= 0 && (j = 0, (s16)count > 0))
            {
                parent = prntp[i].np;
                limit = mad->n;
                do
                {
                    if (parent == prntp[j].nc)
                    {
                        super = &mad->object[j]->locate;
                        goto coordinate_init;
                    }
                    j++;
                } while (j < limit);
            }
        coordinate_init:
            GsInitCoordinate2(super, &objp->locate);
            objp->locate.coord.t[0] = prntp[i].dx;
            objp->locate.coord.t[1] = prntp[i].dy;
            objp->locate.coord.t[2] = prntp[i].dz;
            RotMatrixYXZ(&objp->rotate, &objp->locate.coord);
            i++;
            objp->locate.flg = 0;
            count = mad->n;
        } while (i < mad->n);
    }
    mad->rotate.pad = (short)mad->object[0]->locate.coord.t[1];
    return mad;
}
