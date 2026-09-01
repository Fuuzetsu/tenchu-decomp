#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void leResetPath(int id);
 *     WORLD.C:1293, 6 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int id
 *
 * Globals it touches, as the original declared them:
 *     extern struct TEnemyLayout enemy[30];
 * END PSX.SYM */

/*
 * leResetPath (0x8003ca4c, 0x2c bytes) — clears the path point-count on one
 * slot of the enemy-layout table (debug menu "path layout > reset path").
 *
 * enemy is TEnemyLayout[0x1e] (0x88/entry: type/ThinkType/nPath, x/y/z,
 * r/pad, VECTOR path[7] — full layout from the Ghidra type export, size
 * matches the asm's id*0x88 stride exactly). Only nPath (s16 @ offset 4) is
 * touched here.
 */

void leResetPath(enemy_layout_index id)
{
    if ((u32)id < MAX_ENEMIES)
    {
        enemy[id].nPath = 0;
    }
}
