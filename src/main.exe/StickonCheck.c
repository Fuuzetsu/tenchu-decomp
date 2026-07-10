#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * STATUS: NON_MATCHING — 90 of 256 bytes differ, all from ONE reorg
 * delay-slot-fill tie (see below); every field offset/type this function
 * establishes is still correct (proven independently of the residual).
 *
 * StickonCheck (0x8001d374, 0x100 bytes) — "can the character stick to the
 * wall/ceiling here" probe: only for character types 0/1 with the area
 * attribute's top 2 bits clear (item.h's new `attrib` @0x28), it queries
 * GetAreaMapVector for the surface ahead of `dtL`, checks the SAME top-2-
 * bits guard on the query's own result, then (unless status 0xc/"already
 * sticking", or the surface's reflect-vector flags a no-stick edge/slope,
 * or its reflect value is the -1 sentinel) switches into the stick motion
 * (unless already status 0xc) and returns the query result; otherwise NULL.
 * Same original TU as dispose_weapon_data_of_char_.c/NowReturnNormal.c/
 * ReturnNormal.c/FUN_80027304.c (Me_MOTION_C, dtL, motID, D_80097F0E all
 * gp-relative here, matching those files' convention).
 *
 * `D_800BE750` (Ghidra's own data symbol table names it "map" — the
 * GetAreaMapVector out-parameter) and `D_80086774` (Ghidra: "RefrectVector",
 * a table of reflect-vector flag words) are declared under their raw
 * auto-generated names because that's what the assembled `.s` actually
 * references (splat only pulls FUNCTION names from the Ghidra export, not
 * arbitrary data symbols) — an extern spelled `RefrectVector` would not
 * link. `D_800BE750`'s type here is a local, offsets-only truncated view
 * (only +0x8/s16 and +0xC/u8 are touched by this function; GetAreaMapVector
 * itself writes a much larger 0x20-byte record, per its own Ghidra
 * decompilation, that this function never reads).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `(u16)Me_MOTION_C->type < 2`: item.h keeps `type` s16 (proven signed by
 *    GetHumanoid's `lh`); this TU reads it `lhu` — cast at the use site,
 *    same per-TU load-width disagreement as `lifemax`/`attrib`.
 *  - The whole function is nested guard clauses exactly as Ghidra renders
 *    them (each early exit falls straight to the shared epilogue with
 *    nothing else pending after it, so a plain `return 0;` per guard
 *    reproduces the asm's repeated `v0=0` delay-slot pattern with no
 *    special-casing needed) — proven for 4 of the 5 guards.
 *  - `idxval = D_80086774[D_800BE750.vector];` is a real STATEMENT before
 *    the `||`, not a comma-expression inside it: the raw asm loads it
 *    unconditionally (no side effects to gate), matching Ghidra's own
 *    comma-rendering of an eagerly-evaluated subexpression.
 *  - `(idxval & 0x200)` and `(s16)idxval != -1` share the SAME cached
 *    load (one `lhu`, reused, with an explicit re-sign-extend for the
 *    second test) — the established u16-field/signed-compare-cast pattern.
 *
 * THE RESIDUAL: the `if ((D_800BE750.attrib & 0xC000) != 0) return 0;` guard
 * right after the GetAreaMapVector call is byte-correct in every respect
 * (condition, operands, both branch targets) except which instruction fills
 * its delay slot. The target fills it with the return path's own `move
 * $v0,zero` (harmless either way, since $v0 is about to be overwritten on
 * the fallthrough path too); our build instead has reorg pull the
 * FALLTHROUGH's first instruction (`lui $v0,%hi(D_80086774)`, the next
 * statement's array-base load — data-independent and therefore an equally
 * "ready" candidate) into the slot, which then needs an extra explicit `j`
 * to reach the return path — a net +1 instruction that is exactly offset by
 * a *different* reorg choice much later (the final `status != 0xc` guard's
 * own delay slot picks up `li $v0,0xc00`, eliding a `nop` target keeps as
 * separate), so the whole-function length still matches (256 bytes both
 * sides) even though ~20 instructions in between sit at addresses shifted
 * by 4. Tried and reverted, no effect on this specific tie: swapping the
 * guard to Ghidra's literal (nested, non-early-return) polarity — this
 * restructured much more of the function AND got WORSE (67 differing bytes
 * but now with an actual length mismatch, because it also changed how
 * `Me_MOTION_C->status` CSEs across the two later reads); a named `u8 vec`
 * temp for `D_800BE750.vector` (zero effect, address-then-index evaluation
 * order is fixed by EXPAND_SUM regardless of source statement split); a
 * `do { guard } while (0);` wrapper around just this guard (zero effect on
 * the delay slot choice). `tools/autorules.py` found no improving edit
 * (u16/u8/u32/s16 retypes of `idxval`, `&&`-split all scored >= baseline).
 * `tools/permute.py StickonCheck -- --stop-on-zero -j4` ran a full bounded
 * ~6-minute / ~34,000-iteration pass (per the cookbook's sub-C-level
 * early-stop budget): the score never dropped below the 460 baseline —
 * flat, confirming this is a reorg/sched delay-slot-fill tie with no
 * source-level lever, the same permuter-immune signature as the `la`-reload
 * class (PrepareAccess/cd_open), just one mechanism over (delay-slot choice
 * between two equally-ready, equally-harmless candidate instructions rather
 * than a register pick). Parked per the cookbook's "do NOT open more
 * surgical sessions on it" guidance.
 */
typedef struct
{
    u8 pad0[0x8];
    s16 attrib; /* 0x8 */
    u8 pad1[0x2];
    u8 vector; /* 0xC */
} AreaMapVectorResult;

extern Humanoid *Me_MOTION_C;
extern VECTOR *dtL;
extern void *GlobalAreaMap;
extern u16 motID;
extern u16 D_80097F0E;
extern u16 D_80086774[];
extern AreaMapVectorResult D_800BE750;
extern s32 GetAreaMapVector(void *mvp, void *pos, s32 wide, s32 width, s32 flag);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/StickonCheck", StickonCheck);
#else
void *StickonCheck(void)
{
    u16 idxval;

    if ((u16)Me_MOTION_C->type >= 2)
    {
        return 0;
    }
    if ((Me_MOTION_C->attrib & 0xC000) != 0)
    {
        return 0;
    }
    GetAreaMapVector(GlobalAreaMap, &D_800BE750, (s32)dtL, Me_MOTION_C->width + 0x64, 5);
    if ((D_800BE750.attrib & 0xC000) != 0)
    {
        return 0;
    }
    idxval = D_80086774[D_800BE750.vector];
    if (Me_MOTION_C->status != 0xc && (idxval & 0x200) != 0)
    {
        return 0;
    }
    if ((s16)idxval == -1)
    {
        return 0;
    }
    if (Me_MOTION_C->status != 0xc)
    {
        motID = 0xc00;
        D_80097F0E = 1;
    }
    return &D_800BE750;
}
#endif
