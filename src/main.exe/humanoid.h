#ifndef TENCHU_HUMANOID_H
#define TENCHU_HUMANOID_H

struct Humanoid;
struct TraceLine;
struct TracePoint;

/* Shared SEMNG.C, MOTION.C, and HUMAN.C interfaces. */
extern short dtPAD;
extern short motID;
extern short motMODE;
/* Raw button-bit view for the MOTION.C sites whose retail loads are unsigned. */
#define MOTION_PAD_BITS (*(unsigned short *)&dtPAD)

extern short Sound(struct Humanoid *human, short seid);
extern short SoundEx(VECTOR *locate, short seid);
extern struct Humanoid *CreateHumanoid(short type, unsigned long *mad);
extern void KillHumanoid(struct Humanoid *human);
extern short ControlAllHumanoid(void);
extern void ControlHumanoid(struct Humanoid *human);
extern short DefaultActionHumanoid(struct Humanoid *human);
extern short SetNowMotion(struct Humanoid *human, short mid, short move);
extern short ControlTraceLine(struct Humanoid *human);
extern struct Humanoid *GetHumanoid(short type);
extern struct Humanoid *GetNearestHumanoid(struct Humanoid *human, short distance);
extern short SearchTarget(struct Humanoid *human, long *distance, short *degree);
extern struct TraceLine *SetupTraceLine(struct Humanoid *human,
                                        struct TracePoint *point);
extern void MoveHumanoid(struct Humanoid *human, short ordr, short side);
extern void GetMoveSpeed(SVECTOR *vect, short ry, short ordr, short side);
/* Retail widened the demo's short roty parameter; the callee uses it directly. */
extern s16 GetDirection(s32 dx, s32 dz, s32 roty);

#endif
