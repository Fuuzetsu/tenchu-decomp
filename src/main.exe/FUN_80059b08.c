#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>
#include "gte.h"

/*
 * FUN_80059b08 (0x80059b08, 0x4ec bytes) — DecodeTMD-family primitive
 * renderer, the 1.00 mnemonic clone of FUN_8005961c (the POLY_GT4 pair of
 * the family; all members share the (u_short *, u_long, u_long *, int,
 * u_long *) signature). The ONLY differences from FUN_8005961c are the
 * record layout constants: rec starts at param_1+0x14 (not +0x20), the four
 * UV words sit at rec-0x10..rec-4 (not rec-0x1C..rec-0x10), all FOUR
 * per-vertex colour words read the ONE source word at rec+0 (the
 * flat-colour variant — FUN_8005961c reads four distinct words at
 * rec-0xC..rec+0), and the record stride is 0x20 (not 0x2C). Everything
 * else — including every matching note — is FUN_8005961c.c verbatim; read
 * that file's header for the full mechanism account.
 *
 * Matching notes: applies the FUN_80059ff4 recipe verbatim (read that
 * header). The original TMD_P_TNF4 record type keeps the normal strength-
 * reduced loop on the target's single cursor; the former function-only flag
 * was compensating for decompiler-style byte offsets. New vs the leaf: work
 * lives as TWO variables (work + the prim copy — the packet accesses go through prim, the context
 * fields through work); flagAddr is a precomputed loop invariant (used by
 * BOTH gte_stflg sites); the 52-byte POLY_GT4 struct assignment emits a
 * 3-chunk movstrsi loop + 4-byte remainder.
 */

