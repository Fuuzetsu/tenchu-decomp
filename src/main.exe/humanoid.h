#ifndef TENCHU_HUMANOID_H
#define TENCHU_HUMANOID_H

/* Humanoid.attribute — the AI/state bit word (mirrored into the think-side
 * global `Attrib` while a character thinks). Bits, by evidence — static
 * reading plus the tools/pcsx_attrbits.py runtime observer (332 transitions
 * over a live mission, 2026-08-27):
 *   0x0001/0x0002  the 2-bit think command subfield (Think1target/
 *                  Think4abandon write 0/1/2; observed toggling on idle NPCs)
 *   0x0004         set by SetupThinkFunction iff the think type is a real
 *                  mix (not 0/0x1111/0x2222)
 *   0x0010         raised on NPCs entering their alarm reaction (ActSTATE
 *                  sets 0x12 on body pickup; exact meaning still open)
 *   ATTR_ALERT     0x0040 — aware of the intruder: observed raised on
 *                  fighters AND civilians the moment EmergencyNotice fires,
 *                  cleared when the alarm expires; without it a hit is an
 *                  instant kill and scores Criticals (the stealth-kill
 *                  counter), with it (or 0x0002) the kill scores Murders
 *   ATTR_SUSPEND   0x0080 — AI suspended: ActivateHumans clears it (and
 *                  raises the model's 0x4000) to wake a human within the
 *                  think budget and sets it on far ones;
 *                  ControlAllHumanoid/GetNearestHumanoid/CVA skip suspended
 *                  humans; the ninken summon template stays suspended
 *   0x0400         ActKAGI raises it on a grapple; also observed as a fast
 *                  player-side transient during engage-family motions
 *   0x8000         spotted-the-player alarm trigger (DefaultActionHumanoid)
 * The other observed player-side transients (0x0100/0x0800/0x1000/0x2000/
 * 0x4000) toggle during damage/turning and are not yet named. */
#define ATTR_ALERT 0x0040
#define ATTR_SUSPEND 0x0080


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
