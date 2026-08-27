#include "common.h"
#include "main.exe.h"
#include "score.h"

/*
 * calculate_score (0x8004e794, 0x160 bytes) builds the six halfword
 * end-of-stage score components and returns their static result record.
 *
 * Matching notes:
 *  - Friendly-fire, spotted, and total scores are signed values. The
 *    explicit unsigned view of the spotted component in the sum preserves
 *    the retail halfword load and its modulo-16-bit accumulation.
 *  - The empty one-shot loop between the 400-point default and its override
 *    is a zero-code allocation boundary that puts the hidden count in $a2
 *    and the default score in $a3.
 *  - `penalty` is distinct from that default score, giving the joined
 *    subtraction the target's $v0 destination.
 *  - Duplicating the spotted-score store into identical arms is a zero-code
 *    CFG fence after jump2.  It makes the following signed check reload the
 *    halfword, while the direct first-component read preserves the original
 *    shared high-address register.
 */
extern ScoreResult STAGE_SCORE_COMPONENTS;
extern void *memset(void *s, s32 c, u32 n);

ScoreResult *calculate_score(ScoreStats *stats, s16 stage)
{
    ScoreResult *result;
    ScoreResult *store_result;
    s32 penalty;
    s32 score;
    s32 spotted;
    u8 hidden;

    store_result = &STAGE_SCORE_COMPONENTS;
    store_result->criticalScore = stats->criticals * 20;
    store_result->murderScore = stats->murders * 5;
    store_result->friendPenalty = stats->friendHits * -30;

    hidden = stats->findEnemies;
    spotted = 400;

    if (hidden != 0)
    {
        spotted = 300;
    }
    if (stage == 8)
    {
        penalty = hidden * 40;
    }
    else
    {
        penalty = hidden * 20;
    }
    penalty = spotted - penalty;
    if (spotted != 0)
    {
        store_result->spottedScore = penalty;
    }
    else
    {
        store_result->spottedScore = penalty;
    }
    result = &STAGE_SCORE_COMPONENTS;
    if (result->spottedScore < 0)
    {
        result->spottedScore = 0;
    }

    result->score = STAGE_SCORE_COMPONENTS.criticalScore + result->murderScore +
                    result->friendPenalty + (u16)result->spottedScore;
    score = result->score;
    if (score < 0)
    {
        score = 0;
    }
    result->grade = score / 100;
    if (result->grade > 4)
    {
        result->grade = 4;
    }

    if (stats->stageBosses + stats->stageEnemies == 0)
    {
        memset(result, 0, sizeof(*result));
    }
    return result;
}
