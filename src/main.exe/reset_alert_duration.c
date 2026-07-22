#include "common.h"
#include "main.exe.h"

/*
 * reset_alert_duration (0x8002f7f4, 0x24 bytes) — (re)starts the
 * alert-state countdown: 300 frames normally, 600 on DIFFICULTY_HARD.
 * Part of the original "think" TU (same as Think1sleep.c): it gp-addresses
 * EmergencyNotice, already declared extern in main.exe.h.
 *
 * gNannido is read via absolute (non-gp) addressing — Ghidra calls it
 * PersistentState._88_1_ (decimal offset 88 = 0x58 from the 0x80010000
 * persistent-state blob). Identified as the difficulty byte: demo PSX.SYM
 * global gNannido / TLinkInfo.Nannido (see game_types.h game_difficulty);
 * TLinkInfo.Nannido is the struct view of the same byte.
 */


void reset_alert_duration(void)
{
    s32 duration;

    duration = 300;
    if (gNannido == DIFFICULTY_HARD)
    {
        duration = 600;
    }
    EmergencyNotice = duration;
}
