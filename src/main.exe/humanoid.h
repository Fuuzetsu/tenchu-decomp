#ifndef TENCHU_HUMANOID_H
#define TENCHU_HUMANOID_H

/* Humanoid.attribute — the AI/state bit word (mirrored into the think-side
 * global `Attrib` while a character thinks). Known bits, by evidence:
 *   0x0001/0x0002  a 2-bit think command subfield (Think1target/Think4abandon
 *                  write 0/1/2 into it while choosing patrol moves)
 *   0x0010         carrying-a-body (ActSTATE sets 0x12 on pickup, ActDEAD
 *                  toggles 0x10 as the corpse is lifted/dropped)
 *   0x0040         aware/in-combat: without it a hit is an instant kill and
 *                  scores Criticals (the stealth-kill counter); with it (or
 *                  0x0002) the kill scores Murders, and StateTransition only
 *                  rolls the 0x602 engage taunt when it is up
 *   0x0080         ActivateHumans think-priority gate for near humans
 *   0x0400         hooked/held (ActKAGI raises it on a grapple; the chase
 *                  logic treats it as a hold)
 *   0x8000         spotted-the-player alarm (wakes Think1sleep with
 *                  EmergencyNotice; DefaultActionHumanoid raises it)
 * The remaining bits (0x0800, 0x4000, ...) appear only in reads; naming them
 * needs runtime observation. This map is documentation — sites keep their
 * literal spellings until the semantics are runtime-confirmed. */

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
