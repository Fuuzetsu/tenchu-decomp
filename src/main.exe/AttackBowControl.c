#include "common.h"
#include "sound.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AttackBowControl(void);
 *     MOTION.C:800, 28 src lines, frame 72 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct PARAM_ITEM_LAUNCH item
 *     reg   $v0       struct VECTOR * pos
 *     stack sp+56     struct SVECTOR vect
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 * END PSX.SYM */

extern Humanoid *Me_MOTION_C;
extern void bow_shoot_logic(s16 kind, VECTOR *start);
extern void UpdateOrnament(OrnamentType *objp, short ry);
extern short DrawOrnament(OrnamentType *objp);

static inline const struct BowTimingEntry *BowTimingFromByteOffset(s32 byte_offset)
{
    return (const struct BowTimingEntry *)((const u8 *)BowTiming + byte_offset);
}

void AttackBowControl(s16 timing_window)
{
    s16 count;
    VECTOR *pos;
    PARAM_ITEM_LAUNCH item; /* Unused local recorded by PSX.SYM. */
    SVECTOR vect;           /* Unused local recorded by PSX.SYM. */
    s32 byte_offset;
    const struct BowTimingEntry *p;
    s32 byte_offset2;
    const struct BowTimingEntry *p2;

    count = dtM->count;
    if (count == 1)
    {
        Sound(Me_MOTION_C, CHAR_SE_ATTACK);
    }
    else
    {
        byte_offset = timing_window << 2;
        p = BowTimingFromByteOffset(byte_offset);
        if (p->min <= count && count < p->max)
        {
            UpdateOrnament(Me_MOTION_C->weapon[WEAPON_SLOT_INACTIVE_0], 0);
            DrawOrnament(Me_MOTION_C->weapon[WEAPON_SLOT_INACTIVE_0]);
        }
    }
    byte_offset2 = timing_window;
    byte_offset2 = (s16)byte_offset2 << 2;
    p2 = BowTimingFromByteOffset(byte_offset2);
    if (dtM->count == p2->max)
    {
        pos = GetAbsolutePosition(Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_0], 0, 0, 0);
        bow_shoot_logic(ITEM_ARROW, pos);
        Sound(Me_MOTION_C, CHAR_SE_ATTACK_ALT);
    }
}
