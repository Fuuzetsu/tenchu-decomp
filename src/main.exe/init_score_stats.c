#include "common.h"
#include "main.exe.h"
#include "score.h"

/*
 * init_score_stats (0x8004e964) — placeholder name (not from Ghidra/Ghidra
 * export; this function isn't in the export at all, no decomp.me reference
 * either — described purely from the raw .s + m2c). Populates an end-of-
 * stage score/stats record from the low bytes of six per-run halfword
 * counters. The explicit `u8 *` views preserve retail's `lbu` snapshots
 * without lying about the globals' PSX.SYM-proven widths, followed by a
 * GameClock tick count clamped to 0x1A5C2 (the only absolute/non-gp global
 * here — GameClock is
 * defined in the think/DoInfoViewProc TU, see item.h's gp note). Returns
 * the same pointer it was given (arg0 survives in $v0 via the jr delay
 * slot, no separate load).
 * gp: StageBosses/StageEnemies/Findenemies/Murders/Criticals/FriendHits
 * added via `tools/gpsyms.py init_score_stats --write`.
 */
ScoreStats *init_score_stats(ScoreStats *stats)
{
    s32 clock;

    stats->stageBosses = *(u8 *)&StageBosses;
    stats->stageEnemies = *(u8 *)&StageEnemies;
    stats->findEnemies = *(u8 *)&Findenemies;
    stats->murders = *(u8 *)&Murders;
    stats->criticals = *(u8 *)&Criticals;
    stats->friendHits = *(u8 *)&FriendHits;
    clock = GameClock;
    if (clock > 0x1A5C2)
    {
        clock = 0x1A5C2;
    }
    stats->clock = clock;
    return stats;
}
