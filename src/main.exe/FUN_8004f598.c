#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * FUN_8004f598 (0x8004f598) — same load/setup shape as initialise_font.c:
 * fetch a TIM (here from an archive, not a file), read its GsIMAGE info,
 * load it, then hand the descriptor to SetupSprite with a null Sprite3D
 * (allocate a fresh sprite) instead of stashing it into a global like
 * load_font_image_into_global does. `adr` survives GetTIMInfo's call in a
 * callee-saved register and feeds LoadTIM directly (matches m2c/Ghidra).
 * get_tim_from_archive is called with NO arguments in this TU — the raw
 * `.s` has no a0/a1 setup before the `jal`, unlike
 * BriefingAndInventorySelectionScreen.c's 2-arg prototype for the same
 * extern (per-TU prototype disagreement, see docs/matching-cookbook.md).
 */
extern u_long *get_tim_from_archive(void);
extern void LoadTIM(u_long *adr);
extern Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image);

void FUN_8004f598(void)
{
    GsIMAGE img;
    u_long *adr;

    adr = get_tim_from_archive();
    GetTIMInfo(adr, &img);
    LoadTIM(adr);
    SetupSprite(0, &img);
}

// triage: TRIVIAL — 17 insns, 4 callees, ~0.58 to initialise_font

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_8004f598(void)
//
// {
//   ulong *adr;
//   GsIMAGE GStack_28;
//
//   adr = (ulong *)FUN_8004f1d8();
//   GetTIMInfo(adr,&GStack_28);
//   LoadTIM(adr);
//   SetupSprite((Sprite3D *)0x0,&GStack_28);
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? GetTIMInfo(s32, ? *);                             /* extern */
// ? LoadTIM(s32);                                     /* extern */
// ? SetupSprite(?, ? *);                              /* extern */
// s32 get_tim_from_archive();                         /* extern */
//
// void FUN_8004f598(void) {
//     ? sp10;
//     s32 temp_v0;
//
//     temp_v0 = get_tim_from_archive();
//     GetTIMInfo(temp_v0, &sp10);
//     LoadTIM(temp_v0);
//     SetupSprite(0, &sp10);
// }
