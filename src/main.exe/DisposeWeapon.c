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

/*
 * DisposeWeapon (0x8002a978, 0x68 bytes) — free all 4 of a character's
 * equipped weapon ornaments (item.h's proven `weapon[4]` @0x94) and clear
 * each slot, via DisposeOrnament (byte-identical clone of DisposeModel).
 * Fixed 4-iteration do-while (no entry guard: the bound is a compile-time
 * constant, so jump.c's duplicate_loop_exit_test folds away entirely —
 * cookbook Loops "bottom-test do-while shape"). The `short i` loop counter
 * indexing the pointer array produces the same fused sign-extend+scale
 * shift pair (`sll i,16` / `sra ,14`) as GetHumanoid's HumanGroup[i] — the
 * natural result of a HImode index scaled by a 4-byte pointer stride.
 */
extern void DisposeOrnament(OrnamentType *objp);

void DisposeWeapon(Humanoid *human)
{
    OrnamentType **wp;
    short i;

    wp = human->weapon;
    i = 0;
    do
    {
        DisposeOrnament(wp[i]);
        wp[i] = 0;
        i++;
    } while (i < 4);
}
