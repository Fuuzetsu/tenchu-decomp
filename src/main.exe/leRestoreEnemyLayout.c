#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void leRestoreEnemyLayout(void *buf);
 *     WORLD.C:1286, 2 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       void * buf
 *
 * Globals it touches, as the original declared them:
 *     extern struct TEnemyLayout enemy[30];
 * END PSX.SYM */

/*
 * leRestoreEnemyLayout (0x8003ca78, 0x2c bytes) — bulk-restore the whole
 * enemy-layout table from a caller-supplied buffer (the inverse of a save;
 * `le`=layout-enemy family, see leResetPath.c for TEnemyLayout, recovered
 * from the Ghidra type export). sizeof(enemy) == 0x1e * 0x88 == 0xFF0,
 * matching the memcpy length exactly (free at compile time).
 */

extern void *memcpy(void *s1, void *s2, u32 n);

void leRestoreEnemyLayout(void *buf)
{
    memcpy(enemy, buf, sizeof(enemy));
}
