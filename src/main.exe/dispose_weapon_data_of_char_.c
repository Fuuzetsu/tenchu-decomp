#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * STATUS: NON_MATCHING — dispose_weapon_data_of_char_ was never CARVED (no `c` subsegment), so its
 * .c was never linked and never actually compared: its bytes came from the
 * raw data blob and matchdiff reported a false MATCH. Carved now, and the
 * draft below does not reproduce the original. matchdiff/progress refuse to
 * count an un-carved function from now on.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/dispose_weapon_data_of_char_", dispose_weapon_data_of_char_);
#else
/*
 * dispose_weapon_data_of_char_ (0x800270c8) — NOT a null-checked disposer
 * despite the name: the raw asm has no branch at all. Same original TU as
 * NowReturnNormal.c (adjacent addresses, 0x80027004/0x800270c8; shares its
 * `extern Humanoid *Me_MOTION_C` gp global) — latches the character and its
 * motion manager into the module-global "current" pointers, then forwards
 * to AttackCancelControl. `a`'s int->short truncation (sll16/sra16 in the
 * jal's delay slot) comes from AttackCancelControl's prototype narrowing an
 * `int` argument to a `short` parameter, not from re-widening a short local.
 * The h->motion load schedules ahead of the Me_MOTION_C store (independent,
 * no alias) even though Ghidra/source order writes Me_MOTION_C first —
 * trust the source order, not the asm position (cookbook: independent loads
 * hoist above unrelated stores).
 */
extern Humanoid *Me_MOTION_C;
extern MotionManager *dtM;
extern void AttackCancelControl(s16 mode);

void dispose_weapon_data_of_char_(Humanoid *h, int a)
{
    Me_MOTION_C = h;
    dtM = h->motion;
    AttackCancelControl(a);
}
#endif /* NON_MATCHING */
