#ifndef TENCHU_HUMANOID_H
#define TENCHU_HUMANOID_H

/* Humanoid.attribute — the AI/state bit word (mirrored into the think-side
 * global `Attrib` while a character thinks). Bits, by evidence — static
 * reading plus the tools/pcsx_attrbits.py runtime observer (332 transitions
 * over a live mission, 2026-08-27):
 *   ATTR_PHASE     0x0003 — the 2-bit awareness phase, written from
 *                  SearchTarget's result: PHASE_CALM 0, PHASE_SUSPICIOUS 1
 *                  (SR==2, the "?" bark), PHASE_ALERT 2 (SR==1, target
 *                  seen — kills in it score Murders, not stealth kills),
 *                  PHASE_INVESTIGATE 3 (SR==-2, target lost; written as
 *                  ATTR_SEARCH | phase with chase[] set to the last-known
 *                  spot)
 *   0x0004         "non-template AI": SetupThinkFunction sets it iff the
 *                  think type is a real mix (not 0/0x1111/0x2222), the
 *                  call-aid/alarm reinforcement morphs set it while
 *                  hand-assigning Think*Func[4], and CVA sets it when a
 *                  cutscene revives a dead actor; StartStageSequence and
 *                  the CVA despawn clear it. Write-only in retail (no
 *                  reader survives), so it stays unnamed
 *   ATTR_TRACE     0x0008 — a patrol route is attached: leLayoutEnemy
 *                  raises it after SetupTraceLine succeeds, Think1trace
 *                  forwards to ControlTraceLine on it, and
 *                  DefaultActionHumanoid's route logic gates on it
 *                  (with human->trace). Never cleared in retail
 *   ATTR_SEARCH    0x0010 — the personal investigation latch: when the
 *                  startle motion (0x80e) completes, ActSTATE sets 0x12
 *                  (think-mode 2 + this bit), stores the player's spot in
 *                  chase[], and the guard goes searching; StateTransition
 *                  keeps alert behavior while it (or EmergencyNotice) is
 *                  up. ActDEAD REUSES the bit on corpses to tag a splash
 *                  (drowning) death, clearing it for every other death
 *   ATTR_ALERT     0x0040 — combat-ready / weapon drawn: EquipWeapon
 *                  raises it on draw (mode 1) and clears it on sheathe,
 *                  and the alarm raises it on fighters AND civilians the
 *                  moment EmergencyNotice fires; without it a hit is an
 *                  instant kill and scores Criticals (the stealth-kill
 *                  counter — a sheathed enemy is a stealth-kill target),
 *                  with it (or 0x0002) the kill scores Murders
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
 *   ATTR_NOFLOOR   0x0800 — nothing solid underfoot: the resolver raises
 *                  it in the no-footing branch; damage taken with it (or
 *                  below ground level) switches to the falling-damage
 *                  motion, and ActSTATE's fall handler branches on it
 *   ATTR_LEDGE     0x1000 — climbable ledge ahead: set (with 0x2000) on
 *                  deep low wall contact; pressing forward with it up
 *                  starts the 0x801 climb motion (ActCHASE)
 *   ATTR_WALLANGLE 0x2000 — the movement probe recorded wall-deflection
 *                  angles (MapVector.angleL/angleH nonzero — the data the
 *                  swim/rope handlers steer along); +ATTR_LEDGE when that
 *                  wall is low (height < -450). Set-only in retail.
 * Still unnamed: 0x0200, the buoyant-surface mirror — set by
 * DefaultActionHumanoid whenever map->attrib bit 2 is up (the clamp that
 * floors vy at 0 and forces height 1), and read only inside compound
 * masks (ActJUMP's ATTR_NOFLOOR|0x200 dive check, ActKAGI's
 * any-contact mask). */
#define ATTR_PHASE 0x0003
#define PHASE_CALM 0
#define PHASE_SUSPICIOUS 1
#define PHASE_ALERT 2
#define PHASE_INVESTIGATE 3
#define ATTR_TRACE 0x0008
#define ATTR_SEARCH 0x0010
#define ATTR_FLOAT 0x0020
#define ATTR_ALERT 0x0040
#define ATTR_SUSPEND 0x0080
#define ATTR_FALL 0x0100
#define ATTR_NOFLOOR 0x0800
#define ATTR_LEDGE 0x1000
#define ATTR_WALLANGLE 0x2000
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
