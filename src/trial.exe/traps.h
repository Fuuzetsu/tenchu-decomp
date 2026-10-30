#ifndef TENCHU_TRIAL_TRAPS_H
#define TENCHU_TRIAL_TRAPS_H

#include "mission_editor.h"

/* One byte per cell: zero is empty, 0xff is a pressure trigger, and
 * launchers pack a present bit, three height bits and two facing bits. */
enum
{
    TRIAL_TRAP_EMPTY = 0,
    TRIAL_TRAP_TRIGGER = 0xff,
    TRIAL_TRAP_SHOOTER = -128, /* bit 7, sign-extended before packing */
    TRIAL_MAP_COLLAPSING = 0x1000,
    TRIAL_SE_ARROW_LAUNCH = 0x23,
    TRIAL_SE_FLOOR_SINK = 0x26,
    TRIAL_SE_TRAP_TRIGGER = 0x27
};

extern u8 TrialTrapMap[TRIAL_MAP_WIDTH * TRIAL_MAP_HEIGHT];
extern s16 TrialLastTriggerX, TrialLastTriggerZ;

void InitTrialTrapMap(void);
void UpdateTrialArrowTraps(void);
void UpdateTrialCollapsingFloors(void);

#endif
