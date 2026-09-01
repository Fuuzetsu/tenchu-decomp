#ifndef TENCHU_SOUND_H
#define TENCHU_SOUND_H

/* Byte terminator shared by the voice, music-remap, and generic sound-id
 * tables. */
#define SOUND_TABLE_END 0xFF

/* Inferred handles for retail sound slots. None of these names survive in
 * PSX.SYM or the game data; they summarize the gameplay role of each known
 * slot. The encoding class is recovered; confidence describes only the
 * inferred role, not the numeric value.
 *
 * Sound() treats values below 0x10 as per-character category slots, ORing
 * them with Humanoid.sound; slots 6..15 are voice lines. Values with a high
 * nibble, and every SoundEx() value, are direct ids in the stage sound bank.
 * Keep those namespaces separate even when their numbers happen to agree.
 *
 * StageSE is reloaded from STAGE<n><R/A>.VAB by CreateStage, so an SE_* name
 * describes a role/slot in the current per-stage, per-character VAB rather
 * than promising one globally fixed waveform. Non-gameplay screens reuse the
 * resident stage samples; those UI uses do not redefine a gameplay slot. */

/* Direct stage sound effects. */
#define SE_MENU_APPLY          0x08 /* medium: apply a menu/loadout change */
#define SE_PAUSE_ENTER         0x09 /* high: enter pause */
#define SE_MENU_CONFIRM        0x0A /* high: confirm/unpause/debug chime */
#define SE_UI_CURSOR           0x0B /* high: item-menu cursor movement */
#define SE_ITEM_UNAVAILABLE    0x0C /* high: unavailable/invalid item action */
#define SE_ITEM_TRANSFER       0x0D /* high: add/return an inventory item */
#define SE_WARNING_BEEP        0x0E /* high: periodic strain warning */
#define SE_TURN_STEP           0x10 /* high: standing/combat turn footfall */
#define SE_FOOTSTEP            0x11 /* high: walk/crouch/wall-slide step */
#define SE_RUN_STEP            0x12 /* high: ordinary running footfall */
#define SE_ACROBATIC_MOVE      0x13 /* high: dash/backflip/climb impact */
#define SE_RUN_STEP_WOOD       0x14 /* high: wooden-surface running footfall */
#define SE_WATER_MOVE          0x15 /* high: swim stroke or water pull */
#define SE_WATER_SPLASH        0x16 /* high: enter water/drowning splash */
#define SE_JUMP_MOVE           0x17 /* medium: run-jump movement */
#define SE_GRAPPLE_PULL        0x18 /* high: grappling-hook pull begins */
#define SE_LAND_LIGHT          0x19 /* high: ordinary landing */
#define SE_LAND_HEAVY          0x1A /* high: heavy/dive landing */
#define SE_LEDGE_GRIP          0x1B /* high: ledge catch/shimmy grip */
#define SE_BODY_SLAM           0x1D /* high: launched body hits ground */
#define SE_THROW_WEAPON        0x1E /* high: shuriken/hook launch */
#define SE_WEAPON_RECOVER      0x1F /* high: thrown weapon/hook recovery */
#define SE_CALTROP_THROW       0x22 /* high: caltrop item is thrown */
#define SE_SMOKE_PUFF          0x23 /* high: smoke-backed appearance */
#define SE_MEDICINE            0x24 /* high: healing medicine effect */
#define SE_EXPLOSION           0x25 /* high: mine/fireball explosion */
#define SE_SLEEP_DART_THROW    0x26 /* high: sleeping-dart throw */
#define SE_MAP_OPEN            0x27 /* high: pause map opens */
#define SE_FIRE                0x28 /* high: flame/ambient fire */
#define SE_PROJECTILE_HIT      0x30 /* high: projectile hits character */
#define SE_PROJECTILE_IMPACT   0x31 /* high: terrain impact/expiry */
#define SE_GUN_HIT_FLESH       0x34 /* high: gunshot hits character */
#define SE_GUN_HIT_SOLID       0x35 /* high: gunshot hits terrain */
#define SE_WEAPON_CLASH        0x36 /* high: weapon-on-weapon impact */
#define SE_BLOOD_SPLATTER      0x37 /* high: blood/gore impact */
#define SE_DEATH               0x38 /* medium-high: ordinary death cue */
#define SE_LIGHTNING           0x39 /* high: lightning cast/damage */
#define SE_MECHANISM           0x40 /* medium-high: door/pitfall mechanism */
#define SE_CUTSCENE_DEATH      0x41 /* medium: scripted character death */
#define SE_LURE_FLUTE          0x43 /* high: lure-flute call */
#define SE_BONFIRE             0x47 /* high: periodic bonfire crackle */
#define SE_JUMP_IMPACT         0x48 /* medium-high: jump/wallkick impact */
#define SE_FATAL_FALL          0x49 /* high: player enters death area */
#define SE_ITEM_USE            0x4C /* high: item activation/use */

/* Per-character effect/category slots passed through Sound(). Slots 0/1 are
 * deliberately A/B: ordinary equip/stow and twin-blade swaps use opposite
 * phases, so a directional name would overclaim. */
#define CHAR_SE_WEAPON_CHANGE_A  0 /* medium: one weapon-change phase */
#define CHAR_SE_WEAPON_CHANGE_B  1 /* medium: the other weapon-change phase */
#define CHAR_SE_ATTACK            2 /* high: primary attack effect */
#define CHAR_SE_ATTACK_ALT        3 /* high: alternate attack effect */
#define CHAR_SE_IMPACT            4 /* medium-high: character impact */
#define CHAR_SE_SPECIAL           5 /* medium: character-specific effect */

/* Per-character voice slots. */
#define CHAR_VOICE_HURT        6    /* medium-high: damage reaction */
#define CHAR_VOICE_HURT_ALT    7    /* medium-high: alternate damage reaction */
#define CHAR_VOICE_HURT_HEAVY  8    /* medium-high: severe/fatal damage */
#define CHAR_VOICE_ACTION_A    9    /* medium: attack/reinforcement line */
#define CHAR_VOICE_ACTION_B    10   /* medium: alternate action line */
#define CHAR_VOICE_TAUNT       11   /* high: taunt attack */
#define CHAR_VOICE_NOTICE      0x0C /* medium-high: glimpse/corpse notice */
#define CHAR_VOICE_ALERT       0x0D /* high: target acquired/alert bark */
#define CHAR_VOICE_REACTION    0x0E /* medium: alarm/give-up reaction */
#define CHAR_VOICE_IDLE        0x0F /* high: idle/fidget voice */

#endif
