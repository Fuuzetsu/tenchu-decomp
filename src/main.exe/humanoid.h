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
 *   ATTR_SEARCH    0x0010 — the personal investigation latch: when the
 *                  startle motion (0x80e) completes, ActSTATE sets 0x12
 *                  (think-mode 2 + this bit), stores the player's spot in
 *                  chase[], and the guard goes searching; StateTransition
 *                  keeps alert behavior while it (or EmergencyNotice) is
 *                  up. ActDEAD REUSES the bit on corpses to tag a splash
 *                  (drowning) death, clearing it for every other death
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
 *   ATTR_FLOAT     0x0020 — free-floating creature: only BreedLife's
 *                  S1/S2 spawn arm sets it, and it exempts the character
 *                  from FallCheck, gravity accumulation, and conflict
 *                  push-out (the moat fish)
 *   ATTR_FALL      0x0100 — airborne: DefaultActionHumanoid raises it while
 *                  map->height > 0 and accumulates gravity (vy += 20)
 *   ATTR_WALL      0x0400 — terrain contact: DefaultActionHumanoid raises it
 *                  whenever map->vector is nonzero (a wall in the movement
 *                  probe); ActKAGI raises it on a grapple (you are on a
 *                  wall). The think layer treats it as "blocked": Think1random
 *                  stops random-walking, Think2contact/StateTransition stop
 *                  when pushing against it, ChasetoTarget gives up
 *   ATTR_HIT      0x4000 — inside a weapon/projectile hitbox: every
 *                  size.pad=CONFLICT_HIT slot is created by an attack
 *                  (ActATTACK's swing, the gun/launcher/happou/napalm
 *                  projectiles), and the resolver saves the slot index in
 *                  vector.pad. StateTransition reacts with the startle
 *                  motion and engage promotion; AttackShort activates
 *                  actmode
 *   ATTR_PUSH     0x8000 — being pushed out of a solid conflict object
 *                  (an earlier note called this the spotted-the-player
 *                  trigger — wrong: the only setter is the object-collision
 *                  resolver). Cleared together with ATTR_FALL (& 0x7eff)
 *                  when the character stands on the object's top
 * Still unnamed (evidence too thin): 0x0010 (alarm-reaction, ActSTATE sets
 * 0x12 on body pickup), 0x0020 (gravity/collision exemption), 0x0200
 * (map->attrib & 2 head clamp), 0x0800 (no footing), 0x1000/0x2000 (wall
 * step/snap pair, set as | 0x3000 on deep wall contact). */
#define ATTR_SEARCH 0x0010
#define ATTR_FLOAT 0x0020
#define ATTR_ALERT 0x0040
#define ATTR_SUSPEND 0x0080
#define ATTR_FALL 0x0100
#define ATTR_WALL 0x0400
#define ATTR_HIT 0x4000
#define ATTR_PUSH 0x8000


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
