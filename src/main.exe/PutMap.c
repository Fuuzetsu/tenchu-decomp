#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void PutMap(void);
 *     INFOVIEW.C:1322, 65 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     reg   $s2       int rgb
 *     reg   $s1       struct POLY_XF4 * ply
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char PutMapMode;
 *     extern struct GsSPRITE MapImage;
 *     extern struct GsOT *OTablePt;
 *     extern int StageID;
 * END PSX.SYM */

/*
 * MATCH.
 *
 * PutMap (0x8004b848, 0x214 bytes) — draws the pause-menu map: a diamond
 * POLY_XF4 wipe outline (fresh each call from the GPU work buffer) plus the
 * MapImage sprite, animated through a 3-state sequence in `PutMapMode`
 * (gp-relative, this TU's own): 0 = just opened (init the wipe position/
 * sound), 1 = wipe animating closed (draws a highlighted-outline "seam"
 * frame each tick, then slides `D_80097F6C` toward 0 by 0x28/tick), 2 =
 * fully closed (draws the player's on-map marker via `FUN_8003d768`).
 *
 * `rgb` (PSX.SYM's declared local) is a brightness ramp derived from the
 * wipe position: `(0xA0 - D_80097F6C) / 4` — plain integer division by the
 * constant 4 (not a hand-written bias+shift): cc1 emits the
 * `if (t<0) t+=3; t>>=2;` guard automatically for INT division by a
 * power-of-2 constant (same lever as PutLifeBar's `mx/4`).
 *
 * `ply` is this call's POLY_XF4, borrowed from the GPU's rolling work
 * buffer (GsGetWorkBase/GsSetWorkBase, bumped by exactly `sizeof(POLY_XF4)`
 * via plain `ply + 1` pointer arithmetic) — same local `POLY_XF4`/`POLY_F4`/
 * `DR_TPAGE` layout shared in game_types.h.
 *
 * Ghidra renders `(CamState.Owner)->model->locate.coord.t[0/2]` for the
 * mode-2 draw call's first two args. The raw asm reaches that same aliased
 * cell through `CURRENTLY_SELECTED_CHARACTER_STATE_PTR[0]` (the
 * already-proven alias at 0x80089F00). `->model` is item.h's
 * `ModelArchiveType *model` @0x58, and
 * `.locate.coord.t[0]/.t[2]` are the MATRIX translation X/Z (offsets
 * 0x18/0x20 from `model`, matching the raw `lw 0x18(v1)`/`lw 0x20(v1)`).
 * The 3rd arg is `&D_8008E50C[StageID]` (a still-unnamed per-stage table,
 * stride 0x10). NOTE: D_8008E50C/D_80097F6C/D_80097F70 were only
 * auto-labeled by splat while PutMap's OWN asm carve referenced them; once
 * this file compiles as plain C, nothing else references them, so they
 * needed explicit `config/symbols.main.exe.txt` entries (same lever as
 * LifeBarStyle in PutLifeBar.c) — first hit here, worth remembering for
 * any OTHER TU where you're the last matcher touching a shared small.
 *
 * The dispatch on `PutMapMode` (values 0/1/2, no default) MUST be written
 * as a real `switch`, not an if/else-if ladder: only `switch` reproduces
 * the target's `lbu`-once + SIGNED `slti` compare (an if/else-if ladder
 * over this exact same u8 read compiles the "smart" UNSIGNED `sltiu`
 * instead — verified by isolated cc1 experiment; neither `signed char` nor
 * an explicit cast reproduces `lbu`+`slti` together, only genuine switch
 * lowering does). Case bodies are laid out in SOURCE order 0,1,2 — the
 * TEST order (1, then <2, then 2) differs from body order (cookbook
 * Dispatch: "case-body memory order reveals the source case order").
 * The x0/y0/x1/y1/x2/y2/x3/y3 POLY_XF4 field assignments must be
 * INTERLEAVED (x0,y0,x1,y1,x2,y2,x3,y3), not grouped (y0..y3 then x0..x3,
 * matching Ghidra's rendering) — grouped order ties two of the four
 * repeated-constant registers to the wrong class (a0/v1 swapped),
 * cascading through ~15 register-only diffs; interleaved order matches
 * the target exactly with no register diffs at all.
 *
 * The alias must be declared as an unknown-size pointer array and read via
 * `[0]`, as in create_ninken_character_. A scalar-pointer declaration leaves
 * `lw ...,CURRENTLY_SELECTED_CHARACTER_STATE_PTR` as one assembler pseudo-op,
 * so reorg instead puts `D_8008E50C`'s `%hi` producer in the case-2 test
 * branch delay slot; maspsx later expands the pointer load in the case body
 * and the resulting load-use hazard needs an extra `nop`. The array form makes
 * cc1 split the alias address itself. Reorg then hoists its `lui` into the
 * test delay slot and leaves the table `lui` to fill the model-pointer load
 * delay, removing the extra instruction and reproducing all 532 bytes.
 */
