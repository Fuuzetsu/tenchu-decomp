#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/Think1sleep", Think1sleep);

/*
 * WIP — everything below is correct EXCEPT gp addressing of its 4 small globals.
 *
 * Done (in main.exe.h): character_state now puts the animation pointer at 0x5C
 * (character_kind/character_status -> 2 bytes, button_mask -> 2 bytes so
 * some_character_button_values is 16), and frames_since_animation_start is s16.
 * With those, the body below compiles to the target's exact arithmetic, branches,
 * struct offsets and li/lh — verified instruction-by-instruction.
 *
 * BLOCKER — gp-relative small globals (Me_THINK_C, SR, Attrib,
 * FRAMES_UNTIL_END_OF_ALERT). The target addresses them via %gp_rel($gp) against
 * *undefined* symbols (resolved to their real addresses at link). We can't yet
 * reproduce that:
 *   - `extern` + `as -G0`  -> absolute (lui/lo), doesn't match.
 *   - tentative def (.comm) -> the symbol is defined in .sbss (which the linker
 *     script discards) at a *sequential* offset, so the reloc is against .sbss
 *     (link error) with the wrong gp offset (4/6/8 vs the real 0x33E/0x344/…).
 *   - `as -G8` (global)     -> gp-addresses EVERY small extern, incl. far ones
 *     like the string FONT_FILE_NAME (reloc-truncated), and shifts section layout
 *     -> breaks everything.
 * The distinction "near-gp vs far" can't come from size alone. The fix is to
 * gp-convert only a WHITELIST of symbols whose address is within ±32 KB of gp
 * (computable from config/symbols), keeping them undefined so the reloc resolves
 * to the absolute address. Cleanest: patch maspsx to accept that whitelist (it
 * already does the %gp_rel rewrite for .comm). Or split .sdata/.sbss symbolically.
 *
 * s32 Think1sleep(void)
 * {
 *     something_about_current_animation *temp_a0;
 *     s32 var_a1;
 *     s32 var_v0;
 *
 *     temp_a0 = Me_THINK_C->something_about_current_animation;
 *     var_a1 = 0;
 *     if (temp_a0->animation_state_perhaps == 0x100)
 *     {
 *         SR = -1;
 *     }
 *     else if (temp_a0->frames_since_animation_start == 0)
 *     {
 *         var_a1 = 0x1001;
 *     }
 *     if ((FRAMES_UNTIL_END_OF_ALERT != 0) || (var_v0 = var_a1 << 0x10, ((Attrib & 0x8000) != 0)))
 *     {
 *         var_v0 = (turn_towards_player_(0, 0) & 0xA000) << 0x10;
 *     }
 *     return var_v0 >> 0x10;
 * }
 */