u_long *FUN_80059b08(u_short *param_1, u_long param_2, u_long *param_3, int param_4, u_long *param_5)
{
    u8 *work;
    u8 *prim;
    u_long *flagAddr;
    u8 *codeAddr;
    s32 codeVal;
    u_long *sz0Ptr;
    TMD_P_TNF4 *record;
    u_long *rgbPtr;
    u_long *otSlot;
    u32 idx0, idx1, idx2;
    s32 b, c, lo, hi, otz;
    s32 z1, z2;

    work = (u8 *)param_5;
    prim = work;
    if (param_4 != 0) {
        flagAddr = (u_long *)(work + 0x74);
        codeAddr = work + 4;
        codeVal = 0x3C;
        sz0Ptr = (u_long *)(work + 0x5C);
        record = (TMD_P_TNF4 *)param_1;
        do {
            idx0 = record->v0;
            idx1 = record->v1;
            idx2 = record->v2;
            gte_ldv3((SVECTOR *)(idx0 * 8 + param_2), (SVECTOR *)(idx1 * 8 + param_2),
                     (SVECTOR *)(idx2 * 8 + param_2));
            gte_rtpt();

            *(s32 *)(prim + 0xC) = *(s32 *)&record->tu0;
            *(s32 *)(prim + 0x18) = *(s32 *)&record->tu1;
            *(s32 *)(prim + 0x24) = *(s32 *)&record->tu2;
            gte_stflg(flagAddr);
            if (*(s32 *)(work + 0x74) < 0) goto next;

            gte_nclip();
            *(s32 *)(prim + 4) = *(s32 *)&record->r0;
            codeAddr[3] = codeVal;
            gte_stopz((u_long *)(work + 0x80));
            if (*(s32 *)(work + 0x80) <= 0) goto next;

            gte_stsxy3_gt3(prim);
            gte_ldv0((SVECTOR *)(record->v3 * 8 + param_2));
            gte_rtps();

            *(s32 *)(prim + 0x30) = *(s32 *)&record->tu3;
            *(s32 *)(prim + 0x10) = *(s32 *)&record->r0;
            *(s32 *)(prim + 0x1C) = *(s32 *)&record->r0;
            *(s32 *)(prim + 0x28) = *(s32 *)&record->r0;
            gte_stflg(flagAddr);
            if (*(s32 *)(work + 0x74) < 0) goto next;

            gte_stsxy((u_long *)(prim + 0x2C));

            lo = *(s16 *)(prim + 8);
            b = *(s16 *)(prim + 0x14);
            if (b < lo) {
                hi = lo;
                lo = b;
            } else {
                hi = b;
            }
            c = *(s16 *)(prim + 0x20);
            if (c < lo) {
                lo = c;
            } else if (hi < c) {
                hi = c;
            }
            c = *(s16 *)(prim + 0x2C);
            if (c < lo) {
                lo = c;
            } else if (hi < c) {
                hi = c;
            }
            if (hi < *(s32 *)(work + 0x94)) goto next;
            if (*(s32 *)(work + 0x98) < lo) goto next;

            lo = *(s16 *)(prim + 0xA);
            b = *(s16 *)(prim + 0x16);
            if (b < lo) {
                hi = lo;
                lo = b;
            } else {
                hi = b;
            }
            c = *(s16 *)(prim + 0x22);
            if (c < lo) {
                lo = c;
            } else if (hi < c) {
                hi = c;
            }
            c = *(s16 *)(prim + 0x2E);
            if (c < lo) {
                lo = c;
            } else if (hi < c) {
                hi = c;
            }
            if (hi < *(s32 *)(work + 0x9C)) goto next;
            if (*(s32 *)(work + 0xA0) < lo) goto next;

            gte_stsz4(sz0Ptr, (u_long *)(work + 0x60), (u_long *)(work + 0x64),
                      (u_long *)(work + 0x68));
            lo = *(s32 *)(work + 0x5C);
            b = *(s32 *)(work + 0x60);
            if (b < lo) {
                otz = lo;
                lo = b;
            } else {
                otz = b;
            }
            c = *(s32 *)(work + 0x64);
            if (c < lo) {
                lo = c;
            } else if (otz < c) {
                otz = c;
            }
            c = *(s32 *)(work + 0x68);
            if (c < lo) {
                lo = c;
            } else if (otz < c) {
                otz = c;
            }
            if (*(s32 *)(work + 0x84) < lo) goto next;

            *(s32 *)(work + 0x78) = otz / 4;
            z1 = *(s32 *)(work + 0x8C);
            if (z1 < otz) {
                rgbPtr = (u_long *)(prim + 4);
                z2 = *(s32 *)(work + 0x5C);
                gte_ldrgb(rgbPtr);
                gte_lddp(z2 - z1);
                gte_dpcs();
                z2 = z1 < z2;
                if (z2 != 0) {
                    gte_strgb(rgbPtr);
                }

                rgbPtr = (u_long *)(prim + 0x10);
                z1 = *(s32 *)(work + 0x60);
                gte_ldrgb(rgbPtr);
                z2 = *(s32 *)(work + 0x8C);
                gte_lddp(z1 - z2);
                gte_dpcs();
                z2 = z2 < z1;
                if (z2 != 0) {
                    gte_strgb(rgbPtr);
                }

                rgbPtr = (u_long *)(prim + 0x1C);
                z1 = *(s32 *)(work + 0x64);
                gte_ldrgb(rgbPtr);
                z2 = *(s32 *)(work + 0x8C);
                gte_lddp(z1 - z2);
                gte_dpcs();
                z2 = z2 < z1;
                if (z2 != 0) {
                    gte_strgb(rgbPtr);
                }

                rgbPtr = (u_long *)(prim + 0x28);
                z1 = *(s32 *)(work + 0x68);
                gte_ldrgb(rgbPtr);
                z2 = *(s32 *)(work + 0x8C);
                gte_lddp(z1 - z2);
                gte_dpcs();
                z2 = z2 < z1;
                if (z2 != 0) {
                    gte_strgb(rgbPtr);
                }
            }

            otSlot = *(u_long **)(*(u_long *)(work + 0x90) + 4) +
                     (*(s32 *)(work + 0x78) >> *(s32 *)(work + 0x88));
            *(u_long *)prim = *otSlot;
            prim[3] = 0xC;
            *(POLY_GT4 *)param_3 = *(POLY_GT4 *)prim;
            *otSlot = (u_long)param_3 & 0xFFFFFF;
            param_3 += 0xD;

        next:
            param_4--;
            record++;
        } while (param_4 != 0);
    }
    return param_3;
}
