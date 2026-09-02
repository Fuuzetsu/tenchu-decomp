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
