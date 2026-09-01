#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void EquipWeapon(struct Humanoid *human, short mode);
 *     APPEAR.C:380, 33 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *     param $a1       short mode
 * END PSX.SYM */

/*
 * EquipWeapon (0x8002a9e0) — toggle a character's "weapon drawn" attribute
 * bit (0x40) and, on the transition, rotate the equipped-weapon slots
 * (item.h's proven `weapon[4]`) according to the weapon's kind (this
 * function proves the recovered `wpatk` field at retail offset 0x8E).
 * dispose_weapon_data_of_char_ is always called first with a
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
 *  - The switch is written directly over `human->wpatk` with the true
 *    weapon-pattern case values (4/5/7/0x1F and 0xC/0x13/0x14/0x16/
 *    0x19/0x1C) — re-measured 2026-08-29: this matches byte-for-byte
 *    (cc1's own expand_case bias supplies the -4 rebase and the same
 *    sll+sra promotion). An earlier draft believed a biased
 *    `idx = wpatk - 4` local was required; that measurement was stale.
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

/* WEAPON_DRAWN raises ATTR_ALERT; WEAPON_SHEATHED clears it. */
void EquipWeapon(Humanoid *human, short mode)
{
    OrnamentType **wp;
    OrnamentType *a, *b, *c;
    OrnamentType *d;

    wp = human->weapon;
    dispose_weapon_data_of_char_(human, 3);
    if (mode != WEAPON_SHEATHED)
    {
        if ((human->attribute & ATTR_ALERT) != 0)
        {
            return;
        }
        human->attribute = human->attribute | ATTR_ALERT;
    }
    else
    {
        if ((human->attribute & ATTR_ALERT) == 0)
        {
            return;
        }
        human->attribute = human->attribute & ~ATTR_ALERT;
    }
    switch (human->wpatk)
    {
    case KODATI:
    case JYUTE:
    case EN:
    case KATANA_2:
        a = wp[WEAPON_SLOT_ACTIVE_0];
        b = wp[WEAPON_SLOT_INACTIVE_0];
        c = wp[WEAPON_SLOT_INACTIVE_1];
        wp[WEAPON_SLOT_INACTIVE_0] = a;
        a = wp[WEAPON_SLOT_ACTIVE_1];
        wp[WEAPON_SLOT_ACTIVE_0] = b;
        wp[WEAPON_SLOT_ACTIVE_1] = c;
        wp[WEAPON_SLOT_INACTIVE_1] = a;
        break;
    case KOZUKA:
    case NINJA:
    case KEITOU:
    case KATANA_0:
    case HOUTOU:
    case KATANA_1:
        d = wp[WEAPON_SLOT_INACTIVE_0];
        a = wp[WEAPON_SLOT_ACTIVE_0];
        wp[WEAPON_SLOT_ACTIVE_0] = d;
        wp[WEAPON_SLOT_INACTIVE_0] = a;
        break;
    }
}
