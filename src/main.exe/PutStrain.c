#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void PutStrain(void);
 *     INFOVIEW.C:218, 70 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
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
 *     reg   $s0       int newpow
 *     reg   $a2       int r
 *     reg   $s1       struct GsSPRITE * spr
 *
 * Globals it touches, as the original declared them:
 *     extern long StrainRatio;
 *     extern long GameClock;
 *     extern struct GsSPRITE NumberImage;
 *     extern struct GsOT *OTablePt;
 *     extern struct Sprite3D *sprNapalm2;
 * END PSX.SYM */

extern s32 StrainRatio;
extern s32 GameClock;
extern GsSPRITE NumberImage;
extern GsOT *OTablePt;
extern GsSPRITE ItemSprite3Ds[4];
extern GsSPRITE D_800C082C;
extern GsSPRITE D_800C0850;
extern GsSPRITE D_800C0874;
extern u16 D_80097F68;

extern void GsSortSprite(GsSPRITE *sp, GsOT *ot, int pri);
extern s16 SoundEx(VECTOR *locate, s16 seid);
extern s32 rsin(u32 ang);

/*
 * PutStrain (0x8004a8f0) — draws the "strain" HUD icon at (x,y): a
 * flashing/pulsing warning glyph whose sprite and pulse phase depend on
 * StrainRatio. 0x7fffffff (a sentinel) draws nothing at all. Otherwise:
 * ratio==0 -> D_800C0850 (neutral icon, no pulse-drift clamp); ratio <
 * -20000 -> D_800C0874 (icon, clamps ratio to 0 for the pulse calc);
 * -20000<=ratio<0 -> D_800C082C (icon; also plays SoundEx(0,0xe) every 30
 * ticks — a warning beep — and clamps ratio to 0); 0<ratio<=20000 draws a
 * PutNumber-style right-to-left digit strip of `(20000-ratio)/200` using
 * NumberImage (the numeric strain percentage) THEN falls through to using
 * ItemSprite3Ds as the icon; ratio>20000 returns without drawing anything.
 * The chosen icon sprite is then positioned at (x,y), tinted a shade of
 * gray that oscillates via rsin() driven by a persistent phase counter
 * (D_80097F68, advanced by the (adjusted) strain delta), and scaled by a
 * strain-proportional factor, then GsSortSprite'd.
 *
 * Matching notes:
 *  - `GameClock == (GameClock / 30) * 30` is EndDrawing.c's proven div-by-30
 *    modulo-test spelling (the magic-multiply reproduces from the literal
 *    div/mul, not `% 30 == 0`).
 *  - The digit loop is PutNumber.c's own do-while shape (goto-free real
 *    do-while; `r` is the quotient, reused as the next iteration's dividend
 *    exactly like PutNumber's `q`).
 *  - `ratio` is ONE variable doing double duty: the dispatch value AND (in
 *    the two negative branches, reset to 0) the value the tail geometry
 *    reads — Ghidra's decompilation renders these as two different
 *    variables (`lVar6`/`StrainRatio`) but m2c's raw register trace shows
 *    a single reused pseudo across both roles.
 *  - The phase-advance uses `delta` ADJUSTED (+0x1f when negative, an
 *    arithmetic-shift-rounds-toward-negative-infinity correction before the
 *    `>>5`), but the scale factor uses the RAW (unadjusted) `delta` — two
 *    separate reads of the same "20000-ratio" expression, not one shared
 *    temp.
 *  - CRITICAL: `NumberImage.u` is NOT read/restored globally — Ghidra's
 *    `uVar1 = NumberImage.u;` at the very top (before the StrainRatio
 *    dispatch) is a decompiler artifact; the target never touches
 *    NumberImage at all in the three simple (non-digit-loop) branches.
 *    `base`/`img` are LOCAL to the digit-loop (`else`) branch only — the
 *    earlier draft that hoisted them to function scope cost an extra
 *    saved register (frame 56 vs target's 48) and never converged; scoping
 *    them into the branch fixed the length exactly.
 *
 * STATUS: NON_MATCHING — 194 of 584 bytes differ, but the draft is the
 * CORRECT LENGTH (146/146 instructions). The residual is (1) a pure
 * register-priority swap: target keeps the digit-loop's `base` in $s3 and
 * the dispatched icon `spr` in $s4; this draft has them swapped ($s3=spr,
 * $s4=base) — tried reordering both the declarations and the `spr =
 * ItemSprite3Ds;` statement's position, neither changed it, so it's a
 * global-alloc priority tie rather than a declaration-order lever; and (2)
 * one narrower tie: the `rsin(phase)` call zero-extends `phase` in the
 * target (`andi $a0,$v0,0xffff`, 1 instruction) but sign-extends here
 * (`sll/sra`, 2 instructions) even though `phase` is declared `u16` per
 * PSX.SYM's own `unsigned char`-adjacent field types — reverting `phase`
 * to `u16` (semantically "more correct") REGRESSES the whole-function
 * length by 4 bytes elsewhere (some other site needs the sign-extending
 * `s16` shape to hit the target length), and an explicit `(u16)phase` cast
 * at just the call site made no difference either. A bounded permuter run
 * (`timeout 300 tools/permute.py PutStrain -- --stop-on-zero -j4`,
 * multiple thousand iterations) plateaued at best score 410 (from a base
 * of 880), never reaching 0 — consistent with both being sub-C-level ties.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/PutStrain", PutStrain);
#else
void PutStrain(s32 x, s32 y)
{
    s32 ratio;
    GsSPRITE *spr;
    s32 delta;
    s32 s;
    s32 iVar4;
    s16 phase;
    u8 shade;
    s16 scale;

    ratio = StrainRatio;
    if (ratio != 0x7fffffff)
    {
        if (ratio == 0)
        {
            spr = &D_800C0850;
        }
        else if (ratio < -20000)
        {
            spr = &D_800C0874;
            ratio = 0;
        }
        else if (ratio < 0)
        {
            spr = &D_800C082C;
            ratio = 0;
            if (GameClock == (GameClock / 30) * 30)
            {
                SoundEx(0, 0xe);
            }
        }
        else
        {
            u8 base;
            GsSPRITE *img;
            s32 newpow;
            s32 r;

            if (ratio > 20000)
                return;
            img = &NumberImage;
            img->w = 4;
            img->x = (s16)(x + 0x22);
            base = img->u;
            img->y = (s16)(y + 8);
            newpow = (20000 - ratio) / 200;
            spr = ItemSprite3Ds;
        strainloop:
            r = newpow / 10;
            img->u = base + (newpow - r * 10) * 4;
            GsSortSprite(img, OTablePt, 0);
            img->x = img->x - 6;
            newpow = r;
            if (newpow != 0)
                goto strainloop;
            img->u = base;
        }

        delta = 20000 - ratio;
        s = delta;
        if (delta < 0)
            s = delta + 0x1f;

        spr->x = (s16)x;
        spr->y = (s16)y;
        phase = D_80097F68 + (s >> 5);
        D_80097F68 = phase;
        iVar4 = rsin(phase) * 0x60;
        if (iVar4 < 0)
            iVar4 = iVar4 + 0xfff;
        shade = (iVar4 >> 0xc) + 0x7f;
        spr->b = shade;
        spr->g = shade;
        spr->r = shade;
        scale = (s16)((delta << 0xb) / 20000) + 0x800;
        spr->scalex = scale;
        spr->scaley = scale;
        GsSortSprite(spr, OTablePt, 0);
    }
}
#endif /* NON_MATCHING */

