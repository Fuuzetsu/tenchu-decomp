#include "common.h"
#include "sound.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AttackGunControl(short length, short frm);
 *     MOTION.C:832, 13 src lines, frame 64 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short length
 *     param $a1       short frm
 *     stack sp+16     struct PARAM_ITEM_LAUNCH item
 * END PSX.SYM */

extern Humanoid *Me_MOTION_C;
extern void bow_shoot_logic(s16 kind, VECTOR *start);

void AttackGunControl(s16 length, s16 frm)
{
    PARAM_ITEM_LAUNCH item;

    if (dtM->count == frm)
    {
        bow_shoot_logic(
            ITEM_GUN,
            GetAbsolutePosition(
                Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_0], 0,
                length, -100));
        Sound(Me_MOTION_C, CHAR_SE_ATTACK);
    }
}
