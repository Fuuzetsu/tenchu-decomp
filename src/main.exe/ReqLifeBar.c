#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqLifeBar(struct Humanoid *h);
 *     INFOVIEW.C:89, 26 src lines, frame 0 bytes, saved-reg mask 0x00000000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $a0       struct Humanoid * h
 *     reg   $a1       int i
 *     reg   $a2       int g
 *
 * Globals it touches, as the original declared them:
 *     extern struct INFOVIEW__198fake LifeBar[4];
 * END PSX.SYM */

/*
 * ReqLifeBar (0x8004a5cc, 0xf0 bytes) — registers a Humanoid to get an
 * on-screen life bar: scans the 5-entry LifeBar[] pool for either an
 * already-assigned slot (`target == h`, breaks immediately) or, failing
 * that, the first free slot (`count < 1`); if neither is found the pool is
 * full and the function returns 0 without allocating.
 *
 * PSX.SYM records exactly TWO extra locals beyond the param — `int i`
 * ($a1, the loop counter) and `int g` ($a2) — NOT Ghidra's three-variable
 * (iVar2/iVar3/iVar4) rendering. The asm confirms a single register serves
 * as both "first free slot found so far" and "the final result": on a
 * match it's overwritten with the break index directly (no separate
 * iVar4), and the loop has no pointer-cursor local either — both the scan
 * (`LifeBar[i]`) and the post-loop fill (`LifeBar[g]`) are plain array
 * indexing (loop.c strength-reduces the scan into a walking pointer on its
 * own; the post-loop index is a fresh multiply since it's outside the
 * loop). One variable, one role — Ghidra's extra iVar4 is a decompiler
 * SSA-phi artifact of the "carry candidate through the loop bottom" idiom.
 *
 * `(s16)h->lifemax` casts a u16 field to force the signed `lh` this TU's
 * reads use (item.h documents this per-TU disagreement: ProcItemKusuri
 * reads it `lhu`, DoInfoViewProc/this file `lh` via the cast).
 *
 * Two structural levers beyond a literal Ghidra transcription:
 *  - `if (g == -1) goto ret_zero; ... return 1; ret_zero: return 0;` —
 *    the null-guard-with-two-returns exception (cookbook Dispatch): Ghidra's
 *    LITERAL polarity (guard first, `== -1`) is what matches, but ONLY when
 *    the guard's `return 0` is written to fall straight into the epilogue
 *    AFTER the whole success body (not inline right after the test) — the
 *    target's `beq` branches all the way to a `jr ra`/`move v0,zero` at the
 *    very end of the function, with the success path (fill + clamp +
 *    `return 1`) as the fallthrough.
 *  - `if (h->life == 0) { LifeBar[g].count = 100; } else {
 *    LifeBar[g].count = 300; }` — TWO separate `LifeBar[g].count = ...;`
 *    statements (one per arm), not a `cnt = 300; if (...) cnt = 100;
 *    LifeBar[g].count = cnt;` default-then-override temp. Ghidra renders the
 *    latter (one shared store fed by a temp), and it reproduces the
 *    100/300 selection correctly, but it also lets cc1 treat the following
 *    `LifeBar[g]` in `if (LifeBar[g].max < 1) LifeBar[g].max = 1;` as the
 *    SAME live address pseudo (0 extra instructions) — the real binary
 *    RECOMPUTES `&LifeBar[g]` from scratch (a fresh `lui/addiu` + the whole
 *    `sll/addu/sll/addu` multiply, 6 extra instructions, with the base/index
 *    registers swapped relative to the first computation) for that final
 *    check. Writing two independent per-arm stores (cross-jump-merged into
 *    one physical `sw` by the compiler, same as the target) is what makes
 *    the address pseudo feeding the max-check a genuinely fresh one again —
 *    a real but not fully root-caused cc1 cse/basic-block boundary effect
 *    tied to the branch structure, not merely a stylistic rewrite.
 */
typedef struct
{
    Humanoid *target; /* 0x00 */
    s32 life;          /* 0x04 */
    s32 max;           /* 0x08 */
    s32 count;         /* 0x0C */
    s32 style;         /* 0x10 */
} tag_LifeBarEntry;    /* 0x14 */

extern tag_LifeBarEntry LifeBar[5];

int ReqLifeBar(Humanoid *h)
{
    int i;
    int g;

    g = -1;
    for (i = 0; i < 5; i++) {
        if (LifeBar[i].count < 1) {
            if (g == -1) {
                g = i;
            }
        } else if (LifeBar[i].target == h) {
            g = i;
            break;
        }
    }
    if (g == -1) {
        goto ret_zero;
    }
    LifeBar[g].target = h;
    LifeBar[g].style = 1;
    LifeBar[g].life = h->life;
    LifeBar[g].max = (s16)h->lifemax;
    if (h->life == 0) {
        LifeBar[g].count = 100;
    } else {
        LifeBar[g].count = 300;
    }
    if (LifeBar[g].max < 1) {
        LifeBar[g].max = 1;
    }
    return 1;
ret_zero:
    return 0;
}
