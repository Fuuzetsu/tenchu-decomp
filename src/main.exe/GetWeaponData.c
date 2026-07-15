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
 * STATUS: NON_MATCHING — exact target extent (500 bytes / 125 instructions),
 * 32 differing bytes, fuzzy 92.00. Moving the WeaponDB search behind an
 * inline pointer/formal barrier separates its loop counter from the table
 * base and removes the old evacuation move; that entire first search now
 * matches byte-for-byte. The same treatment plus distinct `table`/`models`
 * aliases and a zero-code identical-arm fence recovers the second search's
 * target extent and base materialisation.
 *
 * The residual is confined to the second (WeaponModel) search's initial
 * compare schedule. The target tests row zero against -1 before copying the
 * table base and materialising `wid`; this compiler keeps the source while-
 * condition's `wid` comparison first. Writing an explicit sentinel precheck
 * lets cc1 prove away the rotated loop tail and makes the function six
 * instructions short, while returning a row pointer adds saved-register
 * pressure and makes it three instructions long. Keep this exact-length
 * checkpoint until a precheck spelling preserves the duplicated tail.
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
 *  - Each search uses a `short i` (not `int`, per PSX.SYM) — the narrow
 *    counter SUPPRESSES loop.c's strength reduction (Loops: "a
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

static inline s16 FindWeaponModel(WeaponModelType *models, s16 wid)
{
    s16 i;

    i = 0;
    while (models[i].wid != wid)
    {
        if (models[i].wid == -1)
        {
            break;
        }
        i++;
    }
    return i;
}

void GetWeaponData(Humanoid *human, s16 body, s16 wid, s16 wpid, int wep)
{
    s16 w;
    s16 i;
    u8 name[100];
    OrnamentType *base;
    WeaponModelType *models;
    WeaponModelType *table;

    w = (s16)wep;
    if (wpid >= 0)
    {
        FindWeaponId(human, wid, wpid);
    }

    if (w >= 0)
    {
        table = WeaponModel;
        if (wid != 0)
            models = table;
        else
            models = table;
        i = FindWeaponModel(table, wid);
        if (models[i].wid != -1)
        {
            if (models[i].model == 0)
            {
                sprintf(name, D_800117EC, D_800117F8, models[i].name);
                models[i].model = FileRead(name);
            }
            base = LoadOrnament(models[i].model);
            human->weapon[w] = base;
            GsInitCoordinate2((GsCOORDINATE2 *)human->model->object[body], &base->locate);
        }
    }
}

#endif /* NON_MATCHING */
