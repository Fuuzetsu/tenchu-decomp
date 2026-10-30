#ifndef TRIAL_TELOP_H
#define TRIAL_TELOP_H

#include "common.h"

typedef s16 TrialTelopStyle;
enum trial_telop_style
{
    TRIAL_TELOP_WHITE,
    TRIAL_TELOP_DIM,
    TRIAL_TELOP_BLUE,
    TRIAL_TELOP_GREEN,
    TRIAL_TELOP_RED,
    TRIAL_TELOP_YELLOW,
    TRIAL_TELOP_STYLE_COUNT
};

void SetupTrialColoredTelop(u8 *text, short line, TrialTelopStyle style);

#endif
