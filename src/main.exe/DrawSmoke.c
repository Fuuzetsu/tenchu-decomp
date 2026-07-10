#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawSmoke(struct tag_EffectSlot *ef);
 *     EFFECT.C:794, 39 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $s3       struct tag_EffectSlot * ef
 *     reg   $s0       struct SmokeType * smoke
 *     reg   $s1       struct Sprite3D * spr
 *     reg   $s2       unsigned char alfa
 *
 * Globals it touches, as the original declared them:
 *     extern struct Sprite3D *sprSmoke;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/DrawSmoke", DrawSmoke);

// triage: MEDIUM — 126 insns, mul/div, 3 callees, ~0.07 to AttackFire
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void DrawSmoke(tag_EffectSlot *ef)
//
// {
//   short sVar1;
//   short sVar2;
//   int iVar3;
//   long lVar4;
//   Sprite3D *sprt;
//   uchar uVar5;
//
//   sprt = (&sprSmoke)[*(byte *)((int)&ef->param + 0x22)];
//   uVar5 = 0x80;
//   if ((ef->param).blood.mode == (ef->param).blood.bright) {
//     if ((ef->param).smoke.vec.vy < -0x14) {
//       (ef->param).smoke.vec.vx = (short)(((ef->param).smoke.vec.vx * 0x50) / 100);
//       sVar1 = (ef->param).smoke.vec.vz;
//       iVar3 = (uint)(ushort)(ef->param).smoke.vec.vy << 0x10;
//       (ef->param).smoke.vec.vy = (short)((iVar3 >> 0x10) - (iVar3 >> 0x1f) >> 1);
//       (ef->param).smoke.vec.vz = (short)((sVar1 * 0x50) / 100);
//     }
//     (ef->param).smoke.scale = (ef->param).smoke.scale + 0x400;
//     iVar3 = rand();
//     (ef->param).blood.bright =
//          ((ef->param).blood.mode + 0xff) - ((char)iVar3 + (char)(iVar3 / 5) * -5);
//   }
//   if ((ef->param).blood.mode < 0x1a) {
//     uVar5 = (ef->param).blood.mode * '\x05';
//   }
//   sVar1 = (ef->param).smoke.vec.vx;
//   (ef->param).blood.pz = (ef->param).blood.pz + (int)(ef->param).smoke.vec.vy;
//   sVar2 = (ef->param).smoke.vec.vz;
//   (ef->param).blood.py = (ef->param).blood.py + (int)sVar1;
//   (ef->param).blood.scale = (ef->param).blood.scale + (int)sVar2;
//   (sprt->locate).coord.t[0] = (ef->param).blood.py;
//   (sprt->locate).coord.t[1] = (ef->param).blood.pz;
//   (sprt->locate).coord.t[2] = (ef->param).blood.scale;
//   sprt->scale = (ef->param).smoke.scale;
//   lVar4 = (ef->param).smoke.rotate;
//   (sprt->sprite).r = uVar5;
//   (sprt->sprite).g = uVar5;
//   (sprt->sprite).b = uVar5;
//   (sprt->sprite).rotate = lVar4;
//   UpdateCoordinate((ModelType *)sprt);
//   DrawSprite(sprt);
//   uVar5 = (ef->param).blood.mode;
//   (ef->param).blood.mode = uVar5 + 0xff;
//   if (uVar5 == '\0') {
//     ef->proc = (undefined **)0x0;
//   }
//   return;
// }
