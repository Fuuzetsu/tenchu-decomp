#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupTelop(unsigned char *telop);
 *     CHRANIM.C:372, 44 src lines, frame 560 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     param $s4       unsigned char * telop
 *     reg   $a2       short * font
 *     stack sp+16     short [16][16] bitmap
 *     reg   $s0       short n
 *     reg   $t0       short u
 *     reg   $t1       short v
 *     stack sp+528    struct RECT rect
 *
 * Globals it touches, as the original declared them:
 *     extern struct POLY_FT4 TelopP;
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SetupTelop", SetupTelop);
#else

typedef struct
{
    s16 x, y, w, h;
} RECT;

typedef struct
{
    u8 pad[3];
    u8 len;
} TelopTag;

typedef struct
{
    TelopTag tag;
    u8 r0, g0, b0, code;
    s16 x0, y0;
    u8 u0, v0;
    u16 clut;
    s16 x1, y1;
    u8 u1, v1;
    u16 tpage;
    s16 x2, y2;
    u8 u2, v2;
    u16 pad1;
    s16 x3, y3;
    u8 u3, v3;
    u16 pad2;
} TelopPoly;

extern TelopPoly TelopP;
extern u16 D_8008F078[];

extern int ClearImage(RECT *rect, u8 r, u8 g, u8 b);
extern int DrawSync(int mode);
extern s16 *Krom2RawAdd(u32 code);
extern int LoadImage(RECT *rect, u_long *pixels);
extern void *memset(void *dst, int value, u32 size);

void SetupTelop(u8 *telop, short line)
{
    s16 bitmap[16][16];
    RECT rect;
    s16 n;
    s16 u;
    s16 v;
    s16 col;
    s16 *font;
    s16 *pixel;
    s32 pixel_offset;
    s16 bits;
    u16 raw_bits;
    s16 north;
    s16 fill_white;
    s16 outline_white;
    s32 scaled_y;
    s32 line_y;
    s32 signed_v;
    s32 final_u;
    s32 final_v;
    s32 final_v2;

    TelopP.u1 = 0;
    TelopP.u0 = 0;
    if ((*telop & 0x80) != 0 && (telop[2] & 0x80) != 0)
    {
        scaled_y = ((s32)line << 0x10) >> 0xc;
        rect.x = 0x300;
        rect.y = 0x1f0 - scaled_y;
        rect.w = 0x100;
        rect.h = 0xf;
        line_y = scaled_y;
        ClearImage(&rect, 0, 0, 0);
        DrawSync(0);
        rect.w = 0x10;
        rect.h = 0xf;

        do
        {
            if (*telop == 0)
            {
                TelopP.u1 = 0;
                TelopP.u0 = 0;
                return;
            }
        } while (0);

        n = 0;
        while (1)
        {
            if (n >= 0x20)
            {
                break;
            }

            do
            {
                do
                {
                    do
                    {
                        do
                        {
                            do
                            {
                                if (telop[n] == 0x81 && telop[n + 1] == 0x99)
                                {
                                    font = D_8008F078;
                                }
                                else
                                {
                                    font = Krom2RawAdd((telop[n] << 8) | telop[n + 1]);
                                }
                            } while (0);
                        } while (0);
                    } while (0);
                } while (0);
            } while (0);

            if (font != (s16 *)-1)
            {
                fill_white = 0x7fff;
                v = 0;
                do
                {
                    raw_bits = font[v];
                    bits = raw_bits >> 8;
                    bits |= raw_bits << 8;
                    col = 0;
                    do
                    {
                        do
                        {
                            pixel_offset = (15 - col) * 2 + v * 0x20;
                        } while (0);
                        final_u = fill_white;
                        pixel = (s16 *)((u8 *)bitmap + pixel_offset);
                        if ((bits >> col) & 1)
                        {
                            *pixel = final_u;
                        }
                        else
                        {
                            *pixel = 0;
                        }
                        col++;
                    } while (col < 16);
                    do
                    {
                        do
                        {
                            v++;
                        } while (0);
                    } while (0);
                } while (v < 15);

                outline_white = 0x7fff;
                u = 1;
                v = 1;
                do
                {
                    if (bitmap[v][u] == 0)
                    {
                        north = bitmap[v - 1][u];
                        if ((north == outline_white && bitmap[v][u - 1] == outline_white) ||
                            (bitmap[v][u - 1] == outline_white && bitmap[v + 1][u] == outline_white) ||
                            (bitmap[v + 1][u] == outline_white && bitmap[v][u + 1] == outline_white) ||
                            (bitmap[v][u + 1] == outline_white && north == outline_white))
                        {
                            bitmap[v][u] = 0x1ce7;
                        }
                    }
                    do
                    {
                        u++;
                    } while (0);
                    signed_v = v;
                    if (u >= 15)
                    {
                        u = 1;
                        v = signed_v + 1;
                    }
                    else
                    {
                        v = signed_v;
                    }
                } while (v < 14);

                LoadImage(&rect, (u_long *)bitmap);
                DrawSync(0);
                rect.x += rect.w;
            }

            n += 2;
            if (telop[n] == 0)
            {
                break;
            }
        }

        memset(&TelopP, 0xff, sizeof(TelopP));
        final_v = 0xf0 - line_y;
        final_v2 = (u8)rect.h + final_v;
        final_u = (u16)rect.x - 0x301;
        TelopP.tag.len = 9;
        TelopP.code = 0x2c;
        TelopP.u2 = 0;
        TelopP.u0 = 0;
        TelopP.v1 = final_v;
        TelopP.v0 = final_v;
        TelopP.u3 = final_u;
        TelopP.u1 = final_u;
        TelopP.v3 = final_v2;
        TelopP.v2 = final_v2;
        TelopP.tpage = GetTPage(2, 0, 0x300,
                               0x1f0 - (s16)line_y);
    }
}

