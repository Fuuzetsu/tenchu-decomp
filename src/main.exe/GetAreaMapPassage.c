#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct VECTOR * GetAreaMapPassage(unsigned long *area, struct VECTOR *pos, struct SVECTOR *vect, short n);
 *     CONFLICT.C:223, 31 src lines, frame 72 bytes, saved-reg mask 0x801f0000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $s4       unsigned long * area
 *     param $a1       struct VECTOR * pos
 *     param $s1       struct SVECTOR * vect
 *     param $s3       short n
 *     reg   $a0       struct AreaNodeType * node
 *     stack sp+24     long [2] x
 *     stack sp+32     long [2] z
 *     stack sp+40     long [2] y
 *
 * Globals it touches, as the original declared them:
 *     extern struct AreaNodeType *FieldArea;
 *     extern struct NodeIndexType *FieldIndex;
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/GetAreaMapPassage", GetAreaMapPassage);

// triage: MEDIUM — 145 insns, 6 loop, 1 callees, ~0.07 to InsertConflict
// likely-relevant cookbook sections:
//   - Loops: 6 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// VECTOR * GetAreaMapPassage(ulong *area,VECTOR *pos,SVECTOR *vect,short n)
//
// {
//   long lVar1;
//   int iVar2;
//
//   cv.vx = pos->vx;
//   cv.vy = pos->vy;
//   cv.vz = pos->vz;
//   cv.pad = pos->pad;
//   if ((int)((uint)(ushort)n << 0x10) < 1) {
//     n = 100;
//   }
//   do {
//     lVar1 = GetAreaMapLevel(area,cv.vx,cv.vy);
//     if (lVar1 == -0x80000000) {
//       cv.vz = cv.vz - vect->vz;
//       cv.vy = cv.vy - vect->vy;
//       cv.vx = cv.vx - vect->vx;
//       return &cv;
//     }
//     if (FieldIndex == (NodeIndexType *)area) {
//       iVar2 = -1000000;
//     }
//     else {
//       iVar2 = FieldIndex[-1].y * 10;
//     }
//     do {
//       cv.vx = cv.vx + vect->vx;
//       cv.vy = cv.vy + vect->vy;
//       n = n + -1;
//       cv.vz = cv.vz + vect->vz;
//       if (n == 0) {
//         return (VECTOR *)0x0;
//       }
//     } while ((((FieldArea->x1 * 10 <= cv.vx) && (cv.vx <= FieldArea->x2 * 10)) && (lVar1 <= cv.vy))
//             && (((cv.vy <= iVar2 && (FieldArea->z1 * 10 <= cv.vz)) && (cv.vz <= FieldArea->z2 * 10))
//                ));
//   } while( true );
// }
