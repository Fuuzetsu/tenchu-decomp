#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PadShockAR(int port, int pow, int attack, int release);
 *     PADCMD.C:241, 5 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
