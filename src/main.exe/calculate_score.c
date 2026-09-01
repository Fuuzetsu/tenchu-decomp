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
 *  - `penalty` is distinct from the default stealth score, giving the
 *    joined subtraction the target's $v0 destination.
 *  - The result pointer is materialized twice (`early`, then `result`
 *    after the fence): byte-required (one pointer recolors the stores;
 *    measured).
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
    ScoreResult *early;
    s32 penalty;
    s32 score;
    s32 stealth_base;
    u8 spots;

    early = &STAGE_SCORE_COMPONENTS;
    early->criticalScore = stats->criticals * SCORE_PER_CRITICAL;
    early->murderScore = stats->murders * SCORE_PER_MURDER;
    early->friendPenalty = stats->friendHits * SCORE_PER_FRIEND_HIT;

    spots = stats->findEnemies;
    stealth_base = STEALTH_BASE;

    if (spots != 0)
    {
        stealth_base = STEALTH_BASE_SEEN;
    }
    /* The medicine-herb stage doubles the per-spot penalty. */
    if (stage == STAGE_CURE_PRINCESS)
    {
        penalty = spots * (SPOT_PENALTY * 2);
    }
    else
    {
        penalty = spots * SPOT_PENALTY;
    }
    penalty = stealth_base - penalty;
    early->spottedScore = penalty;
    /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
    do
    {
    } while (0);
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
    result->grade = score / SCORE_PER_GRADE;
    if (result->grade > RANK_GRAND_MASTER)
    {
        result->grade = RANK_GRAND_MASTER;
    }

    if (stats->stageBosses + stats->stageEnemies == 0)
    {
        memset(result, 0, sizeof(*result));
    }
    return result;
}
