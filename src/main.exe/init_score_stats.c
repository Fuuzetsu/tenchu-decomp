#include "common.h"
#include "main.exe.h"
#include "score.h"

ScoreStats *init_score_stats(ScoreStats *stats)
{
    s32 clock;

    stats->stageBosses = (u8)StageBosses;
    stats->stageEnemies = (u8)StageEnemies;
    stats->findEnemies = (u8)Findenemies;
    stats->murders = (u8)Murders;
    stats->criticals = (u8)Criticals;
    stats->friendHits = (u8)FriendHits;
    clock = GameClock;
    if (clock > SCORE_CLOCK_MAX)
    {
        clock = SCORE_CLOCK_MAX;
    }
    stats->clock = clock;
    return stats;
}
