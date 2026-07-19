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

#include "item.h"

/*
 * STATUS: MATCHING — pure C, all 760 bytes / 190 instructions exact.
 *
 * The second pass follows LoadOrnamentArchive's proven parent-search shape.
 * Giving its parent pointer a separate `super` identity lets it die in a3 at
 * GsInitCoordinate2 while `objp` remains in s0. The unsigned count view and
 * signed `mad->n` field have the same verified 0x64 address but distinct C
 * identities, preserving the target's adjacent lhu/lh loads instead of cc1
 * folding them together. `limit` then keeps j in v1 through the inner loop.
 */
extern void *valloc(u32 size);
extern MATRIX *RotMatrixYXZ(SVECTOR *r, MATRIX *m);
extern void SystemOut(char *msg);
extern char D_80011064[]; /* "NO MODEL ARCHIVE DATA" */

typedef struct
{
    GsCOORDINATE2 locate; /* 0x00 */
} WorldType;

extern WorldType World;

typedef struct
{
    u8 pad[0x64];
    u16 n;
} ModelArchiveCountView;

typedef struct
{
    s16 parent;  /* 0x0 */
    s16 id;      /* 0x2 */
    s16 x;       /* 0x4 */
    s16 y;       /* 0x6 */
    s16 z;       /* 0x8 */
    s32 offset;  /* 0xC */
} ParentingType; /* size 0x10 */

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

    if (adr == 0) {
        SystemOut(D_80011064);
    }
    mad = (ModelArchiveType *)valloc(sizeof(ModelArchiveType));
    adr++;
    mad->n = *(short *)adr;
    adr++;
    i = 0;
    mad->object = (ModelType **)valloc(mad->n * sizeof(ModelType *));
    prntp = (ParentingType *)adr;
    tmdp = (u8 *)prntp;
    if (0 < mad->n) {
        do {
            dtmd = (int)tmdp + prntp[i].offset;
            dim = (ModelType *)valloc(sizeof(ModelType));
            if (dtmd != 0) {
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
            i = i + 1;
        } while (i < mad->n);
    }
    if (prnt == 0) {
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
    count = ((ModelArchiveCountView *)mad)->n;
    if (0 < mad->n) {
        do {
            objp = mad->object[i];
            super = (ModelType *)mad;
            if (prntp[i].parent >= 0 && (j = 0, 0 < (count << 16))) {
                parent = prntp[i].parent;
                limit = mad->n;
                do {
                    if (parent == prntp[j].id) {
                        super = mad->object[j];
                        goto coordinate_init;
                    }
                    j = j + 1;
                } while (j < limit);
            }
coordinate_init:
            GsInitCoordinate2(&super->locate, &objp->locate);
            objp->locate.coord.t[0] = prntp[i].x;
            objp->locate.coord.t[1] = prntp[i].y;
            objp->locate.coord.t[2] = prntp[i].z;
            RotMatrixYXZ(&objp->rotate, &objp->locate.coord);
            i = i + 1;
            objp->locate.flg = 0;
            count = ((ModelArchiveCountView *)mad)->n;
        } while (i < mad->n);
    }
    mad->rotate.pad = (short)mad->object[0]->locate.coord.t[1];
    return mad;
}
// triage: MEDIUM — 190 insns, 3 loop, 6 callees, ~0.13 to LoadModel
// likely-relevant cookbook sections:
//   - Loops: 3 back-edge(s) — for/while/do vs goto shape

// Ghidra decompilation (reference):
//
//
// ModelArchiveType * LoadModelArchive(ulong *adr,ModelType *prnt)
//
// {
//   short sVar1;
//   ushort uVar2;
//   ulong uVar3;
//   ModelType *base;
//   ModelType **ppMVar4;
//   int iVar5;
//   ModelType *pMVar6;
//   int iVar7;
//   ModelType *base_00;
//   int iVar8;
//   int iVar9;
//
//   if (adr == (ulong *)0x0) {
//                     /* WARNING: Subroutine does not return */
//     SystemOut((uchar *)"NO MODEL ARCHIVE DATA");
//   }
//   base = (ModelType *)valloc(0x6c);
//   uVar3 = adr[1];
//   iVar9 = 0;
//   *(ushort *)&base->object = (ushort)uVar3;
//   ppMVar4 = (ModelType **)valloc((int)((uint)(ushort)uVar3 << 0x10) >> 0xe);
//   *(ModelType ***)((int)&base->object + 4) = ppMVar4;
//   if (0 < *(short *)&base->object) {
//     iVar5 = 0;
//     do {
//       iVar8 = (int)adr + adr[(iVar5 >> 0x10) * 4 + 5] + 8;
//       pMVar6 = (ModelType *)valloc(0x74);
//       if (iVar8 != 0) {
//         GsMapModelingData((ulong *)(iVar8 + 4));
//         GsLinkObject4(iVar8 + 0xc,&pMVar6->object,0);
//       }
//       (pMVar6->object).coord2 = (GsCOORDINATE2 *)pMVar6;
//       (pMVar6->object).attribute = 0;
//       GsInitCoordinate2(&World.locate,(GsCOORDINATE2 *)pMVar6);
//       (pMVar6->locate).coord.t[0] = 0;
//       (pMVar6->locate).coord.t[1] = 0;
//       (pMVar6->locate).coord.t[2] = 0;
//       (pMVar6->rotate).vx = 0;
//       (pMVar6->rotate).vy = 0;
//       (pMVar6->rotate).vz = 0;
//       (pMVar6->clip).vx = 0;
//       (pMVar6->clip).vy = 0;
//       (pMVar6->clip).vz = 0;
//       RotMatrixYXZ(&pMVar6->rotate,&(pMVar6->locate).coord);
//       iVar9 = iVar9 + 1;
//       pMVar6->id = -1;
//       (pMVar6->locate).flg = 0;
//       pMVar6->attribute = 0;
//       (*(ModelType ***)((int)&base->object + 4))[iVar5 >> 0x10] = pMVar6;
//       iVar5 = iVar9 * 0x10000;
//     } while (iVar9 * 0x10000 >> 0x10 < (int)*(short *)&base->object);
//   }
//   if (prnt == (ModelType *)0x0) {
//     prnt = &World;
//   }
//   GsInitCoordinate2(&prnt->locate,(GsCOORDINATE2 *)base);
//   (base->locate).coord.t[0] = 0;
//   (base->locate).coord.t[1] = 0;
//   (base->locate).coord.t[2] = 0;
//   (base->rotate).vx = 0;
//   (base->rotate).vy = 0;
//   (base->rotate).vz = 0;
//   (base->clip).vx = 0;
//   (base->clip).vy = 0;
//   (base->clip).vz = 0;
//   RotMatrixYXZ(&base->rotate,&(base->locate).coord);
//   iVar9 = 0;
//   uVar2 = *(ushort *)&base->object;
//   sVar1 = *(short *)&base->object;
//   (base->locate).flg = 0;
//   base->id = -1;
//   base->attribute = 0;
//   if (0 < sVar1) {
//     iVar5 = 0;
//     do {
//       base_00 = (*(ModelType ***)((int)&base->object + 4))[iVar5 >> 0x10];
//       pMVar6 = base;
//       if ((-1 < (short)adr[(iVar5 >> 0x10) * 4 + 2]) && (iVar8 = 0, 0 < (int)((uint)uVar2 << 0x10)))
//       {
//         iVar7 = 0;
//         do {
//           iVar8 = iVar8 + 1;
//           if ((short)adr[(iVar5 >> 0x10) * 4 + 2] ==
//               *(short *)((int)adr + (iVar7 >> 0x10) * 0x10 + 10)) {
//             pMVar6 = (*(ModelType ***)((int)&base->object + 4))[iVar7 >> 0x10];
//             break;
//           }
//           iVar7 = iVar8 * 0x10000;
//         } while (iVar8 * 0x10000 >> 0x10 < (int)*(short *)&base->object);
//       }
//       GsInitCoordinate2(&pMVar6->locate,&base_00->locate);
//       iVar5 = (iVar9 << 0x10) >> 0xc;
//       (base_00->locate).coord.t[0] = (int)*(short *)((int)adr + iVar5 + 0xc);
//       (base_00->locate).coord.t[1] = (int)*(short *)((int)adr + iVar5 + 0xe);
//       (base_00->locate).coord.t[2] = (int)*(short *)((int)adr + iVar5 + 0x10);
//       RotMatrixYXZ(&base_00->rotate,&(base_00->locate).coord);
//       iVar9 = iVar9 + 1;
//       (base_00->locate).flg = 0;
//       uVar2 = (ushort)(base->object).attribute;
//       iVar5 = iVar9 * 0x10000;
//     } while (iVar9 * 0x10000 >> 0x10 < (int)(short)(base->object).attribute);
//   }
//   (base->rotate).pad = *(short *)(((base->object).coord2)->flg + 0x1c);
//   return (ModelArchiveType *)base;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? GsInitCoordinate2(? *, ? *);                      /* extern */
// ? GsLinkObject4(s32, ? *, ?);                       /* extern */
// ? GsMapModelingData(s32);                           /* extern */
// ? RotMatrixYXZ(? *, ? *);                           /* extern */
// ? SystemOut(? *);                                   /* extern */
// ? *valloc(s32);                                     /* extern */
// extern ? D_80011064;
// extern ? *World;
//
// void LoadModelArchive(void *arg0, ? *arg1) {
//     ? *temp_s0_2;
//     ? *temp_s0_3;
//     ? *temp_v0;
//     ? *var_a0;
//     ? *var_a0_2;
//     s16 temp_a0;
//     s16 temp_v0_2;
//     s16 temp_v0_4;
//     s16 temp_v1_2;
//     s16 var_s3;
//     s16 var_s3_2;
//     s16 var_v1;
//     s32 temp_a0_2;
//     s32 temp_s0;
//     s32 temp_s1;
//     s32 temp_s4;
//     s32 temp_v0_3;
//     s32 var_v0;
//     s32 var_v0_2;
//     s32 var_v0_3;
//     u16 temp_v1;
//     void *temp_v0_5;
//
//     if (arg0 == NULL) {
//         SystemOut(&D_80011064);
//     }
//     temp_v0 = valloc(0x6C);
//     temp_v1 = arg0->unk4;
//     temp_s0 = arg0 + 4 + 4;
//     var_s3 = 0;
//     temp_v0->unk64 = temp_v1;
//     temp_v0->unk68 = valloc((s32) (temp_v1 << 0x10) >> 0xE);
//     if ((s16) temp_v0->unk64 > 0) {
//         var_v0 = 0 << 0x10;
//         do {
//             temp_s4 = var_v0 >> 0x10;
//             temp_s1 = temp_s0 + ((temp_s4 * 0x10) + temp_s0)->unkC;
//             temp_s0_2 = valloc(0x74);
//             if (temp_s1 != 0) {
//                 GsMapModelingData(temp_s1 + 4);
//                 GsLinkObject4(temp_s1 + 0xC, temp_s0_2 + 0x64, 0);
//             }
//             temp_s0_2->unk68 = temp_s0_2;
//             temp_s0_2->unk64 = 0;
//             GsInitCoordinate2(&World, temp_s0_2);
//             temp_s0_2->unk18 = 0;
//             temp_s0_2->unk1C = 0;
//             temp_s0_2->unk20 = 0;
//             temp_s0_2->unk50 = 0;
//             temp_s0_2->unk52 = 0;
//             temp_s0_2->unk54 = 0;
//             temp_s0_2->unk5C = 0;
//             temp_s0_2->unk5E = 0;
//             temp_s0_2->unk60 = 0;
//             RotMatrixYXZ(temp_s0_2 + 0x50, temp_s0_2 + 4);
//             temp_v1_2 = var_s3 + 1;
//             var_s3 = temp_v1_2;
//             temp_s0_2->unk58 = -1;
//             temp_s0_2->unk0 = NULL;
//             temp_s0_2->unk5A = 0;
//             temp_v0->unk68[temp_s4] = temp_s0_2;
//             var_v0 = var_s3 << 0x10;
//         } while (temp_v1_2 < (s16) temp_v0->unk64);
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
//     temp_v0->unk5C = 0;
//     temp_v0->unk5E = 0;
//     temp_v0->unk60 = 0;
//     RotMatrixYXZ(temp_v0 + 0x50, temp_v0 + 4);
//     var_s3_2 = 0;
//     temp_v0->unk0 = NULL;
//     temp_v0->unk58 = -1;
//     temp_v0->unk5A = 0;
//     if ((s16) temp_v0->unk64 > 0) {
//         var_v0_2 = 0 << 0x10;
//         do {
//             temp_v0_3 = var_v0_2 >> 0x10;
//             temp_a0 = *((temp_v0_3 * 0x10) + temp_s0);
//             temp_s0_3 = temp_v0->unk68[temp_v0_3];
//             if ((temp_a0 >= 0) && (var_v1 = 0, ((temp_v0->unk64 << 0x10) > 0))) {
//                 var_v0_3 = 0 << 0x10;
// loop_14:
//                 temp_a0_2 = var_v0_3 >> 0x10;
//                 temp_v0_4 = var_v1 + 1;
//                 if (temp_a0 == ((temp_a0_2 * 0x10) + temp_s0)->unk2) {
//                     var_a0_2 = temp_v0->unk68[temp_a0_2];
//                 } else {
//                     var_v1 = temp_v0_4;
//                     var_v0_3 = var_v1 << 0x10;
//                     if (temp_v0_4 >= (s16) temp_v0->unk64) {
//                         goto block_17;
//                     }
//                     goto loop_14;
//                 }
//             } else {
// block_17:
//                 var_a0_2 = temp_v0;
//             }
//             GsInitCoordinate2(var_a0_2, temp_s0_3);
//             temp_v0_5 = ((s32) (var_s3_2 << 0x10) >> 0xC) + temp_s0;
//             temp_s0_3->unk18 = (s32) temp_v0_5->unk4;
//             temp_s0_3->unk1C = (s32) temp_v0_5->unk6;
//             temp_s0_3->unk20 = (s32) temp_v0_5->unk8;
//             RotMatrixYXZ(temp_s0_3 + 0x50, temp_s0_3 + 4);
//             temp_v0_2 = var_s3_2 + 1;
//             var_s3_2 = temp_v0_2;
//             temp_s0_3->unk0 = NULL;
//             var_v0_2 = var_s3_2 << 0x10;
//         } while (temp_v0_2 < (s16) temp_v0->unk64);
//     }
//     temp_v0->unk56 = (u16) (*temp_v0->unk68)->unk1C;
// }
