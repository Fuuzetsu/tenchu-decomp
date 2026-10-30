#include "common.h"
#include "mission_menu.h"
#include "../main.exe/music.h"

/* Narration is in TORA.XA, whose cue namespace begins at 100. */
short GetTrialMissionNarrationCue(void)
{
    short stealth = (TrialMission.header.flags & 15) >> 3;
    short cue;

    switch (TrialMission.header.flags & TRIAL_OBJECTIVE_MASK)
    {
    case TRIAL_REACH_LORD:
        cue = 3;
        if (stealth)
            cue = 4;
        break;
    case TRIAL_ASSASSINATE:
        cue = TrialMission.header.theme * 2 + 5;
        cue += stealth ? 1 : 0;
        break;
    case TRIAL_ELIMINATE_ENEMIES:
        cue = 29;
        if (stealth)
            cue = 31;
        cue += TRIAL_STATE->CharType != 0;
        break;
    case TRIAL_PROTECT_PRINCESS:
        cue = 33;
        if (stealth)
            cue = 34;
        break;
    case TRIAL_FIND_HERB:
        cue = 35;
        if (stealth)
            cue = 36;
        break;
    default:
        cue = -1;
        break;
    }
    if (cue == -1)
        return -1;
    return cue + MUSIC_CUE_TORA_FIRST;
}
