#ifndef TUNING_H
#define TUNING_H

/* Named game-tuning constants. None of these names appear in the demo
 * symbols — they are our handles on recurring retail values so a mod can
 * retune them in one place (./Build mod patches in place; the values
 * below are the byte-matched retail defaults, and changing any of them
 * deliberately fails ./Build check). Add a constant here when the same
 * quantity recurs across files or is an obvious gameplay knob; leave
 * one-off frame counts inline. */

/* Display */
#define SCREEN_W 320
#define SCREEN_H 240

/* Every Draw* emitter culls primitives at or beyond this ordering-table
 * depth; DepthPoint starts here and EndDrawing splices this row. */
#define DEPTH_LIMIT 1250

/* Heights above the feet, in world units. */
#define EYE_HEIGHT 3050        /* AI sight-probe origin (StateTransition, Think1ninja) */
#define THROW_HEIGHT 1200      /* launch origin for thrown/scattered items */
#define CAMERA_EYE_HEIGHT 1550 /* first-person camera origin */

/* Size of the shared effect pool (effect.h's EffectSlot[]). */
#define N_EFFECT_SLOTS 200

/* Movement speeds (MoveHumanoid order-speed units). */
#define CHASE_WALK_SPEED 120 /* enemies closing in (ActCHASE/ActENGAGE) */
#define SWIM_SPEED 60

/* Weapon-item damage (DamageControl's per-item switch; a fall-through
 * chain lets a nonzero incoming dmg override the shuriken/happou/gun/
 * arrow defaults). */
#define DMG_MAKIBISHI 3
#define DMG_SHURIKEN 20
#define DMG_HAPPOU 30
#define DMG_GUN 20
#define DMG_ARROW 10
#define DMG_NAPALM 25
#define DMG_FIRE 30 /* also lightning */
#define DMG_JIRAI 45

/* Poisoned-bait attraction radius (world units). */
#define DOKUDANGO_RANGE 10000

/* Item effect durations, in frames. */
#define SMOKE_DURATION 120    /* smoke-bomb cloud lifetime */
#define SHINSOKU_DURATION 75  /* speed-potion boost length */
#define GOSIN_DURATION 450    /* protection-charm length */
#define NINKEN_DURATION 1800  /* summoned dog lifetime */
#define NINGYO_DURATION 90    /* decoy-doll walk time */
#define NINGYO_HP 99          /* decoy-doll hit points */
#define MANEBUE_DURATION 30   /* lure-flute effect window */
#define KAENGEKI_DELAY 40     /* flame-wave wind-up after the swing */

/* Timers, in frames. */
#define GAME_OVER_TIMEOUT 2700 /* game-over screen auto-advance (45 s) */

#endif
