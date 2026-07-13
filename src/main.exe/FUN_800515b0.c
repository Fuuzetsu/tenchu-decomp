#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

extern GsOT *OTablePt;
extern void GsSortSprite(GsSPRITE *sprite, GsOT *ot, s32 priority);

/* Draw a minutes/seconds time value and optional separator from a digit sprite. */
void FUN_800515b0(GsSPRITE *sprite, s32 time, s32 x, s32 y, s32 drawColon)
{
    s16 value;
    s32 signedValue;
    s16 quotient;
    s32 negative;
    s32 lastNegative;
    s16 multiplier;
    s32 signBase;
    u8 baseU;

    time /= 30;
    value = time / 60;
    sprite->y = y;
    sprite->x = x - 0x20;
    do
    {
        signedValue = (s16)value;
    } while (0);
    negative = 0;
    if (signedValue < 0)
    {
        value = -signedValue;
        negative = 1;
    }

    do
    {
        quotient = (s16)value / 10;
        baseU = sprite->u;
        sprite->u = baseU + ((s16)value % 10) * sprite->w;
        GsSortSprite(sprite, OTablePt, 0);
        value = quotient;
        sprite->u = baseU;
        sprite->x -= 12;
    } while ((quotient << 16) != 0);

    if (negative != 0)
    {
        multiplier = 10;
        signBase = baseU & 0xff;
        sprite->u = signBase + multiplier * sprite->w;
        GsSortSprite(sprite, OTablePt, 0);
        sprite->u = signBase;
    }

    time %= 60;
    sprite->y = y;
    sprite->x = x - 12;
    value = time / 10;
    signedValue = (s16)value;
    negative = 0;
    if (signedValue < 0)
    {
        value = -signedValue;
        negative = 1;
    }

    do
    {
        quotient = (s16)value / 10;
        baseU = sprite->u;
        sprite->u = baseU + ((s16)value % 10) * sprite->w;
        GsSortSprite(sprite, OTablePt, 0);
        value = quotient;
        sprite->u = baseU;
        sprite->x -= 12;
    } while ((quotient << 16) != 0);

    if (negative != 0)
    {
        multiplier = 10;
        signBase = baseU & 0xff;
        sprite->u = signBase + multiplier * sprite->w;
        GsSortSprite(sprite, OTablePt, 0);
        sprite->u = signBase;
    }

    sprite->x = x;
    value = time % 10;
    signedValue = (s16)value;
    do
    {
        sprite->y = y;
    } while (0);
    if (signedValue < 0)
    {
        value = -signedValue;
        lastNegative = 1;
    }
    else
    {
        lastNegative = 0;
    }

    do
    {
        quotient = (s16)value / 10;
        baseU = sprite->u;
        sprite->u = baseU + ((s16)value % 10) * sprite->w;
        GsSortSprite(sprite, OTablePt, 0);
        value = quotient;
        sprite->u = baseU;
        sprite->x -= 12;
    } while ((quotient << 16) != 0);

    if (lastNegative != 0)
    {
        multiplier = 10;
        signBase = baseU & 0xff;
        sprite->u = signBase + multiplier * sprite->w;
        GsSortSprite(sprite, OTablePt, 0);
        sprite->u = signBase;
    }

    if (drawColon != 0)
    {
        multiplier = 11;
        signBase = sprite->u;
        sprite->u = signBase + multiplier * sprite->w;
        sprite->x = x - 0x16;
        GsSortSprite(sprite, OTablePt, 0);
        sprite->u = signBase;
    }
}

// triage: HARD — 259 insns, mul/div, 3 loop, 1 callees, ~0.07 to BriefingAndInventorySelectionScreen
// likely-relevant cookbook sections:
//   - Loops: 3 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_800515b0(GsSPRITE *param_1,int param_2,short param_3,short param_4,int param_5)
//
// {
//   uchar uVar1;
//   bool bVar2;
//   GsOT *ot;
//   short sVar3;
//   int iVar4;
//   uint uVar5;
//   int iVar6;
//
//   param_1->y = param_4;
//   param_1->x = param_3 + -0x20;
//   uVar5 = (param_2 / 0x1e) / 0x3c;
//   iVar4 = (int)(uVar5 * 0x10000) >> 0x10;
//   bVar2 = false;
//   if (iVar4 < 0) {
//     uVar5 = -iVar4;
//     bVar2 = true;
//   }
//   do {
//     sVar3 = (short)uVar5;
//     uVar5 = (int)sVar3 / 10;
//     uVar1 = param_1->u;
//     param_1->u = uVar1 + (char)((uint)(((int)sVar3 % 10) * 0x10000) >> 0x10) * (char)param_1->w;
//     GsSortSprite(param_1,OTablePt,0);
//     param_1->u = uVar1;
//     param_1->x = param_1->x + -0xc;
//   } while ((uVar5 & 0xffff) != 0);
//   if (bVar2) {
//     param_1->u = uVar1 + (char)param_1->w * '\n';
//     GsSortSprite(param_1,OTablePt,0);
//     param_1->u = uVar1;
//   }
//   iVar6 = (param_2 / 0x1e) % 0x3c;
//   param_1->y = param_4;
//   param_1->x = param_3 + -0xc;
//   uVar5 = iVar6 / 10;
//   iVar4 = (int)(uVar5 * 0x10000) >> 0x10;
//   bVar2 = false;
//   if (iVar4 < 0) {
//     uVar5 = -iVar4;
//     bVar2 = true;
//   }
//   do {
//     sVar3 = (short)uVar5;
//     uVar5 = (int)sVar3 / 10;
//     uVar1 = param_1->u;
//     param_1->u = uVar1 + (char)((uint)(((int)sVar3 % 10) * 0x10000) >> 0x10) * (char)param_1->w;
//     GsSortSprite(param_1,OTablePt,0);
//     param_1->u = uVar1;
//     param_1->x = param_1->x + -0xc;
//   } while ((uVar5 & 0xffff) != 0);
//   if (bVar2) {
//     param_1->u = uVar1 + (char)param_1->w * '\n';
//     GsSortSprite(param_1,OTablePt,0);
//     param_1->u = uVar1;
//   }
//   param_1->x = param_3;
//   uVar5 = iVar6 % 10;
//   iVar4 = (int)(uVar5 * 0x10000) >> 0x10;
//   param_1->y = param_4;
//   if (iVar4 < 0) {
//     uVar5 = -iVar4;
//     bVar2 = true;
//   }
//   else {
//     bVar2 = false;
//   }
//   do {
//     sVar3 = (short)uVar5;
//     uVar5 = (int)sVar3 / 10;
//     uVar1 = param_1->u;
//     param_1->u = uVar1 + (char)((uint)(((int)sVar3 % 10) * 0x10000) >> 0x10) * (char)param_1->w;
//     GsSortSprite(param_1,OTablePt,0);
//     param_1->u = uVar1;
//     param_1->x = param_1->x + -0xc;
//   } while ((uVar5 & 0xffff) != 0);
//   if (bVar2) {
//     param_1->u = uVar1 + (char)param_1->w * '\n';
//     GsSortSprite(param_1,OTablePt,0);
//     param_1->u = uVar1;
//   }
//   if (param_5 != 0) {
//     uVar1 = param_1->u;
//     param_1->u = uVar1 + (char)param_1->w * '\v';
//     ot = OTablePt;
//     param_1->x = param_3 + -0x16;
//     GsSortSprite(param_1,ot,0);
//     param_1->u = uVar1;
//   }
//   return;
// }
