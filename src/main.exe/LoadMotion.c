#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct MotionPackType * LoadMotion(unsigned long *data);
 *     ACTION.C:77, 19 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     param $a0       unsigned long * data
 *     reg   $s0       struct MotionPackType * mpd
 *     reg   $a1       struct MotionDataType * mmp
 *     reg   $a3       short i
 *     reg   $a2       short j
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionPackType *MotionPack;
 * END PSX.SYM */

/*
 * LoadMotion (0x8001c2c0, 0xf0 bytes) — fixup pass for a freshly-loaded
 * motion-pack blob. Every MotionPackType.motion[] entry starts life as a
 * pack-relative on-disk byte OFFSET and is rewritten in place into a real
 * MotionDataType pointer by adding the pack base; within each MotionDataType,
 * .locate and every .rotate[] entry get the same offset -> pointer fixup,
 * relative to THAT MotionDataType's own (already-fixed-up) address — same
 * on-disk-offset idiom as LoadAreaMap.c's `index` fixup.
 *
 * Matching notes: `if (mpd == 0) SystemOut(...)` has no early return — Ghidra
 * flags SystemOut noreturn, but the asm falls straight through into the fixup
 * loop reading through the null pointer on that path (LoadAreaMap.c's
 * identical SystemOut-then-continue shape). Both `mmp->n != 0` tests are the
 * SAME source condition: the second is just the natural entry-duplicated
 * bound check of `for (j = 0; j < mmp->n; j++)` (cookbook Loops), not a
 * second nested if.
 */
extern MotionPackType *MotionPack;
extern void SystemOut(char *);
extern char D_80011230[]; /* "NO MOTION DATA" */

MotionPackType *LoadMotion(unsigned long *data)
{
    MotionPackType *mpd;
    MotionDataType *mmp;
    short i;
    short j;

    mpd = (MotionPackType *)data;
    if (mpd == 0) {
        SystemOut(D_80011230);
    }
    for (i = 0; i < mpd->n; i++) {
        mpd->motion[i] = (MotionDataType *)((s32)mpd->motion[i] + (s32)mpd);
        mmp = mpd->motion[i];
        if (mmp->n != 0) {
            mmp->locate = (MotionElementType *)((s32)mmp->locate + (s32)mmp);
            for (j = 0; j < mmp->n; j++) {
                mmp->rotate[j] = (MotionElementType *)((s32)mmp->rotate[j] + (s32)mmp);
            }
        }
    }
    MotionPack = mpd;
    return mpd;
}
