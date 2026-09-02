#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DisposeWeapon(struct Humanoid *human);
 *     APPEAR.C:364, 7 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 * END PSX.SYM */

extern void DisposeOrnament(OrnamentType *objp);

void DisposeWeapon(Humanoid *human)
{
    OrnamentType **weapons;
    short i;

    weapons = human->weapon;
    i = 0;
    do
    {
        DisposeOrnament(weapons[i]);
        weapons[i] = 0;
        i++;
    } while (i < N_WEAPON_SLOTS);
}
