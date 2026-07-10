#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct MotionDataType * SearchMotion(short id);
 *     ACTION.C:100, 22 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a1       short id
 *     reg   $a3       struct MotionPackType * mpd
 *     reg   $a2       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionPackType *CommonMotion;
 *     extern struct MotionPackType *PlayerMotion;
 *     extern struct MotionPackType *StageMotion;
 * END PSX.SYM */

/*
 * SearchMotion (0x8001b514, 0x148 bytes) — id lookup across the three fixed
 * motion pools in priority order (common, then player, then stage), each a
 * plain `for (i = 0; i < mpd->n; i++) if (mpd->motion[i]->id == id) return
 * mpd->motion[i];` over item.h's MotionPackType (LoadMotion.c's proven
 * fixed-up-pointer layout) — same short-counter recompute-from-base shape as
 * LoadMotion's relocation loops (cookbook Loops: a short loop counter
 * suppresses strength reduction). `mpd` is reused across all three blocks
 * (one C variable, reassigned), matching PSX.SYM's single-`mpd`/single-`i`
 * local list.
 */
extern MotionPackType *CommonMotion;
extern MotionPackType *PlayerMotion;
extern MotionPackType *StageMotion;

MotionDataType *SearchMotion(short id)
{
    MotionPackType *mpd;
    short i;

    mpd = CommonMotion;
    if (mpd != 0) {
        for (i = 0; i < mpd->n; i++) {
            if (mpd->motion[i]->id == id) {
                return mpd->motion[i];
            }
        }
    }
    mpd = PlayerMotion;
    if (mpd != 0) {
        for (i = 0; i < mpd->n; i++) {
            if (mpd->motion[i]->id == id) {
                return mpd->motion[i];
            }
        }
    }
    mpd = StageMotion;
    if (mpd != 0) {
        for (i = 0; i < mpd->n; i++) {
            if (mpd->motion[i]->id == id) {
                return mpd->motion[i];
            }
        }
    }
    return 0;
}
