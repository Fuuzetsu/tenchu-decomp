#ifndef TENCHU_HUMANOID_H
#define TENCHU_HUMANOID_H

struct Humanoid;

/* Shared SEMNG.C, MOTION.C, and HUMAN.C interfaces. */
extern short Sound(struct Humanoid *human, short seid);
extern short SoundEx(VECTOR *locate, short seid);
extern short SetNowMotion(struct Humanoid *human, short mid, short move);
extern short ControlTraceLine(struct Humanoid *human);
/* Retail widened the demo's short roty parameter; the callee uses it directly. */
extern s16 GetDirection(s32 dx, s32 dz, s32 roty);

#endif
