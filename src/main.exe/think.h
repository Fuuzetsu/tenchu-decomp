#ifndef TENCHU_THINK_H
#define TENCHU_THINK_H

struct Humanoid;

extern void StateTransition(struct Humanoid *human);
extern void register_character_death(struct Humanoid *human);
extern s16 GotoPosition(s32 x, s32 z);
extern s16 ChasetoTarget(s32 distance);

extern s16 Think1trace(void);
extern s16 Think1random(void);
extern s16 Think1ninja(void);
extern s16 Think1chase(void);
extern s16 Think1target(void);
extern s16 Think1watch(void);
extern s16 Think1sleep(void);

extern s16 think_alarm_reaction_(void);
extern s16 Think2confirm(void);
extern s16 Think2contact(void);

extern short Think3callaid(void);
extern s16 Think3chase(void);
extern s16 Think3attack(void);
extern s16 Think3escape(void);
extern s16 Think3area(void);
extern s16 Think3hitaway(void);
extern s16 Think3firstattack(void);

extern s16 Think4abandon(void);
extern s16 Think4contact(void);
extern s16 Think4chase(void);

extern void SetupThinkFunction(struct Humanoid *human, TThinkType type);
extern void reset_alert_duration(void);
extern s16 ThinkBasicNone(void);
extern s16 ThinkBasicHuman1(void);
extern s16 ThinkBasicHuman2(void);

#endif
