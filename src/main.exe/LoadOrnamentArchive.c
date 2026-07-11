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
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 * STATUS: NON_MATCHING — right length (568 bytes / 142 insns); 260 of 568
 * bytes differ, concentrated in the "parent lookup" sub-loop inside loop2
 * (searching prntp[] for the entry whose `id` matches the current entry's
 * `parent`, to inherit that already-created ornament's coordinate as the
 * new one's super instead of the archive root `dim`). Structural work
 * confirmed correct: both loops' shape (loop1 top-tested goto-loop over the
 * tag/pointer table exactly like ClearItemLayout's family; loop2's nested
 * do-while search with `k`/`next` matching the target's "increment before
 * compare" order); every struct offset (OrnamentArchiveType, OrnamentType,
 * ParentingType — the latter's own name straight from PSX.SYM) verified
 * against the raw .s field-by-field; the `tagMask = 0xA0000000` and other
 * persistent-register promotions needed to reach the right frame/length.
 *
 * `autorules` found idx (loop1's captured pre-increment index) needs `s8`,
 * not `short`, to reach this length/residual (1004 -> 260 differing bytes) —
 * kept despite the semantic narrowing risk (an archive with >127 sub-models
 * would wrap) because every `short` variant tried assembles 4 bytes LONG
 * (572 vs target 568) with a much larger residual; this is the best
 * available draft, not a claim that `s8` is the true original type.
 *
 * Residual shape: the target reads `dim->object[j]` in ONE instruction
 * (`lw v0,96(s1)`) before the parent-search condition; this draft's
 * compiled order computes the search address first, and the whole
 * "found" path (id-compare, `dim->object[k]`) ends up on different
 * registers (s1/s2 swapped roles) — a downstream cascade from that one
 * reordering. Several source-order variants of the `objp =` / `super =`
 * pair and the `idx` capture were tried with no further improvement. One
 * bounded permuter run (~470 iterations, `-j4`) found no score-0 candidate
 * (best partial 2730 vs base 3610, increasingly erroring past ~iteration
 * 400) — not adopted.
 */

struct OrnamentType
{
    GsCOORDINATE2 locate; /* 0x00 */
    GsDOBJ2 object;       /* 0x50 */
};

typedef struct
{
    GsCOORDINATE2 locate;    /* 0x00 */
    SVECTOR rotate;          /* 0x50 */
    s16 id;                  /* 0x58 */
    s16 attribute;           /* 0x5A */
    s16 n;                   /* 0x5C */
    OrnamentType **object;   /* 0x60 */
    u32 *data;                /* 0x64 */
} OrnamentArchiveType;        /* size 0x68 */

typedef struct
{
    GsCOORDINATE2 locate; /* 0x00 */
} WorldType;

typedef struct
{
    s16 parent;  /* 0x0 */
    s16 id;      /* 0x2 */
    s16 x;       /* 0x4 */
    s16 y;       /* 0x6 */
    s16 z;       /* 0x8 */
    s32 offset;  /* 0xC */
} ParentingType;  /* size 0x10 */

extern void *valloc(u32 size);
extern void GsInitCoordinate2(GsCOORDINATE2 *super, GsCOORDINATE2 *base);
extern void UpdateOrnament(OrnamentType *objp, short ry);
extern OrnamentType *LoadOrnament(u32 *adr);
extern void SystemOut(char *msg);
extern char D_800120AC[]; /* "NO MODEL ARCHIVE DATA" */
extern WorldType World;

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/LoadOrnamentArchive", LoadOrnamentArchive);
#else
OrnamentArchiveType *LoadOrnamentArchive(u32 *adr, ModelType *prnt)
{
    OrnamentArchiveType *dim;
    ParentingType *prntp;
    u8 *tmdp;
    short i;
    short j;
    OrnamentType *objp;
    ModelType *super;
    u32 tagMask;

    if (adr == 0) {
        SystemOut(D_800120AC);
    }
    dim = (OrnamentArchiveType *)valloc(sizeof(OrnamentArchiveType));
    dim->data = adr;
    dim->n = *(u16 *)(adr + 1);
    i = 0;
    tagMask = 0xA0000000;
    dim->object = (OrnamentType **)valloc(dim->n * 4);
    prntp = (ParentingType *)((u8 *)adr + 8);
    tmdp = (u8 *)adr + 8;

loop1:
    {
        s8 idx = i;
        if (!(idx < dim->n))
            goto loop1_end;
        i = idx + 1;
        objp = LoadOrnament((u32 *)(tmdp + prntp[idx].offset));
        dim->object[idx] = (OrnamentType *)((u32)objp | tagMask);
    }
    goto loop1;
loop1_end:

    if (prnt == 0) {
        prnt = (ModelType *)&World;
    }
    GsInitCoordinate2((GsCOORDINATE2 *)prnt, (GsCOORDINATE2 *)dim);
    dim->locate.coord.t[0] = 0;
    dim->locate.coord.t[1] = 0;
    dim->locate.coord.t[2] = 0;
    dim->rotate.vx = 0;
    dim->rotate.vy = 0;
    dim->rotate.vz = 0;
    UpdateCoordinate((ModelType *)dim);
    j = 0;
    dim->id = -1;
    dim->attribute = 0;
loop2:
    if (!(j < dim->n))
        goto loop2_end;
    objp = dim->object[j];
    super = (ModelType *)dim;
    if (0 <= prntp[j].parent && 0 < dim->n) {
        short k;
        short next;

        k = 0;
        do {
            next = k + 1;
            if (prntp[j].parent == prntp[k].id) {
                super = (ModelType *)dim->object[k];
                break;
            }
            k = next;
        } while (k < dim->n);
    }
    GsInitCoordinate2((GsCOORDINATE2 *)super, &objp->locate);
    objp->locate.coord.t[0] = prntp[j].x;
    objp->locate.coord.t[1] = prntp[j].y;
    objp->locate.coord.t[2] = prntp[j].z;
    UpdateOrnament(objp, 0);
    j = j + 1;
    objp->object.attribute |= 0x400;
    goto loop2;
loop2_end:

    dim->rotate.pad = (short)dim->object[0]->locate.coord.t[1];
    return dim;
}
#endif

// triage: MEDIUM — 142 insns, 1 loop, 6 callees, ~0.07 to SetupMotionRegist
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// OrnamentArchiveType * LoadOrnamentArchive(ulong *adr,ModelType *prnt)
//
// {
//   ulong uVar1;
//   ModelType *dim;
//   undefined *puVar2;
//   OrnamentType *pOVar3;
//   int iVar4;
//   int iVar5;
//   ModelType *super;
//   int iVar6;
//   int iVar7;
//   short sVar8;
//   ushort uVar9;
//
//   if (adr == (ulong *)0x0) {
//                     /* WARNING: Subroutine does not return */
//     SystemOut((uchar *)"NO MODEL ARCHIVE DATA");
//   }
//   dim = (ModelType *)valloc(0x68);
//   (dim->object).attribute = (ulong)adr;
//   uVar1 = adr[1];
//   sVar8 = 0;
//   (dim->clip).vx = (ushort)uVar1;
//   puVar2 = valloc((int)((uint)(ushort)uVar1 << 0x10) >> 0xe);
//   *(undefined **)&(dim->clip).vz = puVar2;
//   while( true ) {
//     iVar7 = (int)sVar8;
//     if ((dim->clip).vx <= iVar7) break;
//     sVar8 = sVar8 + 1;
//     pOVar3 = LoadOrnament((ulong *)((int)adr + adr[iVar7 * 4 + 5] + 8));
//     *(uint *)(iVar7 * 4 + *(int *)&(dim->clip).vz) = (uint)pOVar3 | 0xa0000000;
//   }
//   if (prnt == (ModelType *)0x0) {
//     prnt = &World;
//   }
//   GsInitCoordinate2(&prnt->locate,(GsCOORDINATE2 *)dim);
//   (dim->locate).coord.t[0] = 0;
//   (dim->locate).coord.t[1] = 0;
//   (dim->locate).coord.t[2] = 0;
//   (dim->rotate).vx = 0;
//   (dim->rotate).vy = 0;
//   (dim->rotate).vz = 0;
//   UpdateCoordinate(dim);
//   uVar9 = 0;
//   dim->id = -1;
//   dim->attribute = 0;
//   do {
//     iVar6 = (int)(dim->clip).vx;
//     iVar7 = (int)(short)uVar9;
//     if (iVar6 <= iVar7) {
//       (dim->rotate).pad = *(short *)(**(int **)&(dim->clip).vz + 0x1c);
//       return (OrnamentArchiveType *)dim;
//     }
//     pOVar3 = *(OrnamentType **)(iVar7 * 4 + *(int *)&(dim->clip).vz);
//     super = dim;
//     if ((-1 < (short)adr[iVar7 * 4 + 2]) && (iVar5 = 0, 0 < iVar6)) {
//       iVar4 = 0;
//       do {
//         iVar5 = iVar5 + 1;
//         if ((short)adr[iVar7 * 4 + 2] == *(short *)((int)adr + (iVar4 >> 0x10) * 0x10 + 10)) {
//           super = *(ModelType **)((iVar4 >> 0x10) * 4 + *(int *)&(dim->clip).vz);
//           break;
//         }
//         iVar4 = iVar5 * 0x10000;
//       } while (iVar5 * 0x10000 >> 0x10 < iVar6);
//     }
//     GsInitCoordinate2(&super->locate,&pOVar3->locate);
//     iVar7 = (int)((uint)uVar9 << 0x10) >> 0xc;
//     (pOVar3->locate).coord.t[0] = (int)*(short *)((int)adr + iVar7 + 0xc);
//     (pOVar3->locate).coord.t[1] = (int)*(short *)((int)adr + iVar7 + 0xe);
//     (pOVar3->locate).coord.t[2] = (int)*(short *)((int)adr + iVar7 + 0x10);
//     UpdateOrnament(pOVar3,0);
//     uVar9 = uVar9 + 1;
//     (pOVar3->object).attribute = (pOVar3->object).attribute | 0x400;
//   } while( true );
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? GsInitCoordinate2(? *, ? *, s16);                 /* extern */
// s32 LoadOrnament(s32);                              /* extern */
// ? SystemOut(? *);                                   /* extern */
// ? UpdateCoordinate(? *);                            /* extern */
// ? UpdateOrnament(? *, ?);                           /* extern */
// ? *valloc(s32);                                     /* extern */
// extern ? D_800120AC;
// extern ? *World;
//
// void LoadOrnamentArchive(void *arg0, ? *arg1) {
//     ? *temp_s0_3;
//     ? *temp_v0;
//     ? *var_a0;
//     ? *var_a0_2;
//     s16 temp_a2;
//     s16 temp_s0_2;
//     s16 temp_v0_3;
//     s16 temp_v0_4;
//     s16 var_s2;
//     s16 var_s2_2;
//     s16 var_v1;
//     s32 temp_a0;
//     s32 temp_a1;
//     s32 temp_s0;
//     s32 var_v0;
//     u16 temp_v0_2;
//     void *temp_v0_5;
//
//     if (arg0 == NULL) {
//         SystemOut(&D_800120AC);
//     }
//     temp_v0 = valloc(0x68);
//     temp_v0->unk64 = arg0;
//     temp_v0_2 = arg0->unk4;
//     temp_s0 = arg0 + 4 + 4;
//     var_s2 = 0;
//     temp_v0->unk5C = temp_v0_2;
//     temp_v0->unk60 = valloc((s32) (temp_v0_2 << 0x10) >> 0xE);
// loop_3:
//     temp_s0_2 = var_s2;
//     if (temp_s0_2 < (s16) temp_v0->unk5C) {
//         temp_a0 = ((temp_s0_2 * 0x10) + temp_s0)->unkC;
//         var_s2 += 1;
//         temp_v0->unk60[temp_s0_2] = (? *) (LoadOrnament(temp_s0 + temp_a0) | 0xA0000000);
//         goto loop_3;
//     }
//     var_a0 = arg1;
//     if (arg1 == NULL) {
//         var_a0 = &World;
//     }
//     GsInitCoordinate2(var_a0, temp_v0);
//     temp_v0->unk18 = 0;
//     temp_v0->unk1C = 0;
//     temp_v0->unk20 = 0;
//     temp_v0->unk50 = 0;
//     temp_v0->unk52 = 0;
//     temp_v0->unk54 = 0;
//     UpdateCoordinate(temp_v0);
//     var_s2_2 = 0;
//     temp_v0->unk58 = -1;
//     temp_v0->unk5A = 0;
// loop_8:
//     temp_a2 = (s16) temp_v0->unk5C;
//     if (var_s2_2 < temp_a2) {
//         temp_v0_3 = *((var_s2_2 * 0x10) + temp_s0);
//         temp_s0_3 = temp_v0->unk60[var_s2_2];
//         var_a0_2 = temp_v0;
//         if (temp_v0_3 >= 0) {
//             var_v1 = 0;
//             if (temp_a2 > 0) {
//                 var_v0 = 0 << 0x10;
// loop_12:
//                 temp_a1 = var_v0 >> 0x10;
//                 temp_v0_4 = var_v1 + 1;
//                 if (temp_v0_3 != ((temp_a1 * 0x10) + temp_s0)->unk2) {
//                     var_v1 = temp_v0_4;
//                     var_v0 = var_v1 << 0x10;
//                     if (temp_v0_4 < temp_a2) {
//                         goto loop_12;
//                     }
//                 } else {
//                     var_a0_2 = temp_v0->unk60[temp_a1];
//                 }
//             }
//         }
//         GsInitCoordinate2(var_a0_2, temp_s0_3, temp_a2);
//         temp_v0_5 = ((s32) (var_s2_2 << 0x10) >> 0xC) + temp_s0;
//         temp_s0_3->unk18 = (s32) temp_v0_5->unk4;
//         temp_s0_3->unk1C = (s32) temp_v0_5->unk6;
//         temp_s0_3->unk20 = (s32) temp_v0_5->unk8;
//         UpdateOrnament(temp_s0_3, 0);
//         var_s2_2 += 1;
//         temp_s0_3->unk50 = (s32) (temp_s0_3->unk50 | 0x400);
//         goto loop_8;
//     }
//     temp_v0->unk56 = (u16) (*temp_v0->unk60)->unk1C;
// }
