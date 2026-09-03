#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void jt_init4(void);
 *     WORLD.C:1329, 110 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 * END PSX.SYM */

void jt_init4(void)
{
    GsFCALL4.f3[0][0] = dmyGsTMDfastF3L;
    GsFCALL4.f3[0][1] = dmyGsTMDfastF3LFG;
    GsFCALL4.f3[0][2] = dmyGsTMDfastF3NL;
    GsFCALL4.f3[1][0] = dmyGsTMDfastF3L;
    GsFCALL4.f3[1][1] = dmyGsTMDfastF3LFG;
    GsFCALL4.f3[1][2] = dmyGsTMDfastF3NL;
    GsFCALL4.nf3[0] = GsTMDfastNF3;
    GsFCALL4.nf3[1] = GsA4divNF3;

    GsFCALL4.g3[0][0] = dmyGsTMDfastG3L;
    GsFCALL4.g3[0][1] = dmyGsTMDfastG3LFG;
    GsFCALL4.g3[0][2] = dmyGsTMDfastG3NL;
    GsFCALL4.g3[1][0] = dmyGsTMDfastG3L;
    GsFCALL4.g3[1][1] = dmyGsTMDfastG3LFG;
    GsFCALL4.g3[1][2] = dmyGsTMDfastG3NL;
    GsFCALL4.ng3[0] = GsTMDfastNG3;
    GsFCALL4.ng3[1] = GsA4divNG3;

    GsFCALL4.tf3[0][0] = GsTMDfastTF3L;
    GsFCALL4.tf3[0][1] = GsTMDfastTF3LFG;
    GsFCALL4.tf3[0][2] = dmyGsTMDfastTF3NL;
    GsFCALL4.tf3[1][0] = GsTMDfastTF3L;
    GsFCALL4.tf3[1][1] = GsTMDfastTF3LFG;
    GsFCALL4.tf3[1][2] = dmyGsTMDfastTF3NL;
    GsFCALL4.ntf3[0] = GsTMDfastTNF3;
    GsFCALL4.ntf3[1] = GsA4divTNF3;

    GsFCALL4.tg3[0][0] = dmyGsTMDfastTG3L;
    GsFCALL4.tg3[0][1] = dmyGsTMDfastTG3LFG;
    GsFCALL4.tg3[0][2] = dmyGsTMDfastTG3NL;
    GsFCALL4.tg3[1][0] = dmyGsTMDfastTG3L;
    GsFCALL4.tg3[1][1] = dmyGsTMDfastTG3LFG;
    GsFCALL4.tg3[1][2] = dmyGsTMDfastTG3NL;
    GsFCALL4.ntg3[0] = GsTMDfastTNG3;
    GsFCALL4.ntg3[1] = GsA4divTNG3;

    GsFCALL4.f4[0][0] = dmyGsTMDfastF4L;
    GsFCALL4.f4[0][1] = dmyGsTMDfastF4LFG;
    GsFCALL4.f4[0][2] = dmyGsTMDfastF4NL;
    GsFCALL4.f4[1][0] = dmyGsTMDfastF4L;
    GsFCALL4.f4[1][1] = dmyGsTMDfastF4LFG;
    GsFCALL4.f4[1][2] = dmyGsTMDfastF4NL;
    GsFCALL4.nf4[0] = GsTMDfastNF4;
    GsFCALL4.nf4[1] = GsA4divNF4;

    GsFCALL4.g4[0][0] = dmyGsTMDfastG4L;
    GsFCALL4.g4[0][1] = dmyGsTMDfastG4LFG;
    GsFCALL4.g4[0][2] = dmyGsTMDfastG4NL;
    GsFCALL4.g4[1][0] = dmyGsTMDfastG4L;
    GsFCALL4.g4[1][1] = dmyGsTMDfastG4LFG;
    GsFCALL4.g4[1][2] = dmyGsTMDfastG4NL;
    GsFCALL4.ng4[0] = GsTMDfastNG4;
    GsFCALL4.ng4[1] = GsA4divNG4;

    GsFCALL4.tf4[0][0] = dmyGsTMDfastTF4L;
    GsFCALL4.tf4[0][1] = dmyGsTMDfastTF4LFG;
    GsFCALL4.tf4[0][2] = dmyGsTMDfastTF4NL;
    GsFCALL4.tf4[1][0] = dmyGsTMDfastTF4L;
    GsFCALL4.tf4[1][1] = dmyGsTMDfastTF4LFG;
    GsFCALL4.tf4[1][2] = dmyGsTMDfastTF4NL;
    GsFCALL4.ntf4[0] = GsTMDfastTNF4;
    GsFCALL4.ntf4[1] = GsA4divTNF4;

    GsFCALL4.tg4[0][0] = dmyGsTMDfastTG4L;
    GsFCALL4.tg4[0][1] = dmyGsTMDfastTG4LFG;
    GsFCALL4.tg4[0][2] = dmyGsTMDfastTG4NL;
    GsFCALL4.tg4[1][0] = dmyGsTMDfastTG4L;
    GsFCALL4.tg4[1][1] = dmyGsTMDfastTG4LFG;
    GsFCALL4.tg4[1][2] = dmyGsTMDfastTG4NL;
    GsFCALL4.ntg4[0] = GsTMDfastTNG4;
    GsFCALL4.ntg4[1] = GsA4divTNG4;

    GsADIVZ = 0x25A;
    GsADIVW = 0x200;
    GsADIVH = 0x200;
}
