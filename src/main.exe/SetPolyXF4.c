#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetPolyXF4(struct POLY_XF4 *ply, short attrib);
 *     EFFECT.C:1770, 5 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct POLY_XF4 * ply
 *     param $a1       short attrib
 * END PSX.SYM */

/*
 * SetPolyXF4 (0x80038e24) — GPU primitive-packet setup, PsyQ-macro-style
 * (a libgpu POLY_F4 quad preceded by a DR_TPAGE mode-change packet in the
 * same combined packet). Ghidra's type library already reconstructs
 * POLY_XF4/DR_TPAGE/POLY_F4 (real PsyQ SDK structs) but they aren't in a
 * shared header yet — defined locally here (PadShockAR.c's local
 * PadArrangeStruct is the precedent for a one-off type); promote to
 * libgs.h if a second caller needs them.
 *
 *  - `ply->ply.tag`'s top byte (offset+3) is the PsyQ `setlen` length field,
 *    set to 5; `.code` is the primitive code byte, set to '*' (0x2A).
 *  - `ply->tpage.tag`'s top byte set to 1 (setlen), and `.code[0]` packed as
 *    `((attrib & 3) << 5) | 0xE1000200` — a DR_TPAGE-style mode word (0xE1
 *    = draw-mode GPU command, with the low tpage bits ORed in).
 */
typedef struct
{
    u_long tag;
    u_long code[1];
} DR_TPAGE;

typedef struct
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    short x1, y1;
    short x2, y2;
    short x3, y3;
} POLY_F4;

typedef struct
{
    DR_TPAGE tpage;
    POLY_F4 ply;
} POLY_XF4;

void SetPolyXF4(POLY_XF4 *ply, short attrib)
{
    *((u_char *)&ply->ply.tag + 3) = 5;
    ply->ply.code = '*';
    *((u_char *)&ply->tpage.tag + 3) = 1;
    ply->tpage.code[0] = ((attrib & 3) << 5) | 0xE1000200;
}
