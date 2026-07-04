#include "common.h"
#include "main.exe.h"

/*
 * load_font_image_into_global (0x8005764c) — stashes the font's GsIMAGE
 * descriptor (returned by GetTIMInfo, see initialise_font.c) into the global
 * FONT_IMAGE_ so later font-drawing code can reuse it. A plain 28-byte
 * (7-word) struct assignment: cc1 fully unrolls it (2 groups of 3 words +
 * a trailing word, `lw`x3/`sw`x3 each) rather than looping — small enough
 * that emit_block_move just unrolls. FONT_IMAGE_ is >8 bytes (non-small), so
 * it addresses via a plain %hi/%lo pair, no gp-extern needed.
 */

extern GsIMAGE FONT_IMAGE_;

void load_font_image_into_global(GsIMAGE *image)
{
    FONT_IMAGE_ = *image;
}
