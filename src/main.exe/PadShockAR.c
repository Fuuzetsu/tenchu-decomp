#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PadShockAR(int port, int pow, int attack, int release);
 *     PADCMD.C:241, 5 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int port
 *     param $a1       int pow
 *     param $a2       int attack
 *     param $a3       int release
 *
 * Globals it touches, as the original declared them:
 *     extern struct PADCMD__141fake PadArrange;
 * END PSX.SYM */

/* Ghidra: struct PadArrange { pow, time, attack, release; } — a pad rumble
 * (shock) envelope: power level + attack/release ramp times, time is a
 * running counter reset here. */

void PadShockAR(int port, int pow, int attack, int release)
{
    PadArrange.time = 0;
    PadArrange.pow = pow;
    PadArrange.attack = attack;
    PadArrange.release = release;
}