extern u8 PutMapMode;
extern GsSPRITE MapImage;
extern GsOT *OTablePt;
extern s32 StageID;
extern s32 D_80097F6C;
extern s32 D_80097F70;
extern Humanoid *CURRENTLY_SELECTED_CHARACTER_STATE_PTR[];
extern s32 D_8008E50C[][4];

extern void SetPolyXF4(POLY_XF4 *ply, short attrib);
extern short SoundEx(VECTOR *locate, short seid);
extern void FUN_8003d768(s32 x, s32 z, s32 *area);
extern void AddXF4(void *ot, POLY_XF4 *ply);

void PutMap(void)
{
    enum { size = 4096 };
    POLY_XF4 *ply;
    s32 rgb;

    ply = (POLY_XF4 *)GsGetWorkBase();
    GsSetWorkBase(ply + 1);
    SetPolyXF4(ply, 2);
    ply->ply.x0 = -0xA0;
    ply->ply.y0 = -0x78;
    ply->ply.x1 = 0xA0;
    ply->ply.y1 = -0x78;
    ply->ply.x2 = -0xA0;
    ply->ply.y2 = 0x78;
    ply->ply.x3 = 0xA0;
    ply->ply.y3 = 0x78;
    rgb = (0xA0 - D_80097F6C) / 4;

    switch (PutMapMode)
    {
    case 0:
        D_80097F6C = 0xA0;
        D_80097F70 = 0;
        PutMapMode = 1;
        ply->ply.r0 = 0;
        ply->ply.g0 = 0;
        ply->ply.b0 = 0;
        SoundEx((VECTOR *)0, 0x27);
        break;
    case 1:
        MapImage.r = 0x3C;
        MapImage.g = 0x3C;
        MapImage.b = 0x3C;
        MapImage.scalex = size;
        MapImage.scaley = size;
        MapImage.x = D_80097F6C;
        MapImage.y = D_80097F70;
        GsSortSprite(&MapImage, OTablePt, 2);
        MapImage.r = 0x80;
        MapImage.g = 0x80;
        MapImage.b = 0x80;
        ply->ply.r0 = rgb;
        ply->ply.g0 = rgb;
        ply->ply.b0 = rgb;
        D_80097F6C = D_80097F6C - 0x28;
        if (D_80097F6C <= 0)
        {
            PutMapMode = PutMapMode + 1;
        }
        break;
    case 2:
        D_80097F6C = 0;
        D_80097F70 = 0;
        ply->ply.r0 = rgb;
        ply->ply.g0 = rgb;
        ply->ply.b0 = rgb;
        FUN_8003d768(CURRENTLY_SELECTED_CHARACTER_STATE_PTR[0]->model->locate.coord.t[0],
                     CURRENTLY_SELECTED_CHARACTER_STATE_PTR[0]->model->locate.coord.t[2],
                     D_8008E50C[StageID]);
        break;
    }

    MapImage.scalex = size;
    MapImage.scaley = size;
    MapImage.x = D_80097F6C;
    MapImage.y = D_80097F70;
    GsSortSprite(&MapImage, OTablePt, 1);
    AddXF4((void *)((u8 *)OTablePt->org + 8), ply);
}
