#ifndef TENCHU_HUMANOID_H
#define TENCHU_HUMANOID_H

struct Humanoid;

/* Original SEMNG.C and MOTION.C interfaces recovered from PSX.SYM. */
extern short Sound(struct Humanoid *human, short seid);
extern short SoundEx(VECTOR *locate, short seid);
extern short SetNowMotion(struct Humanoid *human, short mid, short move);
extern short ControlTraceLine(struct Humanoid *human);

#endif
