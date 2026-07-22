#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void KillHumanoid(struct Humanoid *human);
 *     HUMAN.C:78, 15 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 * END PSX.SYM */

/*
 * KillHumanoid (0x800291a8) — tear down a Humanoid (conflict box, model
 * archive, motion manager, weapon data, the Humanoid block itself), then
 * swap-remove it from HumanGroup[] if present.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - The search-then-swap-remove loop is GetHumanoid.c's exact twin: a
 *    `short i` loop counter over `HumanGroup[]` (a `Humanoid *[]`) fuses the
 *    sign-extend with the pointer's 4-byte stride into one `sll 16/sra 14`
 *    per iteration instead of loop.c strength-reducing to a walking
 *    pointer — see the toolchain-gotchas note citing GetHumanoid/
 *    DisposeWeapon for this exact 2-instruction shape.
 *  - `for (i = 0; i < Humans; i++) if (HumanGroup[i] == human) break;` is
 *    the standard entry-duplicated bottom-test do-while with a `break`
 *    joining the same exit as the counter running out — `i` is live after
 *    the loop either way (found index, or == Humans if not found).
 *  - `Humans` is read `lh` (signed) for the entry `0 < Humans` guard and
 *    again `lh`+`lhu` (two un-CSE'd loads) for the post-loop `i < Humans`
 *    test / `Humans - 1` capture — the same per-read-purpose disagreement
 *    as CreateHumanoid/InsertConflict.
 */
extern void DeleteConflict(ModelType *m);
extern void DisposeModelArchive(ModelArchiveType *mad);
extern void DisposeMotionManager(MotionManager *mm);
extern void dispose_weapon_data_of_char_(Humanoid *h, int a);
extern void vfree(void *p);


void KillHumanoid(Humanoid *human)
{
    short i;

    if (human != 0)
    {
        DeleteConflict(human->model->object[0]);
        DisposeModelArchive(human->model);
        DisposeMotionManager(human->motion);
        DisposeWeapon(human);
        dispose_weapon_data_of_char_(human, 3);
        vfree(human);
        for (i = 0; i < Humans; i++)
        {
            if (HumanGroup[i] == human)
            {
                break;
            }
        }
        if (i < Humans)
        {
            Humans = Humans - 1;
            HumanGroup[i] = HumanGroup[Humans];
        }
    }
}
