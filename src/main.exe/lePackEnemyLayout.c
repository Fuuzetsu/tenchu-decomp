#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void lePackEnemyLayout(void *buf, long size);
 *     WORLD.C:1275, 7 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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

extern void AdtMessageBox(char *fmt, ...);
extern void *memcpy(void *s1, void *s2, u32 n);
extern char fmt_enemy_storing_size_too[]; /* enemy storing size too small %d/%d */

void lePackEnemyLayout(void *buf, long size)
{
    if (size < sizeof(enemy))
    {
        AdtMessageBox(fmt_enemy_storing_size_too, size, sizeof(enemy));
    }
    else
    {
        memcpy(buf, enemy, sizeof(enemy));
    }
}