#endif

// triage: HARD — 269 insns, 4 loop, frame 0x230, 6 callees, ~0.04 to BriefingAndInventorySelectionScreen
// likely-relevant cookbook sections:
//   - Loops: 4 back-edge(s) — for/while/do vs goto shape
//   - Stack objects: 0x230 frame — buffer casts / spills

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void SetupTelop(uchar *telop)
//
// {
//   short sVar1;
//   short sVar2;
//   undefined2 *puVar3;
//   int iVar4;
//   int iVar5;
//   int iVar6;
//   int in_a1;
//   int iVar7;
//   undefined2 *puVar8;
//   int iVar9;
//   ushort uVar10;
//   int iVar11;
//   int iVar12;
//   u_long auStack_220 [128];
//   RECT local_20;
//
//   TelopP.u1 = '\0';
//   TelopP.u0 = '\0';
//   if (((*telop & 0x80) != 0) && ((telop[2] & 0x80) != 0)) {
//     iVar5 = (in_a1 << 0x10) >> 0xc;
//     local_20.x = 0x300;
//     sVar1 = (short)iVar5;
//     local_20.y = 0x1f0 - sVar1;
//     local_20.w = 0x100;
//     local_20.h = 0xf;
//     ClearImage(&local_20,'\0','\0','\0');
//     DrawSync(0);
//     local_20.w = 0x10;
//     local_20.h = 0xf;
//     if (*telop == '\0') {
//       TelopP.u1 = '\0';
//       TelopP.u0 = '\0';
//     }
//     else {
//       iVar12 = 0;
//       do {
//         sVar2 = (short)iVar12;
//         if (0x1f < sVar2) break;
//         if ((telop[sVar2] == 0x81) && ((telop + sVar2)[1] == 0x99)) {
//           puVar8 = &DAT_8008f078;
//         }
//         else {
//           puVar8 = (undefined2 *)Krom2RawAdd((uint)CONCAT11(telop[sVar2],(telop + sVar2)[1]));
//         }
//         iVar11 = 0;
//         if (puVar8 != (undefined2 *)0xffffffff) {
//           do {
//             iVar4 = 0;
//             uVar10 = puVar8[(short)iVar11];
//             iVar6 = 0;
//             do {
//               puVar3 = (undefined2 *)
//                        ((int)auStack_220 + (0xf - (iVar6 >> 0x10)) * 2 + (short)iVar11 * 0x20);
//               if (((int)(short)(uVar10 >> 8 | uVar10 << 8) >> (iVar6 >> 0x10 & 0x1fU) & 1U) == 0) {
//                 *puVar3 = 0;
//               }
//               else {
//                 *puVar3 = 0x7fff;
//               }
//               iVar4 = iVar4 + 1;
//               iVar6 = iVar4 * 0x10000;
//             } while (iVar4 * 0x10000 >> 0x10 < 0x10);
//             iVar11 = iVar11 + 1;
//           } while (iVar11 * 0x10000 >> 0x10 < 0xf);
//           iVar4 = 1;
//           uVar10 = 1;
//           iVar11 = 0x10000;
//           do {
//             iVar11 = iVar11 >> 0x10;
//             iVar9 = iVar11 * 2;
//             iVar7 = (int)(short)uVar10;
//             iVar6 = iVar7 * 0x20;
//             if (*(short *)((int)auStack_220 + iVar9 + iVar6) == 0) {
//               sVar2 = *(short *)((int)auStack_220 + iVar9 + (iVar7 + -1) * 0x20);
//               if (((((sVar2 == 0x7fff) &&
//                     (*(short *)((int)auStack_220 + (iVar11 + -1) * 2 + iVar6) == 0x7fff)) ||
//                    ((*(short *)((int)auStack_220 + (iVar11 + -1) * 2 + iVar6) == 0x7fff &&
//                     (*(short *)((int)auStack_220 + iVar9 + (iVar7 + 1) * 0x20) == 0x7fff)))) ||
//                   ((*(short *)((int)auStack_220 + iVar9 + (iVar7 + 1) * 0x20) == 0x7fff &&
//                    (*(short *)((int)auStack_220 + (iVar11 + 1) * 2 + iVar6) == 0x7fff)))) ||
//                  ((*(short *)((int)auStack_220 + (iVar11 + 1) * 2 + iVar6) == 0x7fff &&
//                   (sVar2 == 0x7fff)))) {
//                 *(undefined2 *)
//                  ((int)auStack_220 + ((iVar4 << 0x10) >> 0xf) + ((int)((uint)uVar10 << 0x10) >> 0xb)
//                  ) = 0x1ce7;
//               }
//             }
//             iVar4 = iVar4 + 1;
//             if (0xe < iVar4 * 0x10000 >> 0x10) {
//               iVar4 = 1;
//               uVar10 = uVar10 + 1;
//             }
//             iVar11 = iVar4 << 0x10;
//           } while ((short)uVar10 < 0xe);
//           LoadImage(&local_20,auStack_220);
//           DrawSync(0);
//           local_20.x = local_20.x + local_20.w;
//         }
//         iVar12 = iVar12 + 2;
//       } while (telop[iVar12 * 0x10000 >> 0x10] != '\0');
//       memset((uchar *)&TelopP,0xff,0x28);
//       TelopP.tag._3_1_ = 9;
//       TelopP._2 = 0xf0 - (char)iVar5;
//       TelopP.code = ',';
//       TelopP.u2 = '\0';
//       TelopP.u0 = '\0';
//       TelopP.u1 = (char)local_20.x + 0xff;
//       TelopP.v2 = (char)local_20.h + TelopP._2;
//       TelopP._3 = TelopP._2;
//       TelopP.u3 = TelopP.u1;
//       TelopP.v3 = TelopP.v2;
//       TelopP.tpage = GetTPage(2,0,0x300,0x1f0 - sVar1);
//     }
//   }
//   return;
// }
