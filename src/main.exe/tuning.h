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

/* Effects closer than this screen depth are not drawn (near-plane cull
 * shared by the sprite-effect emitters). */
#define NEAR_DEPTH 36

/* Every Draw* emitter culls primitives at or beyond this ordering-table
 * depth; DepthPoint starts here and EndDrawing splices this row. */
#define DEPTH_LIMIT 1250

/* Heights above the feet, in world units. */
#define EYE_HEIGHT 3050        /* AI sight-probe origin (StateTransition, Think1ninja) */
#define THROW_HEIGHT 1200      /* launch origin for thrown items (ReqItemDefault) */
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

/* Alert-state countdown (reset_alert_duration), in frames. */
#define ALERT_DURATION 300
#define ALERT_DURATION_HARD 600

/* AI activation radii (ActivateHumans' once-a-second sweep). */
#define ACTIVATE_RADIUS 13000      /* enemies inside this stay awake */
#define ACTIVATE_RADIUS_WIDE 26000 /* while the player casts the far-sight item */
#define DEACTIVATE_RADIUS 17000    /* beyond this an enemy is parked/teleported */

/* Combat pacing. */
#define ATTACK_COOLDOWN_PER_LEVEL 10 /* frames per EngageLevel between AI attacks */
#define SR_CLEAR_RANGE 10000         /* beyond this, the search-result state clears */

/* The alert-refresh block the think TUs paste inline (the same body as
 * reset_alert_duration(); `tmp` names each site's local). Macro is
 * reconstruction shorthand, expands to the identical text. */
#define RESET_ALERT_DURATION(tmp)                                             \
    tmp = ALERT_DURATION;                                                     \
    if (gNannido == DIFFICULTY_HARD)                                          \
    {                                                                         \
        tmp = ALERT_DURATION_HARD;                                            \
    }                                                                         \
    EmergencyNotice = tmp;

/* Memory-card retry cap (both card state machines). */
#define CARD_RETRY_LIMIT 3

/* Timers, in frames. */
#define GAME_OVER_TIMEOUT 2700 /* game-over screen auto-advance (45 s) */

/* Screen projection distance (GsSetProjection): apparent sprite size is
 * size * PROJECTION_DISTANCE / depth in every sprite-effect renderer. */
#define PROJECTION_DISTANCE 300

/* Camera wall avoidance (AntiWall): sideways push vector magnitude. */
#define WALL_AVOID_PUSH 1000

/* Ledge/hang probe (ActMOVE/ActCHASE): height above the feet at which
 * HangCheck looks for a grabbable edge. */
#define LEDGE_PROBE_RISE 400

/* Grapple wire rendering (SetWire). */
#define WIRE_SEG_LEN 300 /* world units per drawn segment */
#define WIRE_SAG_DIV 32  /* midpoint sags by length / this */

/* End-of-stage scoring (calculate_score). */
#define SCORE_PER_CRITICAL 20
#define SCORE_PER_MURDER 5
#define SCORE_PER_FRIEND_HIT (-30)
#define STEALTH_BASE 400      /* stealth component when never spotted */
#define STEALTH_BASE_SEEN 300 /* ...when spotted at least once */
#define SPOT_PENALTY 20       /* per spot; the medicine-herb stage doubles it */
#define SCORE_PER_GRADE 100
#define GRADE_MAX 4

#endif
