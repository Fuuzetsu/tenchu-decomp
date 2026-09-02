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
 *  - Each stage branch writes the final spotted score using its own penalty
 *    rate. Factoring out that store lets the compiler retain the full-width
 *    value instead of reloading the stored halfword before the clamp.
 *  - The first four components are written directly; `result` begins at the
 *    point where the completed record is read, clamped, and returned.
 */
extern ScoreResult STAGE_SCORE_COMPONENTS;
extern void *memset(void *s, s32 c, u32 n);

ScoreResult *calculate_score(ScoreStats *stats, packed_stage_id stage)
{
    ScoreResult *result;
    s32 score;
    s32 stealth_base;
    u8 spots;

    STAGE_SCORE_COMPONENTS.criticalScore = stats->criticals * SCORE_PER_CRITICAL;
    STAGE_SCORE_COMPONENTS.murderScore = stats->murders * SCORE_PER_MURDER;
    STAGE_SCORE_COMPONENTS.friendPenalty = stats->friendHits * SCORE_PER_FRIEND_HIT;

    spots = stats->findEnemies;
    stealth_base = STEALTH_BASE;

    if (spots != 0)
    {
        stealth_base = STEALTH_BASE_SEEN;
    }
    /* Training doubles the per-spot penalty.  `stage` is a StageConfig id,
     * not the campaign uid whose value 8 names Cure the Princess. */
    if (stage == STAGE_ID_TRAINING)
    {
        STAGE_SCORE_COMPONENTS.spottedScore =
            stealth_base - spots * (SPOT_PENALTY * 2);
    }
    else
    {
        STAGE_SCORE_COMPONENTS.spottedScore = stealth_base - spots * SPOT_PENALTY;
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
