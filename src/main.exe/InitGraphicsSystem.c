#include "common.h"
#include "main.exe.h"
#include "item.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitGraphicsSystem(void);
 *     3DCTRL.C:45, 29 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType World;
 *     extern struct GsFOGPARAM Fog;
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct GsOT OTable[2];
 *     extern struct GsOT_TAG ZSortTable[2][2048];
 * END PSX.SYM */

/*
 * InitGraphicsSystem (0x80016e94, 0x19c bytes) — one-time renderer bring-up,
 * called once from main(): mode-set the GPU (SetDispMask/GsInitGraph/
 * GsDefDispBuff/GsInit3D), root the World coordinate at the GPU's implicit
 * top (GsInitCoordinate2(0, &World.locate)) and compute its initial matrix
 * (UpdateCoordinate.c, matched — same TU), reset the TMD draw mode, program
 * the depth-cueing constants (SetDepthQ + DepthPoint/SlightPoint + Fog),
 * light/ambient/projection/view defaults (GsSetLightMode/GsSetAmbient/
 * GsSetProjection/GsSetRefView2/GsSetNearClip), load the debug font
 * (AdtFntLoad/AdtFntOpen.c, matched — same two prototypes reused verbatim),
 * set up the two Z-sort tables (OTable[0]/[1] pointing into ZSortTable's two
 * 0x800-entry halves), then seed the RNG from a VSync(-1) timestamp added
 * onto the existing STARTING_RNG_SEED.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - GsOT, GsOT_TAG, GsRVIEW2, and GsFOGPARAM use the canonical PsyQ LIBGS
 *    declarations, independently confirmed by PSX.SYM.
 *  - `SetDepthQ(-0x7ef4, 0x2f282e0)` and the later `Fog.dqa`/`Fog.dqb` stores
 *    use the SAME two numeric constants, but the asm rematerializes both
 *    from `$zero` a SECOND time after the call instead of keeping either
 *    call argument alive across it (two independent `addiu`/`lui+ori` pairs)
 *    — write them as bare literals at each site, not a shared named local;
 *    a named variable held live across the call would need a callee-saved
 *    register or a spill/reload, neither of which the asm shows.
 *  - `OTable[1].org = ZSortTable[1];` — ZSortTable is `[2][2048]`, so the
 *    second row's address is already `base + 2048*sizeof(GsOT_TAG)` ==
 *    `base + 0x2000` with no extra scaling; plain 2D indexing reproduces the
 *    asm's `addiu $v0,$v0,0x2000` with no source-level arithmetic needed.
 */

extern GsOT_TAG ZSortTable[2][2048];
extern s32 DepthPoint;
extern s32 SlightPoint;

/* STARTING_RNG_SEED (config/symbols.main.exe.txt: 0x80010e70) — read, added
 * to, and re-read within one statement pair straddling a call. A named
 * `extern s32` here compiles through the generic MIPS assembler pseudo-ops
 * (`lw $r,SYM`/`sw $r,SYM`), each independently expanded (own `lui`+fold,
 * one instruction longer, no delay-slot fill) because -G8 leaves an extern's
 * gp-eligibility undecided at compile time. A raw literal address is a
 * plain 32-bit constant with no such ambiguity, so cc1 commits to explicit
 * `lui+ori` constant-loading RTL that CSEs across the read/write and lets
 * reorg steal the store into the following call's delay slot — verified by
 * a standalone cc1 compile (docs/matching-cookbook.md: "an absolute global
 * read-modified-and-reread across a call wants a raw address literal, not
 * a named extern").
 */
#define STARTING_RNG_SEED (*(s32 *)TENCHU_PERSISTENT_RNG_ADDRESS)

extern void SetDepthQ(s32 dqa, s32 dqb);
extern void GsSetNearClip(s32 near);
extern void AdtFntLoad(int tx, int ty);
extern void AdtFntOpen(int x, int y, int w, int h, int isbg, int n);
extern s32 VSync(s32 mode);
extern void srand(u32 seed);

void InitGraphicsSystem(void)
{
    SetDispMask(0);
    GsInitGraph(0x140, 0xf0, 0x34, 1, 0);
    GsDefDispBuff(0, 0, 0, 0xf0);
    GsInit3D();
    GsInitCoordinate2((GsCOORDINATE2 *)0, &World.locate);
    UpdateCoordinate(&World);
    DrawTMDmode = 0;
    SetDepthQ(-0x7ef4, 0x2f282e0);
    DepthPoint = 0x4e2;
    SlightPoint = 0x96;
    Fog.dqa = -0x7ef4;
    Fog.dqb = 0x2f282e0;
    Fog.bfc = 0;
    Fog.gfc = 0;
    Fog.rfc = 0;
    GsSetFogParam(&Fog);
    GsSetLightMode(0);
    GsSetAmbient(0x800, 0x800, 0x800);
    GsSetProjection(300);
    ViewInfo.vpx = 0;
    ViewInfo.vpy = 0;
    ViewInfo.vpz = -1000;
    ViewInfo.vrx = 0;
    ViewInfo.vry = 0;
    ViewInfo.vrz = 0;
    ViewInfo.rz = 0;
    ViewInfo.super = &World.locate;
    GsSetRefView2(&ViewInfo);
    GsSetNearClip(0);
    AdtFntLoad(0x3c0, 0x100);
    AdtFntOpen(-0xa0, -0x68, 0x140, 0xd0, 0, 0x400);
    OTable[1].length = 0xb;
    OTable[0].length = 0xb;
    OTable[0].org = ZSortTable[0];
    OTable[1].org = ZSortTable[1];
    STARTING_RNG_SEED = STARTING_RNG_SEED + VSync(-1);
    srand(STARTING_RNG_SEED);
}
