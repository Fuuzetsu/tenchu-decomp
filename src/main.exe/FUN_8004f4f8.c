#include "common.h"
#include "main.exe.h"

/*
 * FUN_8004f4f8 (0x8004f4f8, 0xa0 bytes) — build/load a tile BackGround
 * (psxsym-types.h's proven layout: BackGround.map@0x24 holds a GsMAP whose
 * ncellw/ncellh sit at map+0x2/+0x4, i.e. BackGround+0x26/+0x28 — matches
 * tools/access.py exactly; index@0x3C, sz@0x40, both already proven by
 * DrawBG.c/DisposeBG.c's own truncated views) from a loaded TIM: reads the
 * TIM header (GetTIMInfo), builds the BG's cell/work buffers and its
 * index[] table pre-filled with 0xffff "empty" sentinels (SetupBG, already
 * matched), marks it "not yet drawn this frame" (sz=100, DrawBG-adjacent
 * convention), uploads the TIM pixels (LoadTIM), then overwrites every
 * index[] entry (one per map cell, ncellw*ncellh of them) with its own
 * linear index 0..n-1 — wiring each map cell to its own TIM region 1:1
 * instead of SetupBG's default "no cell assigned" state.
 *
 * Matching note: the raw .s recomputes `ncellw*ncellh` a SECOND time for the
 * do-while's own bottom test (a fresh lhu/lhu/mult/mflo, not a cached
 * value) — Ghidra's literal rendering (repeating the whole expression
 * instead of naming a local) is the real source shape here, not an SSA
 * artifact; caching it into a local drops those extra instructions.
 *
 * Called by CreateStage, BriefingAndInventorySelectionScreen (matched),
 * StageEndScreen and two other still-asm helpers — all places that load a
 * background image straight off a TIM buffer. No confirmed original name.
 */
extern BackGround *SetupBG(GsIMAGE *image, s16 w, s16 h);
extern void LoadTIM(unsigned long *adr);

BackGround *FUN_8004f4f8(u_long *tim)
{
    BackGround *bg;
    s32 i;
    GsIMAGE im;

    GetTIMInfo(tim, &im);
    bg = SetupBG(&im, 0x140, 0xf0);
    bg->sz = 100;
    LoadTIM(tim);
    i = 0;
    if (0 < bg->map.ncellw * bg->map.ncellh) {
        do {
            bg->index[i] = (u16)i;
            i = i + 1;
        } while (i < bg->map.ncellw * bg->map.ncellh);
    }
    return bg;
}
