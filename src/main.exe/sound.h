#ifndef TENCHU_SOUND_H
#define TENCHU_SOUND_H

/* Inferred handles for retail sound ids. None of these names survive in
 * PSX.SYM or the game data; they summarize the common meaning of every known
 * call site. The encoding class is recovered; confidence describes only the
 * inferred role, not the numeric value.
 *
 * Sound() treats values below 0x10 as per-character category slots, ORing
 * them with Humanoid.sound; slots 6..15 are voice lines. Values with a high
 * nibble, and every SoundEx() value, are direct ids in the stage sound bank.
 * Keep those namespaces separate even when their numbers happen to agree. */

/* Direct stage sound effects. */
#define SE_UI_CURSOR         0x0B /* high: item-menu cursor movement */
#define SE_ITEM_UNAVAILABLE  0x0C /* high: unavailable/invalid item action */
#define SE_TURN_STEP         0x10 /* high: standing/combat turn footfall */
#define SE_FOOTSTEP          0x11 /* high: walk, crouch-walk, and wall-slide step */
#define SE_WATER_SPLASH      0x16 /* high: enter water and drowning splash */
#define SE_LEDGE_GRIP        0x1B /* high: ledge catch and shimmy grip */
#define SE_SMOKE_PUFF        0x23 /* high: smoke-backed item appear/disappear */
#define SE_EXPLOSION         0x25 /* high: mine and fireball explosion */
#define SE_FIRE              0x28 /* high: flame attack and ambient fire */
#define SE_BLOOD_SPLATTER    0x37 /* high: blood and gore particle impact */
#define SE_JUMP_IMPACT       0x48 /* medium-high: run-jump landing and wallkick */
#define SE_ITEM_USE          0x4C /* high: item activation/use success */

/* Per-character voice/category slots passed through Sound(). */
#define CHAR_VOICE_HURT        6    /* medium-high: hit, choke, or hard landing */
#define CHAR_VOICE_HURT_HEAVY  8    /* medium-high: severe/fatal damage */
#define CHAR_VOICE_NOTICE      0x0C /* medium-high: glimpse/corpse/animal notice */
#define CHAR_VOICE_ALERT       0x0D /* high: target acquired or alert bark */

#endif
