#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct ModelArchiveType * LoadModelArchive(unsigned long *adr, struct ModelType *prnt);
 *     3DCTRL.C:335, 54 src lines, frame 56 bytes, saved-reg mask 0x80ff0000 (DEMO build -- see below)
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

/*
 * LoadModelArchive (0x80017394) — 3DCTRL.C's loader: turns a packed
 * multi-part TMD archive into a live ModelArchiveType hierarchy of
 * GsCOORDINATE2-linked ModelTypes. Returns the new archive. Complains
 * through SystemOut(msg_no_model_archive_data) on a NULL address but
 * carries on regardless. vallocs the archive header, reads the part
 * count from the archive's second word into mad->n, vallocs the
 * ModelType* table, and treats the words after the count as a
 * ParentingType array. First pass, once per part: valloc a ModelType,
 * and unless the entry's index is zero hand the TMD at tmdp + index to
 * GsMapModelingData and GsLinkObject4. Each part is then reset to
 * identity — coord2 pointed back at itself, attribute 0,
 * GsInitCoordinate2 under World, zeroed translation/rotate/clip,
 * RotMatrixYXZ, id -1, flg 0 — and stored in mad->object[i]. The
 * archive node itself is parented to prnt (World when prnt is NULL)
 * and reset the same way. Second pass re-parents: a non-negative
 * prntp[i].np is searched against every prntp[j].nc, and the matching
 * object becomes that part's coordinate parent; parts with no match
 * stay attached to the archive node. The entry's dx/dy/dz become the
 * part's local translation and RotMatrixYXZ rebuilds its matrix.
 * Finally object[0]'s Y translation is stashed in mad->rotate.pad.
 */

#include "item.h"

/*
 * STATUS: MATCHING — pure C, all 760 bytes / 190 instructions exact.
 *
 * The second pass follows LoadOrnamentArchive's proven parent-search shape.
 * Giving its parent pointer a separate `super` identity lets it die in a3 at
 * GsInitCoordinate2 while `objp` remains in s0. The `*(u16 *)&mad->n`
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
    ModelType *super;
    int dtmd;
    int parent;

    if (adr == 0)
    {
        SystemOut(msg_no_model_archive_data);
    }
    mad = (ModelArchiveType *)valloc(sizeof(ModelArchiveType));
    adr++;
    mad->n = *(short *)adr;
    adr++;
    i = 0;
    mad->object = (ModelType **)valloc(mad->n * sizeof(ModelType *));
    prntp = (ParentingType *)adr;
    tmdp = (u8 *)prntp;
    if (mad->n > 0)
    {
        do
        {
            dtmd = (int)tmdp + prntp[i].index;
            dim = (ModelType *)valloc(sizeof(ModelType));
            if (dtmd != 0)
            {
                GsMapModelingData((u_long *)(dtmd + 4));
                GsLinkObject4(dtmd + 0xc, &dim->object, 0);
            }
            dim->object.coord2 = (GsCOORDINATE2 *)dim;
            dim->object.attribute = 0;
            GsInitCoordinate2(&World.locate, (GsCOORDINATE2 *)dim);
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
            dim->id = -1;
            dim->locate.flg = 0;
            dim->attribute = 0;
            mad->object[i] = dim;
            i++;
        } while (i < mad->n);
    }
    if (prnt == 0)
    {
        prnt = (ModelType *)&World;
    }
    GsInitCoordinate2(&prnt->locate, (GsCOORDINATE2 *)mad);
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
    mad->id = -1;
    mad->attribute = 0;
    count = *(u16 *)&mad->n;
    if (mad->n > 0)
    {
        do
        {
            objp = mad->object[i];
            super = (ModelType *)mad;
            if (prntp[i].np >= 0 && (j = 0, 0 < (count << 16)))
            {
                parent = prntp[i].np;
                limit = mad->n;
                do
                {
                    if (parent == prntp[j].nc)
                    {
                        super = mad->object[j];
                        goto coordinate_init;
                    }
                    j++;
                } while (j < limit);
            }
        coordinate_init:
            GsInitCoordinate2(&super->locate, &objp->locate);
            objp->locate.coord.t[0] = prntp[i].dx;
            objp->locate.coord.t[1] = prntp[i].dy;
            objp->locate.coord.t[2] = prntp[i].dz;
            RotMatrixYXZ(&objp->rotate, &objp->locate.coord);
            i++;
            objp->locate.flg = 0;
            count = *(u16 *)&mad->n;
        } while (i < mad->n);
    }
    mad->rotate.pad = (short)mad->object[0]->locate.coord.t[1];
    return mad;
}
