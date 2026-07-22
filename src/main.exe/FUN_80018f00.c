#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern int AccessPower;
 *     extern struct POLY_GT4 AccessImage;
 * END PSX.SYM */

/*
 * FUN_80018f00 (0x80018f00, 0x110 bytes) — FILEIO.C's "stop the access
 * indicator" routine: disarms the vsync draw callback (`VSyncCallback(0)`,
 * the opposite of PrepareAccess's `VSyncCallback(cbAccess)`), and if the
 * meter was still armed (`AccessPower >= 0`) draws ONE final all-black
 * `AccessImage` quad to erase it from the screen before the callback stops
 * running — same DRAWENV/DISPENV swap-and-restore idiom as cbAccess.c (its
 * header comment names this file as the sibling to read for that shape:
 * `n_draw = o_draw;` is a plain DRAWENV struct assignment, `n_draw.clip =
 * o_disp.disp;` a plain RECT assignment, both matching cc1's emit_block_move
 * shape rather than Ghidra's field-by-field unrolling). Called from
 * FileRead.c only (once, straight-line, after the ReadMode dispatch); not in
 * the demo's PSX.SYM (a retail-only addition, unlike its FILEIO.C siblings
 * InitAccessInfo/cbAccess/PrepareAccess/FileRead which the demo already had).
 * No candidate name in reference/psxsym-candidates.tsv; not proposing one
 * without corroboration.
 *
 * MATCH. This function is cbAccess.c's twin and needs cbAccess.c's TWO
 * levers TOGETHER; an earlier checkpoint parked at 11 bytes because it
 * treated them as mutually exclusive alternatives.
 *
 * Two independent constraints must hold at once:
 *
 * 1. STACK SLOTS. cc1 gives each address-taken local a slot in DECLARATION
 *    order, ascending from the outgoing-arg boundary (sp+16), each rounded
 *    up to 8. With DRAWENV=92 and DISPENV=20 that makes the six orders pure
 *    arithmetic, and exactly ONE reaches the target's o_disp=16 / o_draw=40
 *    (n_draw=136):
 *        o_disp, o_draw, n_draw ->  16 /  40 / 136   <- target
 *        o_draw, o_disp, n_draw -> 112 /  16 / 136
 *        o_draw, n_draw, o_disp -> 208 /  16 / 112
 *        o_disp, n_draw, o_draw ->  16 / 136 /  40
 *        n_draw, o_draw, o_disp -> 208 / 112 /  16
 *        n_draw, o_disp, o_draw -> 112 / 136 /  16
 *    So slot order is DETERMINED, not searchable: `o_disp` must be declared
 *    first. (The 11-byte residual was only ever 2 free slots: the three
 *    `&o_draw` sites, `&o_disp` + its `disp` reads, and the block-move's
 *    LIMIT cursor at `&o_draw + 80` — sp+120 target / sp+96 ours — which is
 *    a derived cursor, not a fourth object. `n_draw` lands at 136 under both
 *    orders, which is why its address never appears in the diff.)
 *
 * 2. NO CSE MERGE. `&o_draw` is used 3 times (GetDrawEnv's arg, the
 *    `n_draw = o_draw;` block-move's source, and the final
 *    `PutDrawEnv(&o_draw)` restore). With `o_disp` declared first, cc1
 *    otherwise merges the first two uses — the ones either side of the
 *    single intervening `GetDispEnv()` call — into one value kept live in
 *    $s0, costing a save/restore (69 insns / 276 bytes, a LENGTH MISMATCH).
 *    The target instead recomputes `sp+40` fresh at all three sites in
 *    caller-saved regs ($a0, then $v0, then $a0 — the identical register
 *    pattern cbAccess.c documents).
 *
 * The parked checkpoint found that declaring `o_draw` FIRST also defeats the
 * merge, and concluded the two constraints were unsatisfiable together
 * ("EITHER the correct length XOR the correct slots"). But declaration order
 * is not the only lever on the merge, and it is the WRONG one — it is spoken
 * for by constraint 1. The merge has its own INDEPENDENT lever, already
 * proven in cbAccess.c: the identical `GetDispEnv(&o_disp)` call duplicated
 * into both arms of a test. Its control-flow boundary separates the first
 * `&o_draw` use from the block-move's source pointer during CSE; after
 * allocation and scheduling, jump cleanup cross-jumps the identical arms and
 * erases the test, so the final binary has no branch and no duplicate call.
 * With `o_disp` declared first (slots) PLUS the fence (no merge), both
 * constraints hold and the function matches exactly.
 *
 * The fence is load-bearing and verified so: unwrapping it to a single
 * `GetDispEnv(&o_disp)` call restores the $s0 merge and yields 276 bytes.
 * As in cbAccess.c the fence must sit on the SECOND call; the outer
 * `AccessPower >= 0` test already loads AccessPower, so the inner
 * `AccessPower != 0` test reuses that value via CSE and leaves nothing
 * behind once cross-jumping removes it.
 *
 * Note `tools/permute.py` scores this file's base 0 and writes no candidate —
 * its scorer does not penalize a pure stack-offset residual, so it is useless
 * on this class; `tools/matchdiff.py` was the only trustworthy signal.
 */
extern void VSyncCallback(void (*f)(void));
extern POLY_GT4 AccessImage;

void FUN_80018f00(void)
{
    DISPENV o_disp;
    DRAWENV o_draw;
    DRAWENV n_draw;

    VSyncCallback(0);
    if (AccessPower >= 0)
    {
        AccessImage.r0 = 0;
        AccessImage.g0 = 0;
        AccessImage.b0 = 0;
        AccessImage.r1 = 0;
        AccessImage.g1 = 0;
        AccessImage.b1 = 0;
        AccessImage.r2 = 0;
        AccessImage.g2 = 0;
        AccessImage.b2 = 0;
        AccessImage.r3 = 0;
        AccessImage.g3 = 0;
        AccessImage.b3 = 0;
        GetDrawEnv(&o_draw);
        if (AccessPower != 0)
            GetDispEnv(&o_disp);
        else
            GetDispEnv(&o_disp);
        n_draw = o_draw;
        n_draw.clip = o_disp.disp;
        n_draw.ofs[0] = o_disp.disp.x;
        n_draw.ofs[1] = o_disp.disp.y;
        PutDrawEnv(&n_draw);
        DrawPrim((u8 *)&AccessImage);
        PutDrawEnv(&o_draw);
    }
}
