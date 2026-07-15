#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long * LoadAreaMap(unsigned long *adr);
 *     CONFLICT.C:47, 23 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
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
 *     param $a0       unsigned long * adr
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct NodeIndexType *FieldIndex;
 *     extern struct AreaNodeType *FieldArea;
 * END PSX.SYM */

/*
 * LoadAreaMap (0x800197a4) — installs a freshly-loaded area-map blob (`adr`)
 * as the live map: relocates every NodeIndexType record's `index` field (an
 * on-disk offset -> absolute pointer) in place, bumps each record's `y` by 2,
 * and — for a subdivided ("division") entry, `n < 0` — also relocates the
 * AreaDivisionType's own leading `area` pointer one level down. Same TU as
 * GetAreaMapLevel.c/character_balma_around_main_routine_.c (shares
 * NodeIndexType/AreaNodeType and the GlobalAreaMap/FieldIndex/FieldArea
 * globals — all %gp_rel here too). Callers: FUN_8001ab64 and LoadConstruction
 * (`GlobalAreaMap = LoadAreaMap(PathFileRead(...));`).
 *
 * Matching notes:
 *  - `adr` (the relocation base) and `map` (the walk cursor) are the SAME
 *    value but occupy TWO callee-saved registers ($s0/$s1) — an explicit
 *    source copy (`map = adr;`), not one variable: the two relocation-fixup
 *    additions (`map[j].index += ...`, the division's `*(long*)map[j].index
 *    += ...`) both add through `adr` ($s0), while the cursor arithmetic
 *    (`map[j]`, and the loop-continuation `map[j+1]`) goes through `map`
 *    ($s1) — confirmed by which register the asm's `addu` operands name at
 *    each site. Repeating `map[j]` at each field access (no intermediate
 *    `rec` cursor pointer) matters too: assigning `&map[j]` to a named
 *    pointer temp first flips the `addu` operand order (base-first, the
 *    struct-pointer-field shape) versus the target's index-first order,
 *    which only falls out of the un-cached repeated-expression form (cc1's
 *    EXPAND_SUM special-cases a mult subterm to expand first UNLESS the sum
 *    is captured in a temp — see the cookbook's address-expression rule).
 *  - The loop counter `j` is `s16`: `map[j]` (NodeIndexType is 16 bytes)
 *    compiles to a combined sign-extend+scale `sll 16/sra 12` pair (same
 *    idiom as GetAreaMapLevel's qx/qz), not a plain `int` multiply/shift —
 *    Ghidra's literal `(iVar2 << 0x10) >> 0xc` rendering is that same
 *    decompiled-pointer-arithmetic artifact, not the real source spelling.
 *  - `map[j].y = map[j].y + 2;` is a narrowing use (result stored right back
 *    as s16), so it loads via `lhu` even though `y` is signed elsewhere
 *    (GetAreaMapLevel reads the same field with `lh` for a real compare) —
 *    same-field, different-op, different-TU-use divergence per the cookbook.
 *  - Neither Ghidra nor m2c has this right: there is only ONE store to
 *    FieldArea, AFTER the whole if-block (not "= 0" before it, "= adr->index"
 *    inside it, as both renderings show) — a plain local (`idx0`) captures
 *    `adr->index` before the guard (that same read IS the guard test) and
 *    again after the loop (freshly relocated by then; iteration j=0 aliases
 *    `adr` itself, so the loop's own first pass rewrites `adr->index` in
 *    place).
 *  - Getting the VALUE right wasn't enough to fix the final block's
 *    INSTRUCTION ORDER — `GlobalAreaMap`/`FieldIndex` (both plain `adr`,
 *    ready immediately) versus `FieldArea`/the return value (both need
 *    `idx0`/`adr` massaged) kept scheduling as [FieldArea, return-move,
 *    GlobalAreaMap, FieldIndex] instead of the target's [GlobalAreaMap,
 *    FieldIndex, FieldArea, return-move] no matter how the 4 statements were
 *    reordered in source (tried every permutation by hand — zero effect on
 *    the emitted order, so it isn't a source-order lever at all here).
 *    decomp-permuter found the actual fix: one MORE `idx0 = adr->index;`
 *    right before `FieldArea = (AreaNodeType *)idx0;` — textually a third,
 *    redundant read (CSE erases it as an instruction; the asm only ever has
 *    the same two physical `lw`s) that exists purely to re-timestamp idx0's
 *    pseudo late enough in cc1's bookkeeping to win the scheduling tie. Byte-
 *    neutral re-reads as permuter seed levers are a known lever (cookbook,
 *    Register allocation steering) — this is that mechanism applied to a
 *    whole-statement re-read rather than a single cached-field re-read.
 *  - SystemOut is annotated by Ghidra as noreturn, but the compiled code
 *    falls straight through after the call (plain `if (adr == 0)
 *    SystemOut(...);` with no early return) — matches InsertConflict.c's
 *    identical SystemOut-then-continue shape.
 *  - "NO AREA DATA" is D_800111E8, 16 bytes before InsertConflict.c's
 *    D_800111F8 ("CONFLICT REGIST FAILURE") — same rodata blob; reference it
 *    via `extern char D_800111E8[];` (a fresh string literal here would open
 *    a NEW .rodata this linker script has no slot for — see AfsInit.c).
 */

typedef struct NodeIndexType
{
    s16 y;      /* 0x0 */
    s16 n;      /* 0x2 */
    long index; /* 0x4 — AreaNodeType* / AreaDivisionType*; 0 terminates */
    s16 x1;     /* 0x8 */
    s16 z1;     /* 0xA */
    s16 x2;     /* 0xC */
    s16 z2;     /* 0xE */
} NodeIndexType;

typedef struct AreaNodeType AreaNodeType; /* opaque here — only the pointer is used */

extern void *GlobalAreaMap;
extern NodeIndexType *FieldIndex;
extern AreaNodeType *FieldArea;

extern char D_800111E8[]; /* "NO AREA DATA" */
extern void SystemOut(char *);

NodeIndexType *LoadAreaMap(NodeIndexType *adr)
{
    NodeIndexType *map;
    s16 j;
    long idx0;

    map = adr;
    if (adr == 0)
        SystemOut(D_800111E8);

    j = 0;
    idx0 = adr->index;
    if (idx0 != 0)
    {
        do
        {
            map[j].index = map[j].index + (long)adr;
            map[j].y = map[j].y + 2;
            if (map[j].n < 0)
            {
                *(long *)map[j].index = *(long *)map[j].index + (long)adr;
            }
            j++;
        } while (map[j].index != 0);
        idx0 = adr->index;
    }
    GlobalAreaMap = adr;
    FieldIndex = adr;
    idx0 = adr->index;
    FieldArea = (AreaNodeType *)idx0;
    return adr;
}
