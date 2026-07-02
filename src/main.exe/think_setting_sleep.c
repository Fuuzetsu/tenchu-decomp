#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/think_setting_sleep", think_setting_sleep_);

/*
 * WIP. The body below (from m2c) is functionally correct and, with maspsx now in
 * the pipeline, compiles VERY close to the target. Remaining, precisely
 * identified blockers before it byte-matches (see docs/toolchain.md "gp globals"):
 *
 *  1. gp addressing: CHARACTER_BEING_UPDATED_, ALERT_STATUS_,
 *     ACTUALLY_ALERT_STATUS_, FRAMES_UNTIL_END_OF_ALERT must be $gp-relative in
 *     the output (target uses %gp_rel). maspsx only gp-converts symbols it sees
 *     as small-data (.comm/.sdata/.sbss), NOT `.extern`. Fix: declare these
 *     globals as tentative definitions (drop `extern` in main.exe.h) so cc1 emits
 *     `.comm SYM,size`; maspsx then rewrites the loads/stores to %gp_rel and the
 *     linker resolves the symbol to its real address via undefined_symbols_auto.
 *  2. character_state layout: `something_about_current_animation` must sit at
 *     0x5C but currently lands at 0x70 — a 20-byte drift from oversized types
 *     (all contradict the field-offset annotations): character_kind and
 *     character_status are 4-byte enums (should be 2 bytes each) and
 *     some_character_button_values is 32 bytes because button_mask is 4 bytes
 *     (should be 2, making the struct 16). Fix those three type sizes.
 *  3. something_about_current_animation.frames_since_animation_start must be s16
 *     (target uses `lh`), not u16 (`lhu`).
 *  4. After 1-3, any residual register-allocation diff is a decomp-permuter job
 *     (as with get_held_buttons).
 *
 * s32 think_setting_sleep_(void)
 * {
 *     something_about_current_animation *temp_a0;
 *     s32 var_a1;
 *     s32 var_v0;
 *
 *     temp_a0 = CHARACTER_BEING_UPDATED_->something_about_current_animation;
 *     var_a1 = 0;
 *     if (temp_a0->animation_state_perhaps == 0x100)
 *     {
 *         ALERT_STATUS_ = -1;
 *     }
 *     else if (temp_a0->frames_since_animation_start == 0)
 *     {
 *         var_a1 = 0x1001;
 *     }
 *     if ((FRAMES_UNTIL_END_OF_ALERT != 0) || (var_v0 = var_a1 << 0x10, ((ACTUALLY_ALERT_STATUS_ & 0x8000) != 0)))
 *     {
 *         var_v0 = (turn_towards_player_(0, 0) & 0xA000) << 0x10;
 *     }
 *     return var_v0 >> 0x10;
 * }
 */
