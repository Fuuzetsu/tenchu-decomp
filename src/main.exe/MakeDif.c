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
 * GsRVIEW2 camera view, gated by CamState.CriticalHit's "just snapped"
 * one-shot flag: a straight
 * 6-field s32 subtraction the first frame after a mode change (and clears
 * the flag), otherwise a smoothed delta via two MakeDifSub calls — one over
 * the rotation-only half (vrx..vrz) using a TMakeDifInfo scratch block that
 * sits right after the retail CamState in memory (`ref` = CamState +
 * 0x20). Ghidra's demo-shaped type mis-renders this as
 * `&CamState.Valiation`, but it is really a separate
 * static, its address just materializes as its own `lui`/`addiu`, never
 * derived from CamState's already-loaded base register), one over the full
 * 6-field view using the already-named `pnt` global.
 *
 * Retail rearranged the demo's TCameraStatus: raw halfword accesses prove
 * DirectionRX/DirectionRY at +0x18/+0x1A, while this function's byte access
 * and SetCameraMode's stores prove CriticalHit at +0x1D. The demo's
 * Valiation at +0x20 disappeared when the record shrank to 0x20 bytes.
 */
extern TMakeDifInfo ref;
extern TMakeDifInfo pnt;
extern void MakeDifSub(VECTOR *src, VECTOR *target, VECTOR *dest, TMakeDifInfo *info);

void MakeDif(GsRVIEW2 *vinfo, GsRVIEW2 *target, GsRVIEW2 *vdif)
{
    if (CamState.CriticalHit == 1) {
        vdif->vpx = target->vpx - vinfo->vpx;
        vdif->vpy = target->vpy - vinfo->vpy;
        vdif->vpz = target->vpz - vinfo->vpz;
        vdif->vrx = target->vrx - vinfo->vrx;
        vdif->vry = target->vry - vinfo->vry;
        vdif->vrz = target->vrz - vinfo->vrz;
        CamState.CriticalHit = 0;
    } else {
        MakeDifSub((VECTOR *)&vinfo->vrx, (VECTOR *)&target->vrx, (VECTOR *)&vdif->vrx,
                   &ref);
        MakeDifSub((VECTOR *)vinfo, (VECTOR *)target, (VECTOR *)vdif, &pnt);
    }
}
