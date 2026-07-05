#include "common.h"
#include "main.exe.h"

/*
 * DisposeSE (0x80018da0) — silence all voices, close the VAB, then free a
 * sound-effect object's VAB header buffer and the SoundEffect itself. Same
 * null-check-then-free shape as DisposeBG/DisposeAfterimage, plus an extra
 * unconditional SsUtAllKeyOff(0)/SsVabClose(se->VABid) pair before the frees
 * (se survives all four calls in a callee-saved register). Local struct
 * matches Ghidra's real SoundEffect exactly (VABid@0 s16, program@2 s16
 * unused here, VABhead@4 pointer) — no truncation needed, it's the whole type.
 */
extern void SsUtAllKeyOff(s32 flag);
extern void SsVabClose(s16 vabId);
extern void vfree(void *p);

typedef struct
{
    s16 VABid;
    s16 program;
    void *VABhead;
} SoundEffect;

void DisposeSE(SoundEffect *se)
{
    if (se != 0)
    {
        SsUtAllKeyOff(0);
        SsVabClose(se->VABid);
        vfree(se->VABhead);
        vfree(se);
    }
}

// triage: TRIVIAL — 19 insns, 3 callees, ~0.50 to DisposeBG

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void DisposeSE(SoundEffect *se)
//
// {
//   if (se != (SoundEffect *)0x0) {
//     SsUtAllKeyOff(0);
//     SsVabClose(se->VABid);
//     vfree((undefined *)se->VABhead);
//     vfree((undefined *)se);
//   }
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? SsUtAllKeyOff(?);                                 /* extern */
// ? SsVabClose(s16);                                  /* extern */
// ? vfree(void *);                                    /* extern */
//
// void DisposeSE(void *arg0) {
//     if (arg0 != NULL) {
//         SsUtAllKeyOff(0);
//         SsVabClose(arg0->unk0);
//         vfree(arg0->unk4);
//         vfree(arg0);
//     }
// }