// triage: MEDIUM — 146 insns, mul/div, 1 loop, 3 callees, ~0.04 to AdtFntOpen
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void PutStrain(void)
//
// {
//   GsOT *ot;
//   uchar uVar1;
//   short sVar2;
//   uint uVar3;
//   int iVar4;
//   short in_a0;
//   short in_a1;
//   int iVar5;
//   long lVar6;
//   GsSPRITE *_29;
//
//   uVar1 = NumberImage.u;
//   lVar6 = StrainRatio;
//   if (StrainRatio != 0x7fffffff) {
//     if (StrainRatio == 0) {
//       _29 = (GsSPRITE *)&DAT_800c0850;
//       NumberImage.u = uVar1;
//     }
//     else if (StrainRatio < -20000) {
//       _29 = (GsSPRITE *)0x800c0874;
//       lVar6 = 0;
//       NumberImage.u = uVar1;
//     }
//     else if (StrainRatio < 0) {
//       _29 = (GsSPRITE *)0x800c082c;
//       lVar6 = 0;
//       NumberImage.u = uVar1;
//       if (GameClock == (GameClock / 0x1e) * 0x1e) {
//         SoundEx((VECTOR *)0x0,0xe);
//       }
//     }
//     else {
//       if (20000 < StrainRatio) {
//         return;
//       }
//       _29 = (GsSPRITE *)&ItemSprite3Ds;
//       NumberImage.w = 4;
//       NumberImage.x = in_a0 + 0x22;
//       NumberImage.y = in_a1 + 8;
//       iVar4 = (20000 - StrainRatio) / 200;
//       do {
//         iVar5 = iVar4 / 10;
//         NumberImage.u = uVar1 + ((char)iVar4 + (char)iVar5 * -10) * '\x04';
//         GsSortSprite(&NumberImage,OTablePt,0);
//         NumberImage.x = NumberImage.x + -6;
//         iVar4 = iVar5;
//         NumberImage.u = uVar1;
//       } while (iVar5 != 0);
//     }
//     iVar5 = -lVar6 + 20000;
//     iVar4 = iVar5;
//     if (iVar5 < 0) {
//       iVar4 = -lVar6 + 0x4e3f;
//     }
//     _29->x = in_a0;
//     _29->y = in_a1;
//     uVar3 = (uint)DAT_80097f68 + (iVar4 >> 5);
//     DAT_80097f68 = (ushort)uVar3;
//     iVar4 = rsin(uVar3 & 0xffff);
//     iVar4 = iVar4 * 0x60;
//     if (iVar4 < 0) {
//       iVar4 = iVar4 + 0xfff;
//     }
//     uVar1 = (char)(iVar4 >> 0xc) + '\x7f';
//     _29->b = uVar1;
//     _29->g = uVar1;
//     _29->r = uVar1;
//     ot = OTablePt;
//     sVar2 = (short)((iVar5 * 0x800) / 20000) + 0x800;
//     _29->scalex = sVar2;
//     _29->scaley = sVar2;
//     GsSortSprite(_29,ot,0);
//   }
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? GsSortSprite(? *, s32, ?, s32);                   /* extern */
// ? SoundEx(s32, ?, s32);                             /* extern */
// s32 rsin(s32);                                      /* extern */
// extern u16 D_80097F68;
// extern ? D_800C082C;
// extern ? D_800C0850;
// extern ? D_800C0874;
// extern s32 GameClock;
// extern ? ItemSprite3Ds;
// extern ? NumberImage;
// extern s32 OTablePt;
// extern s32 StrainRatio;
//
// void PutStrain(s16 arg0, s16 arg1) {
//     ? *var_s4;
//     s16 temp_v0_3;
//     s32 temp_s0;
//     s32 var_a1;
//     s32 var_s2;
//     s32 var_v1;
//     s32 var_v1_2;
//     s8 temp_v0_2;
//     u16 temp_v0;
//     u8 temp_s3;
//
//     var_s2 = StrainRatio;
//     if (var_s2 != 0x7FFFFFFF) {
//         if (var_s2 == 0) {
//             var_s4 = &D_800C0850;
//             goto block_12;
//         }
//         if (var_s2 < -0x4E20) {
//             var_s4 = &D_800C0874;
//             var_s2 = 0;
//             goto block_12;
//         }
//         if (var_s2 < 0) {
//             var_s4 = &D_800C082C;
//             var_s2 = 0;
//             if (GameClock == ((GameClock / 30) * 0x1E)) {
//                 SoundEx(0, 0xE, MULT_HI(GameClock, 0x88888889));
//             }
//             goto block_13;
//         }
//         if (var_s2 < 0x4E21) {
//             var_s4 = &ItemSprite3Ds;
//             NumberImage.unk8 = 4;
//             NumberImage.unk4 = (u16) (arg0 + 0x22);
//             temp_s3 = NumberImage.unkE;
//             NumberImage.unk6 = (s16) (arg1 + 8);
//             var_v1 = (0x4E20 - var_s2) / 200;
//             do {
//                 NumberImage.unkE = (u8) (temp_s3 + ((var_v1 % 10) * 4));
//                 GsSortSprite(&NumberImage, OTablePt, 0, MULT_HI(var_v1, 0x66666667));
//                 var_v1 /= 0xA;
//                 NumberImage.unk4 = (u16) (NumberImage.unk4 - 6);
//             } while (var_v1 != 0);
//             NumberImage.unkE = temp_s3;
// block_12:
// block_13:
//             temp_s0 = 0x4E20 - var_s2;
//             var_v1_2 = temp_s0;
//             if (temp_s0 < 0) {
//                 var_v1_2 = temp_s0 + 0x1F;
//             }
//             var_s4->unk4 = arg0;
//             var_s4->unk6 = arg1;
//             temp_v0 = D_80097F68 + (var_v1_2 >> 5);
//             D_80097F68 = temp_v0;
//             var_a1 = rsin(temp_v0 & 0xFFFF) * 0x60;
//             if (var_a1 < 0) {
//                 var_a1 += 0xFFF;
//             }
//             temp_v0_2 = (var_a1 >> 0xC) + 0x7F;
//             var_s4->unk16 = temp_v0_2;
//             var_s4->unk15 = temp_v0_2;
//             var_s4->unk14 = temp_v0_2;
//             temp_v0_3 = ((temp_s0 << 0xB) / 20000) + 0x800;
//             var_s4->unk1C = temp_v0_3;
//             var_s4->unk1E = temp_v0_3;
//             GsSortSprite(var_s4, OTablePt, 0, MULT_HI((temp_s0 << 0xB), 0x68DB8BAD));
//         }
//     }
// }
