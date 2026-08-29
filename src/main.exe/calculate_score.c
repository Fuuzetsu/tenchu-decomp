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
    s32 stealth_base;
    u8 spots;

    store_result = &STAGE_SCORE_COMPONENTS;
    store_result->criticalScore = stats->criticals * SCORE_PER_CRITICAL;
    store_result->murderScore = stats->murders * SCORE_PER_MURDER;
    store_result->friendPenalty = stats->friendHits * SCORE_PER_FRIEND_HIT;

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
    /* Identical arms on a never-zero test: retail's own dead branch,
     * byte-required (collapsing mismatches; measured). */
    if (stealth_base != 0)
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
    result->grade = score / SCORE_PER_GRADE;
    if (result->grade > GRADE_MAX)
    {
        result->grade = GRADE_MAX;
    }

    if (stats->stageBosses + stats->stageEnemies == 0)
    {
        memset(result, 0, sizeof(*result));
    }
    return result;
}
