#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * load_archive_sprite_ (0x8004f598) — same load/setup shape as initialise_font.c:
 * fetch a TIM (here from an archive, not a file), read its GsIMAGE info,
 * load it, then hand the descriptor to SetupSprite with a null Sprite3D
 * (allocate a fresh sprite) instead of stashing it into a global like
 * load_font_image_into_global does. `adr` survives GetTIMInfo's call in a
 * callee-saved register and feeds LoadTIM directly (matches m2c/Ghidra).
 * This wrapper forwards its own archive/index arguments to
 * get_tim_from_archive. They are already in a0/a1, so the raw assembly needs
 * no argument-setup instructions before the call.
 */
extern Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image);

void load_archive_sprite_(ArcFile *archive, int idx)
{
    GsIMAGE img;
    u_long *adr;

    adr = get_tim_from_archive(archive, idx);
    GetTIMInfo(adr, &img);
    LoadTIM(adr);
    SetupSprite(0, &img);
}
