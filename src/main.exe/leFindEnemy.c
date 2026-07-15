#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int leFindEnemy(void);
 *     WORLD.C:1099, 31 src lines, frame 88 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
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
 *     reg   $s1       int i
 *     reg   $s6       long px
 *     reg   $s5       long py
 *     reg   $s4       long pz
 *     reg   $s3       int find
 *     reg   $s2       int r
 *     reg   $v1       int rr
 *     stack sp+16     struct SVECTOR pow
 *     stack sp+24     struct VECTOR pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct TEnemyLayout enemy[30];
 * END PSX.SYM */

/*
 * leFindEnemy (0x8003c470, 0x1a4 bytes) — `le`=layout-enemy family (see
 * leAddPath.c/leResetPath.c for TEnemyLayout): scans the 30-slot `enemy[]`
 * table for the live (`type != -1`) entry nearest CamState.Owner's model
 * position, returning its index (or -1 if none is closer than the initial
 * 2000-unit cutoff). On a hit, spawns the same marker explosion effect as
 * leAddPath (SetExplosion with a zeroed rotation and the found enemy's own
 * x/y/z) at the found enemy's position.
 *
 * Matching notes:
 *  - `local_48 = D_80097ABC[0];` (the whole-SVECTOR copy) is computed BEFORE
 *    the `memset` call in source, not after — same pooled rodata constant
 *    as leAddPath.c, same lwl/lwr+swl/swr block-copy shape.
 *  - The zeroed/filled VECTOR is a SEPARATE staging local from the one
 *    passed to SetExplosion: `local_40` gets memset then vx/vy/vz filled
 *    from `enemy[find]`, and `pos = local_40;` (a whole 4-word struct copy,
 *    including the untouched zero `pad`) is what SetExplosion actually
 *    receives — not a fused "memset+fill one local" shape. PSX.SYM's demo
 *    build only names the one surviving local (`pos`); retail keeps the
 *    staging copy.
 *  - `enemy[find]`'s address is computed as `find*17*8` (`(find<<4)+find,
 *    then <<3`), the compiler's own strength-reduced multiply by
 *    sizeof(TEnemyLayout)==0x88 — write plain `&enemy[find]`, not a manual
 *    shift/add, and let cc1 pick this multiply sequence itself.
 */

typedef struct
{
    s16 type;
    s16 ThinkType;
    s16 nPath;
    s32 x;
    s32 y;
    s32 z;
    s16 r;
    s16 pad;
    VECTOR path[7];
} TEnemyLayout; /* 0x88 */

typedef struct
{
    VECTOR TargetVector; /* 0x00 */
    Humanoid *Owner;     /* 0x10 */
    s32 Mode;            /* 0x14 */
    s16 DirectionRX;     /* 0x18 */
    s16 DirectionRY;     /* 0x1A */
    s32 OldMode;         /* 0x1C */
    u8 Valiation;        /* 0x20 */
} TCameraStatus;

extern TCameraStatus CamState;
extern TEnemyLayout enemy[0x1E];
extern SVECTOR D_80097ABC[];

extern s32 SquareRoot0(s32 x);
extern void SetExplosion(VECTOR *pos, SVECTOR *rot);
extern void *memset(void *s, s32 c, u32 n);

int leFindEnemy(void)
{
    int i;
    s32 px, py, pz;
    int find;
    int r;
    int rr;
    int dx, dy, dz;
    SVECTOR local_48;
    VECTOR pos;
    VECTOR local_40;

    find = -1;
    r = 2000;
    px = CamState.Owner->model->locate.coord.t[0];
    py = CamState.Owner->model->locate.coord.t[1];
    pz = CamState.Owner->model->locate.coord.t[2];

    i = 0;
    while (1)
    {
        if (!(i < 0x1E))
            break;
        if (enemy[i].type != -1)
        {
            dx = enemy[i].x - px;
            dy = enemy[i].y - py;
            dz = enemy[i].z - pz;
            rr = SquareRoot0(dx * dx + dy * dy + dz * dz);
            if (rr < r)
            {
                find = i;
                r = rr;
            }
        }
        i = i + 1;
    }

    if (find != -1)
    {
        local_48 = D_80097ABC[0];
        memset((void *)&local_40, 0, 0x10);
        local_40.vx = enemy[find].x;
        local_40.vy = enemy[find].y;
        local_40.vz = enemy[find].z;
        pos = local_40;
        SetExplosion(&pos, &local_48);
    }

    return find;
}
