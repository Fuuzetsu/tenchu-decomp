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

/* The whole of PSX video memory, as a framebuffer rectangle. */
#define VRAM_W 1024
#define VRAM_H 512

/* PSX 12.12 fixed-point values used by sprite/effect scales and angles. */
#define FIXED_QUARTER 0x0400
#define FIXED_HALF 0x0800
#define FIXED_ONE 0x1000

/* Packed 0xRRGGBB effect colours. Use RGB24 for one-off palette shades. */
#define RGB24(r, g, b) (((r) << 16) | ((g) << 8) | (b))
#define COLOR_WHITE RGB24(255, 255, 255)
#define COLOR_YELLOW RGB24(255, 255, 0)
#define COLOR_RED RGB24(255, 0, 0)
#define COLOR_GRAY RGB24(128, 128, 128)
#define COLOR_GRAY_DARK RGB24(127, 127, 127)

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
#define DAMAGE_LAUNCH_SPEED 0x46 /* high: damage-launch knockback magnitude */
#define RUN_JUMP_SPEED 0x7F      /* high: MOT_JUMP_RUN transition from a forward dash */

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
#define INDIRECT_RANGE 20000         /* bow/gun AI: engagement + search-clear radius */

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

/* Shared full-screen fade and game-over stagger. */
#define SCREEN_FADE_FRAMES 0x20
#define SCREEN_FADE_MODE 2
#define SCREEN_FADE_LEVEL 8
#define GAME_OVER_LINE_1_FADE_FRAME 0x119
#define GAME_OVER_LINE_2_FADE_FRAME 0x15F
#define GAME_OVER_LINE_3_FADE_FRAME 0x1A5
#define GAME_OVER_TITLE_OT_PRIORITY 0x0A
#define GAME_OVER_TEXT_OT_PRIORITY 0x50

/* PadShockAR envelope values. */
#define RUMBLE_POWER_OFF 0
#define RUMBLE_POWER_HALF 0x7F
#define RUMBLE_POWER_MAX 0xFF
#define RUMBLE_ATTACK_NONE 0
#define RUMBLE_ATTACK_FAST 5
#define RUMBLE_ATTACK_NORMAL 10
#define RUMBLE_RELEASE_NONE 0
#define RUMBLE_RELEASE_SHORT 10
#define RUMBLE_RELEASE_MEDIUM 20
#define RUMBLE_RELEASE_LONG 30

/* SPU master-volume ceiling used when restoring music/voice. */
#define MASTER_VOLUME_MAX 0x7F
#define MASTER_VOLUME_MUTE 0

/* Screen projection distance (GsSetProjection): apparent sprite size is
 * size * PROJECTION_DISTANCE / depth in every sprite-effect renderer. */
#define PROJECTION_DISTANCE 300

/* Camera wall avoidance (AntiWall/CameraDirection): sideways push
 * vector magnitude. */
#define WALL_AVOID_PUSH 1000

/* Models closer than this screen depth draw from the plain renderer
 * bank; farther ones use the fog bank (DrawModel/DrawClip). */
#define FOG_DEPTH 300
#define FOG_DQA (-0x7EF4) /* GTE depth-cue slope */
#define FOG_DQB 0x2F282E0 /* GTE depth-cue offset */

/* Free-look camera (CameraDirection). */
#define CAMERA_LOOK_LIMIT_X 0x38E /* ~80 degrees up/down */
#define CAMERA_LOOK_LIMIT_Y 0x400 /* 90 degrees sideways */
#define CAMERA_BOOM_LEN 1200      /* eye distance behind the look target */

/* Ledge/hang probe (ActMOVE/ActCHASE): height above the feet at which
 * HangCheck looks for a grabbable edge. */
#define LEDGE_PROBE_RISE 400

/* Smoke puffs (SetSmoke/SetSmokeS/spawn_smoke_burst_): random spawn
 * scale in [SMOKE_SCALE_MIN, SMOKE_SCALE_MIN + SMOKE_SCALE_SPREAD). */
#define SMOKE_SCALE_MIN 0x1000
#define SMOKE_SCALE_SPREAD 0x2000
#define SMOKE_DRIFT_DIVISOR_DEFAULT 12

/* Snowfall (SetSnow/DrawSnow/ProcMiscSnowfall): flakes wrap in a
 * SNOW_SPAN-wide box around the viewpoint. */
#define SNOW_RANGE 3000
#define SNOW_SPAN 6000

/* Grapple wire rendering (SetWire). */
#define WIRE_SEG_LEN 300 /* world units per drawn segment */
#define WIRE_SAG_DIV 32  /* midpoint sags by length / this */

/* Terrain: a forward drop steeper than this cancels crouch-walk /
 * default action stepping. */
#define STEP_DROP_LIMIT (-450)

/* Falling (DefaultActionHumanoid). */
#define GRAVITY_ACCEL 20      /* vy gain per frame while airborne */
#define FALL_SPEED_MAX 400    /* terminal fall speed */
#define DEATH_FALL_HEIGHT 25000 /* a MAP_DEATH floor farther than this kills */

/* Loadout: how many item stacks fit in the mission inventory. */
#define MAX_SELECTED_ITEMS 6

/* End-of-stage scoring (calculate_score / mission_score_screen). */
#define SCORE_CLOCK_MAX 0x1A5C2 /* highest recorded stage clock */
#define MEDAL_PULSE_PERIOD 90
#define MEDAL_PULSE_AMPLITUDE 80
#define ROW_PULSE_AMPLITUDE 60
#define SCORE_PER_CRITICAL 20
#define SCORE_PER_MURDER 5
#define SCORE_PER_FRIEND_HIT (-30)
#define STEALTH_BASE 400      /* stealth component when never spotted */
#define STEALTH_BASE_SEEN 300 /* ...when spotted at least once */
#define SPOT_PENALTY 20       /* per spot; the medicine-herb stage doubles it */
#define SCORE_PER_GRADE 100
#define GRADE_MAX 4

#endif
