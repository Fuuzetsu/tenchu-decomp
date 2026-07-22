#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void GetWeaponData(struct Humanoid *human, short body, short wid, short wpid, int wep);
 *     APPEAR.C:270, 25 src lines, frame 144 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $s1       struct Humanoid * human
 *     param $s3       short body
 *     param $t1       short wid
 *     param $a3       short wpid
 *     param stack+16  int wep
 *     reg   $s2       short wep
 *     reg   $a0       short i
 *     stack sp+16     unsigned char [100] name
 *
 * Globals it touches, as the original declared them:
 *     extern struct WeaponType WeaponDB[28];
 *     extern struct WeaponModelType WeaponModel[41];
 * END PSX.SYM */

/*
 * GetWeaponData (0x8002a290, 0x1f4 bytes) — two independent sentinel-
 * terminated linear searches feeding into `human`:
 *  1. WeaponDB[].ilup1.pad (SVECTOR's pad field, repurposed as an id) ==
 *     wid resolves an index stored into human->wepid[wpid] (item.h's
 *     proven `wepid[2]`@0x90, this function's own proof — see item.h).
 *  2. WeaponModel[].wid == wid resolves a row whose .model (lazily
 *     FileRead of "%s%s.TMD" formatted from the fixed
 *     "K:\WORK\CDIMAGE\HUMAN\WEAPON\" prefix + the row's .name) backs a
 *     LoadOrnament call; the result is stored to human->weapon[wep] and
 *     wired via GsInitCoordinate2 to human->model->object[body].
 * Ghidra completely lost the 5th (stack-passed) parameter `int wep`,
 * rendering it as an unnamed `in_stack_00000010` — PSX.SYM's prototype
 * recovers it (a known Ghidra weak spot: stack-passed args past the 4
 * register ones). `wep` is only ever used narrowed to its low 16 bits
 * (loaded `lhu` straight off the stack slot, per the Expressions
 * "narrowing use reads even a signed global with lhu" rule) — reflected
 * here as a `short` local, matching PSX.SYM's `reg $s2 short wep`.
 *
 * Both searches share the SAME idiom, matching PSX.SYM's single `reg $a0
 * short i` (one counter, not one per search): `while (Table[i].sentinel_field
 * != -1) { if (Table[i].key == wid) { ...; break; } i++; }` — the loop's
 * OWN controlling test is the -1 sentinel (duplicated at entry and at the
 * bottom by loop rotation); the `wid` compare is the inner break, never the
 * while-condition. Getting these two roles backwards (testing `wid` as the
 * while-condition, `-1` as the inner break) still reaches the exact target
 * LENGTH but materialises `wid` before the entry sentinel test instead of
 * after, a 32-byte residual confined to the second search's opening
 * instructions.
 *
 * The second search additionally needs its post-loop found-check
 * (`WeaponModel[i].wid != -1`) INLINE in GetWeaponData rather than behind a
 * helper function that returns `i` — with a real call/return boundary in the
 * way, cc1's jump optimizer cannot thread the loop's entry test (which is
 * RTL-identical to the post-loop check when `i` is still 0) straight through
 * to the shared "not found" label the way it does when both tests are
 * visible in one flat function; it instead re-lands inside the loop-exit
 * merge and recomputes `&WeaponModel[i]`, which is both longer and wrong.
 * The first search doesn't need this — it has no post-loop check, so a
 * `static inline` helper (FindWeaponId) reaches the target fine; only the
 * second search's helper had to be flattened into GetWeaponData itself.
 *
 * `wep` narrows to `short w` once at entry (PSX.SYM's `reg $s2 short wep` —
 * a repeated name over the `int wep` parameter, i.e. a storage-class split,
 * not a second source declaration: a literal `short wep = (short)wep;`
 * would self-reference its own uninitialized declarator in C). `base`
 * (OrnamentType *) stays live in $v0 across the `human->weapon[w] = base;`
 * store so GsInitCoordinate2's second argument (`&base->locate`, offset 0)
 * reads it directly instead of reloading through human->weapon[w].
 */

extern char D_800117EC[]; /* "%s%s.TMD" */
extern char D_800117F8[]; /* "K:\\WORK\\CDIMAGE\\HUMAN\\WEAPON\\" */

extern int sprintf(char *buf, char *fmt, ...);
extern OrnamentType *LoadOrnament(u_long *adr);

static inline void FindWeaponId(Humanoid *human, s16 wid, s16 wpid)
{
    s16 i;

    i = 0;
    while (WeaponDB[i].ilup1.pad != -1)
    {
        if (WeaponDB[i].ilup1.pad == wid)
        {
            human->wepid[wpid] = i;
            break;
        }
        i++;
    }
}

void GetWeaponData(Humanoid *human, s16 body, s16 wid, s16 wpid, int wep)
{
    s16 w;
    s16 i;
    u8 name[100];
    OrnamentType *base;

    w = (s16)wep;
    if (wpid >= 0)
    {
        FindWeaponId(human, wid, wpid);
    }

    if (w >= 0)
    {
        i = 0;
        while (WeaponModel[i].wid != -1)
        {
            if (WeaponModel[i].wid == wid)
            {
                break;
            }
            i++;
        }
        if (WeaponModel[i].wid != -1)
        {
            if (WeaponModel[i].model == 0)
            {
                sprintf(name, D_800117EC, D_800117F8, WeaponModel[i].name);
                WeaponModel[i].model = FileRead(name);
            }
            base = LoadOrnament(WeaponModel[i].model);
            human->weapon[w] = base;
            GsInitCoordinate2((GsCOORDINATE2 *)human->model->object[body], &base->locate);
        }
    }
}
