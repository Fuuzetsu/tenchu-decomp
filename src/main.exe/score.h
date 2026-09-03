#ifndef TENCHU_SCORE_H
#define TENCHU_SCORE_H

struct ScoreStats;
struct ScoreResult;

extern struct ScoreStats *init_score_stats(struct ScoreStats *stats);
extern struct ScoreResult *calculate_score(struct ScoreStats *stats,
                                           packed_stage_id stage);
extern void draw_time_(GsSPRITE *digits, s32 time, s32 x, s32 y,
                       s32 draw_colon);
extern void award_stage_items_(TLinkInfo *state, ScoreResult *result);
extern void score_screen_input_(void);
extern void mission_score_screen(s32 stage);

#endif
