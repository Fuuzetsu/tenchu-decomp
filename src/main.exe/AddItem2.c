#include "common.h"
#include "main.exe.h"
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
 * original TU: CamState/TCameraStatus and the MENU_ITEM_TBL/AdtSelect idiom
 * are duplicated here as this TU's own file-scope declarations, same as
 * there). Prompts for an item kind via AdtSelect, then places a
 * PARAM_ITEM_STAY on the ground a fixed distance in front of Owner
 * (rsin/rcos of model->rotate.vy projects the offset, GetAreaMapLevel snaps
 * it to the floor height) and kicks off a smoke puff at the spot.
 *
 * Matching notes (all verified against the bytes; see
 * docs/matching-cookbook.md):
 *  - `mi` is one u8[0xC8] buffer reused three ways, the same buffer+casts
 *    idiom as ProcItemKusuri: first the whole-struct-assignment destination
 *    for `*(MENU_ITEM_TBL *)mi = DEBUG_MENU_ITEM_CHOICE_OPTIONS` (a
 *    word-block copy: the 16-bytes-per-iteration loop + a 8-byte tail,
 *    the cookbook's align-4 struct-copy rule — this is what Ghidra
 *    mis-renders as a field-by-field "copy loop" assigning unrelated
 *    PARAM_ITEM_STAY fields from adt_menu_entry_t label/index pairs), then
 *    a PARAM_ITEM_STAY (first 0x14 bytes, after memset), then an SVECTOR at
 *    mi+0x18 (align-2 copy from D_80097B88, so lwl/lwr + swl/swr).
 *  - `extern SVECTOR D_80097B88[];` (unknown-size array), NOT a plain
 *    SVECTOR: an 8-byte extern is -G8-small, so cc1 materializes its
 *    address as ONE `la` insn (assuming gp addressability; GAS -G0 expands
 *    it lui+addiu into the SAME register). The unknown-size declaration is
 *    non-small, giving the split HIGH/LO_SUM pair with TWO pseudos — the
 *    original's `lui v0,%hi / addiu t3,v0,%lo` (local-alloc gives the
 *    short-lived hi temp the first free reg, $v0).
 *  - GetAreaMapLevel's real prototype (see ReqItemDrop.c) takes 5 args
 *    (map,x,y,z,flag); Ghidra's rendering of this call site drops 2 of them
 *    (y and flag) even though the registers show all 5 (a0=map,
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
 *  - gx/gz are real locals: computed once, interleaved into the following
 *    call's argument setup, and reused unmodified for the locate.vx/vz
 *    stores (Ghidra renders gz's subtraction after the rcos call; the
 *    source order is sin-block then cos-block, each self-contained).
 */

typedef struct { TAdtSelect e[25]; } MENU_ITEM_TBL;    /* 0xC8 */

extern TCameraStatus CamState;
extern MENU_ITEM_TBL DEBUG_MENU_ITEM_CHOICE_OPTIONS;
extern char D_800124C0[];                   /* "select item" */
extern SVECTOR D_80097B88[];                /* smoke-puff velocity/offset const */

extern s32 AdtSelect(char *title, TAdtSelect *menu, s32 mode);
extern long GetAreaMapLevel(void *map, long x, long y, long z, long e);

void AddItem2(void)
{
    s32 n;
    s32 sx, cx;
    s32 gx, gy, gz;
    s32 h;
    ModelArchiveType *pm;
    u8 mi[0xC8];

    *(MENU_ITEM_TBL *)mi = DEBUG_MENU_ITEM_CHOICE_OPTIONS;
    n = AdtSelect(D_800124C0, (TAdtSelect *)mi, 0);
    memset(mi, 0, sizeof(PARAM_ITEM_STAY));
    ((PARAM_ITEM_STAY *)mi)->type = n;

    sx = rsin(CamState.Owner->model->rotate.vy) * 1000;
    pm = CamState.Owner->model;
    if (sx < 0)
        sx += 0xfff;
    h = pm->locate.coord.t[1];
    gy = h;
    gx = pm->locate.coord.t[0] - (sx >> 0xc);
    cx = rcos(pm->rotate.vy) * 1000;
    pm = CamState.Owner->model;
    if (cx < 0)
        cx += 0xfff;
    gz = pm->locate.coord.t[2] - (cx >> 0xc);
    h = GetAreaMapLevel(GlobalAreaMap, gx, gy, gz, 1);
    if (h != -0x80000000)
    {
        ((PARAM_ITEM_STAY *)mi)->locate.vx = gx;
        ((PARAM_ITEM_STAY *)mi)->locate.vy = h;
        ((PARAM_ITEM_STAY *)mi)->locate.vz = gz;
        ReqItemStay((PARAM_ITEM_STAY *)mi);
        *(SVECTOR *)(mi + 0x18) = D_80097B88[0];
        SetSmoke(&((PARAM_ITEM_STAY *)mi)->locate, (SVECTOR *)(mi + 0x18), 3, 10);
    }
}
