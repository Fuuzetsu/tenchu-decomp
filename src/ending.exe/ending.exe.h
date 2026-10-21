/* ENDING.EXE shares the engine types and the persistent handoff with MAIN.EXE. */
#ifndef ENDING_EXE_H
#define ENDING_EXE_H

#include "../main.exe/main.exe.h"

/* This state survives executable replacement at a fixed protocol address. */
#define ENDING_STATE ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)

/* ENDING retains the earlier 12-byte controller record. MAIN inserts an
 * active flag before fAnalog and uses a different array stride. */
typedef struct EndingPadPort
{
    u16 button;
    s16 x;
    s16 y;
    u8 fAnalog;
    u8 act1;
    u8 act2;
    u8 actbuf[PAD_ACTUATOR_COUNT];
    u8 Send;
} EndingPadPort;

extern EndingPadPort EndingPads[PAD_PORT_COUNT][PAD_SLOTS_PER_PORT];

short PlaySEHigh(SoundEffect *effect, short sound_id, long spatial_value);
void StopSE(short channel);
short FindSETone(SoundEffect *effect, short sound_id);

#endif
