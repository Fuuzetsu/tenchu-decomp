#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/*
 * stop_access_meter_ (0x80018f00, 0x110 bytes) — FILEIO.C's "stop the access
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
 * STATUS: MATCHING. Two independent constraints from cbAccess must coexist:
 *
 *  - Declare o_disp, o_draw, then n_draw. Address-taken locals receive
 *    declaration-order stack slots, giving the target offsets 16, 40, and 136.
 *    Changing declaration order may affect CSE, but necessarily breaks those
 *    slots.
 *  - Keep the identical GetDispEnv(&o_disp) call in both arms of the
 *    AccessPower != 0 test. This second-call control-flow boundary prevents
 *    the first two &o_draw uses from merging across the call; jump cleanup
 *    later erases the test and duplicate arm. A single call adds an $s0
 *    save/restore and grows the function from 272 to 276 bytes.
 *
 * The inner test deliberately reuses the AccessPower value already loaded by
 * the outer guard, so the fence leaves no branch residue. Stack order solves
 * object placement; the duplicated-call fence independently solves address
 * liveness. Neither is an alternative for the other.
 */
extern void VSyncCallback(void (*f)(void));

void stop_access_meter_(void)
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
