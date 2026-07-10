#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void StartDrawing(void);
 *     3DCTRL.C:140, 6 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern short DrawingPage;
 *     extern struct GsOT OTable[2];
 *     extern struct GsOT *OTablePt;
 *     extern long GameClock;
 * END PSX.SYM */

/*
 * StartDrawing (0x800181d4, 0x74 bytes) — per-frame draw-page flip: toggles
 * the double-buffer page index, points the GPU work/packet area at the new
 * page (GsSetWorkBase, page stride 0x10000), repoints the global sort table
 * pointer OTablePt at OTable[DrawingPage] and clears it (GsClearOt), then
 * bumps the frame counter GameClock. Called once per frame from the main
 * loop alongside the (unmatched) present/flip step.
 *
 * DrawingPage/OTable/GameClock are Ghidra-recovered names (symbols.tsv);
 * D_80098040 (the GPU work/packet buffer immediately following OTable's two
 * entries — OTable+2*sizeof(GsOT) == 0x80098018+0x28 == 0x80098040) has no
 * Ghidra name, so it keeps splat's auto name. GsOT is only forward-declared
 * (opaque) in include/psxsdk/libgs.h ("only passed around by pointer" —
 * true of every OTHER matched caller); this is the first function that
 * needs its actual layout (to index OTable), so it's completed here from
 * the Ghidra type export (5 words, 0x14 bytes) — completing an
 * already-opaque tag locally, not inventing a new struct name.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - DrawingPage is signed `short` (Ghidra's own `(short)` cast on the
 *    store). Its two reads in this function don't CSE: `1 - DrawingPage`
 *    only feeds a narrowing store (`newPage` is itself `short`), so the
 *    sign bits of the load are provably dead and cc1 emits `lhu`; the
 *    later `OTable[DrawingPage]` index needs the full signed value and
 *    reloads fresh with `lh` (DeleteConflict's ConflictObjects rule: two
 *    un-CSE'd loads of one signed-short global, one lhu one lh — give the
 *    narrowing use its own temp and let the index re-read the global).
 *  - `(newPage << 16) + (s32)D_80098040`: EXPAND_SUM special-cases a MULT
 *    sub-term (always expands first, any source order) but NOT a shift —
 *    a shift preserves source order, so the shift is spelled first to land
 *    it as the addu's first source register, matching the target's
 *    `addu $a0,$v1(shift),$a0(addr)` (cookbook's fold/EXPAND_SUM section).
 *  - OTable/D_80098040 are ABSOLUTE (`lui`/`addiu` to %hi/%lo) in this TU,
 *    not %gp_rel — declared as unknown-size arrays (cookbook's "unknown
 *    size is non-small" trick) to force that; DrawingPage/OTablePt/
 *    GameClock ARE %gp_rel here (tools/gpsyms.py --write; Build.hs
 *    maspsxGpExterns + permute.py GP_EXTERNS both list StartDrawing now).
 *  - GsSetWorkBase/GsClearOt live above 0x80060000 (precompiled PsyQ SDK —
 *    don't source-shape their bodies, only this call site's argument
 *    setup).
 */

struct GsOT_TAG;

struct GsOT
{
    u32 length;
    struct GsOT_TAG *org;
    u32 offset;
    u32 point;
    struct GsOT_TAG *tag;
};

extern short DrawingPage;
extern GsOT OTable[];
extern GsOT *OTablePt;
extern s32 GameClock;
extern u8 D_80098040[];

extern void GsSetWorkBase(void *workBase);
extern void GsClearOt(s32 tx, s32 ty, GsOT *ot);

void StartDrawing(void)
{
    short newPage;

    newPage = 1 - DrawingPage;
    DrawingPage = newPage;
    GsSetWorkBase((void *)((newPage << 16) + (s32)D_80098040));

    OTablePt = &OTable[DrawingPage];
    GsClearOt(0, 0, OTablePt);

    GameClock += 1;
}
