#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/think_setting_sleep", think_setting_sleep_);

/*
s32 think_setting_sleep_(void)
{
    something_about_current_animation *temp_a0;
    s32 var_a1;
    s32 var_v0;

    temp_a0 = CHARACTER_BEING_UPDATED_->something_about_current_animation;
    var_a1 = 0;
    if (temp_a0->animation_state_perhaps == 0x100)
    {
        ALERT_STATUS_ = -1;
    }
    else if (temp_a0->frames_since_animation_start == 0)
    {
        var_a1 = 0x1001;
    }
    if ((FRAMES_UNTIL_END_OF_ALERT != 0) || (var_v0 = var_a1 << 0x10, ((ACTUALLY_ALERT_STATUS_ & 0x8000) != 0)))
    {
        var_v0 = (turn_towards_player_(0, 0) & 0xA000) << 0x10;
    }
    return var_v0 >> 0x10;
}
*/