#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void lePackEnemyLayout(void *buf, long size);
 *     WORLD.C:1275, 7 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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
 *     param $a0       void * buf
 *     param $a1       long size
 *
 * Globals it touches, as the original declared them:
 *     extern struct TEnemyLayout enemy[30];
 * END PSX.SYM */

/*
 * lePackEnemyLayout (0x8003caa4, 0x48 bytes) — the save-side counterpart of
 * leRestoreEnemyLayout: if the caller's buffer is big enough (>= sizeof
 * (enemy), 0xFF0), pack (copy) the whole enemy-layout table into it;
 * otherwise report the shortfall via AdtMessageBox and skip the copy.
 * `le`=layout-enemy family (see leResetPath.c for TEnemyLayout, recovered
 * from the Ghidra type export).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - m2c undercounts BOTH calls' arg lists: a register carried in live from
 *    the caller and never overwritten before the call doesn't look like an
 *    "argument" to m2c's basic-block-local view. AdtMessageBox keeps `size`
 *    live in $a1 across the branch and memcpy keeps `buf` live in $a0 the
 *    same way — both need all 3 arguments from Ghidra's rendering, not
 *    m2c's 1-2 arg version.
 */

typedef struct
{
    s16 type;
    s16 ThinkType;
    s16 nPath;
    s32 x;
    s32 y;
    s32 z;
    s16 r;
    s16 pad;
    VECTOR path[7];
} TEnemyLayout; /* 0x88 */

extern TEnemyLayout enemy[0x1E];
extern void AdtMessageBox(char *fmt, ...);
extern void *memcpy(void *s1, void *s2, u32 n);
extern char D_80012174[]; /* "enemy storing size too small %d/%d" */

void lePackEnemyLayout(void *buf, u32 size)
{
    if (size < sizeof(enemy))
    {
        AdtMessageBox(D_80012174, size, sizeof(enemy));
    }
    else
    {
        memcpy(buf, enemy, sizeof(enemy));
    }
}
