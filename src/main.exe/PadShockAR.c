#include "common.h"
#include "main.exe.h"

/* Ghidra: struct PadArrange { pow, time, attack, release; } — a pad rumble
 * (shock) envelope: power level + attack/release ramp times, time is a
 * running counter reset here. */
typedef struct
{
    s32 pow;     /* 0x0 */
    s32 time;    /* 0x4 */
    s32 attack;  /* 0x8 */
    s32 release; /* 0xC */
} PadArrangeStruct;

extern PadArrangeStruct PadArrange;

void PadShockAR(int port, int pow, int attack, int release)
{
    PadArrange.time = 0;
    PadArrange.pow = pow;
    PadArrange.attack = attack;
    PadArrange.release = release;
}
