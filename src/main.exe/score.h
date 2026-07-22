#ifndef TENCHU_SCORE_H
#define TENCHU_SCORE_H

struct ScoreStats;
struct ScoreResult;

extern struct ScoreStats *init_score_stats(struct ScoreStats *stats);
extern struct ScoreResult *calculate_score(struct ScoreStats *stats,
                                           short stage);

#endif
