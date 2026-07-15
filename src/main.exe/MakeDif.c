#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void MakeDif(struct GsRVIEW2 *vinfo, struct GsRVIEW2 *target, struct GsRVIEW2 *vdif);
 *     CAMERA.C:418, 8 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     param $a0       struct GsRVIEW2 * vinfo
 *     param $a1       struct GsRVIEW2 * target
 *     param $a2       struct GsRVIEW2 * vdif
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

/*
 * MakeDif (0x80032088, 0xfc bytes) — computes vdif = target - vinfo for a
 * GsRVIEW2 camera view, gated by CamState's "just snapped" one-shot flag
 * (byte 1 of the OldMode enum, per Ghidra's own `._1_1_`): a straight
 * 6-field s32 subtraction the first frame after a mode change (and clears
 * the flag), otherwise a smoothed delta via two MakeDifSub calls — one over
 * the rotation-only half (vrx..vrz) using a TMakeDifInfo scratch block that
 * sits right after CamState in memory (D_80089F10 = CamState + 0x20; Ghidra
 * mis-renders this as `&CamState.Valiation`, a field cast past
 * TCameraStatus's own recorded 0x24-byte size — it's really a separate
 * static, its address just materializes as its own `lui`/`addiu`, never
 * derived from CamState's already-loaded base register), one over the full
 * 6-field view using the already-named `pnt` global.
 *
 * TCameraStatus's field order for OldMode's position is Ghidra's own
 * independently-built struct (reference/ghidra_types.h), NOT psxsym-types.h
 * (which puts DirectionRX/RY AFTER OldMode instead of before it) — the raw
 * `.s`'s `lbu $v1, 0x1D($a0)` off CamState's base settles it: byte 0x1D is
 * OldMode's high byte only if DirectionRX(0x18)/DirectionRY(0x1A) precede
 * OldMode(0x1C), matching Ghidra, not psxsym.
 */
typedef struct
{
    s32 vpx;   /* +0x00 */
    s32 vpy;   /* +0x04 */
    s32 vpz;   /* +0x08 */
    s32 vrx;   /* +0x0C */
    s32 vry;   /* +0x10 */
    s32 vrz;   /* +0x14 */
    s32 rz;    /* +0x18 */
    void *super; /* +0x1C */
} GsRVIEW2;

/* Truncated to the one byte MakeDif actually touches (byte 1 of the
 * 4-byte OldMode enum at +0x1C) — this repo's per-TU local-view
 * convention (FUN_800565f0.c's TCameraStatus precedent). */
typedef struct
{
    u8 pad[0x1D];
    u8 oldModeByte1; /* +0x1D: TCameraMode OldMode's high byte */
} TCameraStatus;

typedef struct
{
    s16 div;    /* +0x0 */
    s16 spd;    /* +0x2 */
    SVECTOR bef; /* +0x4 */
} TMakeDifInfo;

extern TCameraStatus CamState;
extern TMakeDifInfo D_80089F10;
extern TMakeDifInfo pnt;
extern void MakeDifSub(VECTOR *src, VECTOR *target, VECTOR *dest, TMakeDifInfo *info);

void MakeDif(GsRVIEW2 *vinfo, GsRVIEW2 *target, GsRVIEW2 *vdif)
{
    if (CamState.oldModeByte1 == 1) {
        vdif->vpx = target->vpx - vinfo->vpx;
        vdif->vpy = target->vpy - vinfo->vpy;
        vdif->vpz = target->vpz - vinfo->vpz;
        vdif->vrx = target->vrx - vinfo->vrx;
        vdif->vry = target->vry - vinfo->vry;
        vdif->vrz = target->vrz - vinfo->vrz;
        CamState.oldModeByte1 = 0;
    } else {
        MakeDifSub((VECTOR *)&vinfo->vrx, (VECTOR *)&target->vrx, (VECTOR *)&vdif->vrx,
                   &D_80089F10);
        MakeDifSub((VECTOR *)vinfo, (VECTOR *)target, (VECTOR *)vdif, &pnt);
    }
}
