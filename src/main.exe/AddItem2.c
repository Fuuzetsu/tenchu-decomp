#include "common.h"
#include "main.exe.h"
#include "infoview.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void AddItem2(void);
 *     INFOVIEW.C:958, 27 src lines, frame 240 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     stack sp+24     struct PARAM_ITEM_STAY param
 *     reg   $s2       long x
 *     reg   $s0       long y
 *     reg   $s1       long z
 *     stack sp+24     struct TAdtSelect [25] ItemName
 *     stack sp+48     struct SVECTOR vec
 * END PSX.SYM */

/*
 * AddItem2 (0x8004afdc) — debug menu's "give item" action, called
 * from DoInfoViewProc's ItemLayoutMenu case 0 (see DoInfoViewProc.c, same
 * original TU, which uses the same menu-template/AdtSelect idiom). Prompts
 * for an item kind via AdtSelect, then places a
 * PARAM_ITEM_STAY on the ground a fixed distance in front of Owner
 * (rsin/rcos of model->rotate.vy projects the offset, GetAreaMapLevel snaps
 * it to the floor height) and kicks off a smoke puff at the spot.
 *
 * Matching notes (all verified against the bytes; see
 * docs/matching-cookbook.md):
 *  - PSX.SYM places ItemName and param at the same sp+24 stack slot, with
 *    vec after param's 24-byte compiler stack slot at sp+48. The local union
 *    exposes those exact original names and types while preserving that
 *    overlap; stack_slot_tail represents the four bytes between the 20-byte
 *    PARAM_ITEM_STAY and the next compiler slot.
 *    ItemName's fixed-size copy from DEBUG_MENU_ITEM_CHOICE_OPTIONS remains
 *    the target's 16-bytes-per-iteration word-block copy with an 8-byte tail,
 *    while vec's align-2 copy remains lwl/lwr + swl/swr.
 *  - `extern SVECTOR D_80097B88[];` (unknown-size array), NOT a plain
 *    SVECTOR: an 8-byte extern is -G8-small, so cc1 materializes its
 *    address as ONE `la` insn (assuming gp addressability; GAS -G0 expands
 *    it lui+addiu into the SAME register). The unknown-size declaration is
 *    non-small, giving the split HIGH/LO_SUM pair with TWO pseudos — the
 *    original's `lui v0,%hi / addiu t3,v0,%lo` (local-alloc gives the
 *    short-lived hi temp the first free reg, $v0).
 *  - GetAreaMapLevel's real prototype (see ReqItemDrop.c) takes 5 args
 *    (area,x,y,z,mode); Ghidra's rendering of this call site drops 2 of them
 *    (y and mode) even though the registers show all 5 (a0=area,
 *    a1=gx, a2=gy, a3=gz, stack=1).
 *  - GetAreaMapLevel's result h is NOT stored into locate.vy until inside
 *    the `if` body, alongside vx/vz; and gy (the t[1] load) is HOSTED in h
 *    first (`h = pm->locate.coord.t[1]; gy = h;`) — h's pseudo takes the
 *    lw and the callee-saved home $s0, gy coalesces into it, and after the
 *    call `h = GetAreaMapLevel(...)` reuses the same $s0 (gy is dead by
 *    then). Loading gy directly leaves h caller-saved and cascades ~50
 *    bytes of register drift through the whole tail (permuter find).
 *  - pm (`ModelArchiveType *pm = CamState.Owner->model;`) is re-read after
 *    each trig call, positioned BETWEEN the *1000 statement and the `if
 *    (x < 0)` fixup so the two loads interleave into the branch shadow the
 *    way the original schedules them.
 *  - rsin/rcos results need DISTINCT locals (sx, cx), not one reused temp
 *    like Ghidra's iVar7: the target has the sin-phase value in $a1 and the
 *    cos-phase value in $a3 (different regs), proving two variables.
 *  - x/z are real locals: computed once, interleaved into the following
 *    call's argument setup, and reused unmodified for the locate.vx/vz
 *    stores (Ghidra renders z's subtraction after the rcos call; the
 *    source order is sin-block then cos-block, each self-contained).
 */

extern char D_800124C0[];                   /* "select item" */
extern SVECTOR D_80097B88[];                /* smoke-puff velocity/offset const */

extern s32 AdtSelect(char *title, TAdtSelect *menu, s32 mode);

void AddItem2(void)
{
    s32 n;
    s32 sx, cx;
    s32 x, y, z;
    s32 h;
    ModelArchiveType *pm;
    union
    {
        TAdtSelect ItemName[25];
        struct
        {
            PARAM_ITEM_STAY param;
            u8 stack_slot_tail[4];
            SVECTOR vec;
        } spawn;
    } work;

    __builtin_memcpy(work.ItemName, DEBUG_MENU_ITEM_CHOICE_OPTIONS,
                     sizeof(work.ItemName));
    n = AdtSelect(D_800124C0, work.ItemName, 0);
    memset(&work.spawn.param, 0, sizeof(work.spawn.param));
    work.spawn.param.type = n;

    sx = rsin(CamState.Owner->model->rotate.vy) * 1000;
    pm = CamState.Owner->model;
    if (sx < 0)
        sx += 0xfff;
    h = pm->locate.coord.t[1];
    y = h;
    x = pm->locate.coord.t[0] - (sx >> 0xc);
    cx = rcos(pm->rotate.vy) * 1000;
    pm = CamState.Owner->model;
    if (cx < 0)
        cx += 0xfff;
    z = pm->locate.coord.t[2] - (cx >> 0xc);
    h = GetAreaMapLevel(GlobalAreaMap, x, y, z, 1);
    if (h != -0x80000000)
    {
        work.spawn.param.locate.vx = x;
        work.spawn.param.locate.vy = h;
        work.spawn.param.locate.vz = z;
        ReqItemStay(&work.spawn.param);
        work.spawn.vec = D_80097B88[0];
        SetSmoke(&work.spawn.param.locate, &work.spawn.vec, 3, 10);
    }
}
