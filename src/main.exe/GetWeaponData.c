#include "common.h"
#include "main.exe.h"
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
 * STATUS: NON_MATCHING — 24 of 500 bytes differ (asmdiff: 35 differing
 * lines in 16 blocks, function 1 instruction too long: 126 vs 125). Every
 * block/branch/instruction is structurally right (loop shapes, dispatch,
 * field offsets, calls all confirmed against the raw .s); the residual is
 * a pure register-allocation tie in the FIRST (WeaponDB) search: the
 * target keeps the loop counter `i` in one register and the WeaponDB base
 * address in another (a1) for the whole loop with no extra move, while
 * this draft's cc1 invocation puts the freshly-materialised WeaponDB base
 * in the SAME register cc1 also wants for `i`'s next value, forcing an
 * extra `move` to evacuate it first. Tried: swapping `i`/`w` declaration
 * order (no effect — pure local order doesn't reach this tie), moving the
 * `w = (short)wep` narrowing statement before/after the first search
 * (moving it later made things WORSE, 127 vs 125 — an unrelated `lw`
 * of the raw stack int got scheduled early instead of the intended `lhu`
 * narrowing read). `tools/autorules.py` found no improving edit (8
 * candidates, all net negative). One bounded `tools/permute.py` run
 * (~5 min, -j4, several hundred iterations across many candidates)
 * bottomed out at score 660 — never near 0 — so this is parked per the
 * cookbook's sub-C-level early-stop, not chased further.
 */

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
 * Matching notes (docs/matching-cookbook.md):
 *  - `short i` (not `int`, per PSX.SYM) reused across BOTH searches — the
 *    narrow counter SUPPRESSES loop.c's strength reduction (Loops: "a
 *    short loop counter suppresses loop.c's strength reduction"), so the
 *    target recomputes `&WeaponDB[i]`/`&WeaponModel[i]` from scratch each
 *    iteration (sign-extend refused into the scale) instead of walking a
 *    pointer — matches plain re-indexed `WeaponDB[i]`/`WeaponModel[i]`
 *    with NO named temp for the compared field (an explicit temp, as
 *    SetupCharacterParameter's `int idx` search needed, is wrong here:
 *    it would make cc1 hoist the loop's first-iteration test into an
 *    extra branch — that trick is specific to the `int` biv/giv case).
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/GetWeaponData", GetWeaponData);
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/GetWeaponData", load_weapon_model__override__prt_8002a418_aee7b64a);
#else /* NON_MATCHING */

typedef struct
{
    SVECTOR confp; /* 0x0 */
    SVECTOR ilup0; /* 0x8 */
    SVECTOR ilup1; /* 0x10 */
} WeaponType;      /* 0x18 */

typedef struct
{
    u8 *name;   /* 0x0 */
    s16 wid;    /* 0x4 */
    u32 *model; /* 0x8 */
} WeaponModelType; /* 0xC */

struct OrnamentType
{
    GsCOORDINATE2 locate; /* 0x00 */
    GsDOBJ2 object;       /* 0x50 */
};

extern WeaponType WeaponDB[28];
extern WeaponModelType WeaponModel[41];
extern char D_800117EC[]; /* "%s%s.TMD" */
extern char D_800117F8[]; /* "K:\\WORK\\CDIMAGE\\HUMAN\\WEAPON\\" */

extern u32 *FileRead(u8 *filename);
extern int sprintf(char *buf, char *fmt, ...);
extern OrnamentType *LoadOrnament(u32 *adr);
extern void GsInitCoordinate2(GsCOORDINATE2 *super, GsCOORDINATE2 *base);

void GetWeaponData(Humanoid *human, s16 body, s16 wid, s16 wpid, int wep)
{
    s16 w;
    s16 i;
    u8 name[100];
    OrnamentType *base;

    w = (s16)wep;
    if (wpid >= 0)
    {
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

    if (w >= 0)
    {
        i = 0;
        while (WeaponModel[i].wid != wid)
        {
            if (WeaponModel[i].wid == -1)
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

#endif /* NON_MATCHING */
