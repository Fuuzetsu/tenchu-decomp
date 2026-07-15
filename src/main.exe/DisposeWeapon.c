#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DisposeWeapon(struct Humanoid *human);
 *     APPEAR.C:364, 7 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
    do {
        DisposeOrnament(wp[i]);
        wp[i] = 0;
        i++;
    } while (i < 4);
}
