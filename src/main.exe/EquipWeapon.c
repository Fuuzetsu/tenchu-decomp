#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void EquipWeapon(struct Humanoid *human, short mode);
 *     APPEAR.C:380, 33 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct Humanoid * human
 *     param $a1       short mode
 * END PSX.SYM */

/*
 * EquipWeapon (0x8002a9e0) — toggle a character's "weapon drawn" attribute
 * bit (0x40) and, on the transition, rotate the equipped-weapon slots
 * (item.h's proven `weapon[4]`) according to the weapon's kind (this
 * function proves a NEW field, `weapon_kind` @0x8E — see item.h).
 * dispose_weapon_data_of_char_ (FUN_800270c8) is always called first with a
 * literal mode of 3.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `wp = human->weapon;` (a pointer to weapon[0]) is set up FIRST, before
 *    the dispose call — matches Ghidra's own statement order and the asm's
 *    `addiu s1,s2,0x94` sitting ahead of the `jal`.
 *  - The if/else is written `if (mode != 0) {set-bit} else {clear-bit}`,
 *    the OPPOSITE of Ghidra's `if (mode == 0)` rendering: cc1 always lays
 *    the THEN body as the physical fallthrough, and the raw asm's `beqz
 *    mode,L` branches AWAY (to the clear-bit body) when mode==0 while
 *    falling through to the set-bit body — so the THEN clause must be the
 *    mode!=0 (set) case for the fallthrough to land there.
 *  - `idx = human->weapon_kind - 4;` is a real source statement (not just
 *    cc1's own expand_case bias): the target sign-extends `idx` with an
 *    explicit `sll+sra` pair between the subtraction and the `sltiu` range
 *    check, which only happens for an actual `short` LOCAL being promoted
 *    for the switch (a switch directly over `human->weapon_kind` gets no
 *    such promotion, 2 bytes short). Ghidra's printed case labels (0,1,3,
 *    0x1b / 8,0xf,0x10,0x12,0x15,0x18) are `idx`'s values, i.e. already
 *    `-4`; the raw source likely names weapon_kind values 4/5/7/0x1F and
 *    0xC/0x13/0x14/0x16/0x19/0x1C, but `idx`'s own literals are what
 *    reproduce the bytes.
 *  - Case-body load/store order follows the raw .s, not Ghidra's SSA
 *    rendering (which reorders loads): the first swap loads wp[0] then
 *    wp[2] then wp[3] (Ghidra shows wp[2] first); the second swap loads
 *    wp[2] then wp[0] (Ghidra shows wp[0] first).
 *  - The second case group is written LAST (no cases follow it) so it
 *    falls straight into the shared return with no explicit `break`/jump —
 *    matches the raw asm having no `j` after its body, unlike the first
 *    group's explicit `j switchD_8002aa7c__caseD_2`.
 *  - Register tie across the two (mutually exclusive) case bodies: the
 *    wp[0]-holding temp is the SAME C variable (`a`) in both cases, which
 *    ties both to $a0 like the target; the OTHER temp in each case (`b`/`c`
 *    in case 1, `d` in case 2) must stay a variable never referenced by the
 *    other case — reusing one of case 1's non-`a` temps for case 2's other
 *    slot (tried `c`) drags case 1's own allocation off-target too, because
 *    one C variable is one pseudo/one hard reg for the WHOLE function, not
 *    per-occurrence. Pick which name to reuse across cases by matching
 *    ROLE (both are "the wp[0] value"), not just by availability.
 */
extern void dispose_weapon_data_of_char_(Humanoid *h, int a);

void EquipWeapon(Humanoid *human, short mode)
{
    OrnamentType **wp;
    OrnamentType *a, *b, *c;
    OrnamentType *d;
    short idx;

    wp = human->weapon;
    dispose_weapon_data_of_char_(human, 3);
    if (mode != 0)
    {
        if ((human->attribute & 0x40) != 0)
        {
            return;
        }
        human->attribute = human->attribute | 0x40;
    }
    else
    {
        if ((human->attribute & 0x40) == 0)
        {
            return;
        }
        human->attribute = human->attribute & 0xFFBF;
    }
    idx = human->weapon_kind - 4;
    switch (idx)
    {
    case 0:
    case 1:
    case 3:
    case 0x1b:
        a = wp[0];
        b = wp[2];
        c = wp[3];
        wp[2] = a;
        a = wp[1];
        wp[0] = b;
        wp[1] = c;
        wp[3] = a;
        break;
    case 8:
    case 0xf:
    case 0x10:
    case 0x12:
    case 0x15:
    case 0x18:
        d = wp[2];
        a = wp[0];
        wp[0] = d;
        wp[2] = a;
        break;
    }
}
