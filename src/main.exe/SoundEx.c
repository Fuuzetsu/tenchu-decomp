#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SoundEx(struct VECTOR *locate, short seid);
 *     SEMNG.C:42, 15 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $a0       struct VECTOR * locate
 *     param $a1       short seid
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *StagePlayer;
 *     extern struct SoundEffect *StageSE;
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

/*
 * SoundEx (0x8004fc70) — play a positional sound effect: if `locate` is null
 * or IS the player's own position, play at a fixed volume (0x3f for seid
 * 0x12, else 0x7f), no direction encoded. Otherwise compute planar+vertical
 * distance from the player; give up (return -1) past 18000/10000 units. If
 * both distances are under 2000, full volume with no direction; otherwise a
 * distance-attenuated volume, ORed with the (clamped, +-0x1000 wrapped)
 * angle from the player to `locate` (relative to the camera's own direction
 * offset in CMODE_DIRECTION) shifted up 8 bits.
 *
 * Matching notes (all verified against the original bytes):
 *  - The two branches of `if (locate == 0 || locate == pp)` each end their
 *    OWN `return PlaySE(...)` — this is NOT a single shared tail after an
 *    if/else. The asm shows the short branch materializing
 *    StageSE/seid and jumping directly to the `jal PlaySE`
 *    instruction, skipping the long branch's own OR-with-angle setup
 *    entirely; jump.c's cross-jumping merged only the byte-identical
 *    `jal PlaySE` + return tail, not the differing argument setup. Ghidra's
 *    `uVar4 = iVar5 << 8 | uVar4;` placed AFTER the if/else is a decompiler
 *    normalization artifact — the short branch never executes that OR.
 *  - **`dist` must carry the final `volume` value** (`dist = 0x7f - ...; dist =
 *    (dist * ...);` then `vol = (angle<<8)|dist`). The SquareRoot0 result and
 *    the volume coalesce into ONE callee-saved register ($s1) in the target,
 *    because `volume` is computed in the ratan2 call's delay slot and survives
 *    the call (forced callee-saved), and `dist` dies exactly where `volume` is
 *    born. Spelling `volume` as its own separate local instead keeps `dist`
 *    caller-saved (a distinct register) and mismatches the entire function via
 *    an extra callee-saved register (frame 0x28 vs the target's 0x30) — 51+
 *    diff lines. This one rewrite collapsed the residual from 61 lines to 5.
 *  - `vol`'s division-by-constant sequences (`mult` + `sra` sign-correction,
 *    not `multu`/`srl`) confirm the intermediate arithmetic is signed —
 *    `dist`/`dy` must stay plain `s32`, not unsigned, for the magic-multiply
 *    constants to match.
 *  - **The vertical-distance abs is the GE-spelled ternary ASSIGNED to `dy`**:
 *    `dy = (raw >= 0) ? raw : -raw;`. Only this arm order folds to ABS_EXPR
 *    (one abssi2 insn writing `dy`: `bgez raw,1f; move dy,raw; negu dy,dy` —
 *    negu of the COPY), and it works as a plain assignment, any context. The
 *    LT spelling `(raw < 0) ? -raw : raw` does NOT fold: fold-const.c's
 *    COND_EXPR case first INVERTS it (second-operand-equivalent clause) but
 *    then re-reads `arg1 = TREE_OPERAND (t, 2)` from the ALREADY-SWAPPED tree
 *    — arg1 becomes the negation arm, so the abs check's
 *    operand_equal_for_comparison_p fails and the branchy singleton path
 *    (negu of the SOURCE, `negu a0,v1` — the old 7-byte residual) is emitted
 *    instead. `raw` dies at the abssi2, freeing the $v0-vs-$v1 tie the parked
 *    draft could not reach. This refines UpdateMotion's "reach abssi2 inline"
 *    rule: the abs RESULT can be a reusable variable — spell the ternary GE.
 *  - **`maxvol = 0x7f;` hoisted above the near/far branch is a PRIORITY
 *    ballast, found by the permuter and verified against global.c**: with the
 *    abs now one insn shorter (x2), `dist`'s live length dropped 50→48 and its
 *    allocno priority (floor_log2(refs)*refs/live_length, allocno_compare)
 *    rose past `pp`'s (27/48*10000 = 5625 > 10/18*10000 = 5555), swapping
 *    their $s0/$s1 colours (14 bytes). The extra li insn inside dist's live
 *    range restores 27/49 = 5510 < 5555 → pp back to $s0, dist $s1. The insn
 *    costs no bytes: it lands exactly on the target's `addiu v0,zero,0x7f`
 *    feeding `subu s1,v0,v1` (the else-arm's `0x7f - q`), and the then-arm
 *    keeps its own LITERAL `dist = 0x7f;` (cse leaves the immediate li in the
 *    jump delay slot rather than copying maxvol's register).
 *  - The ratan2 result reuses `raw` (dead since the abs): its pseudo is
 *    $v0-homed everywhere, so the call-result move deletes as a no-op and
 *    `subu a2,v0,v1` reads $v0 directly.
 *  - The shared retail TCameraStatus supplies Mode at +0x14 and signed
 *    DirectionRY at +0x1A.
 */


extern SoundEffect *StageSE;

extern short PlaySE(SoundEffect *se, short pt, long dv);

short SoundEx(VECTOR *locate, short seid)
{
    VECTOR *pp;
    s32 dist, dx, dz;
    s32 maxvol;
    s32 dy;
    s32 angle;
    s32 vol;
    s32 raw;

    pp = StagePlayer->locate;
    if (locate == 0 || locate == pp) {
        return PlaySE(StageSE, seid, (seid == 0x12) ? 0x3f : 0x7f);
    }

    dx = locate->vx - pp->vx;
    dz = locate->vz - pp->vz;
    dist = SquareRoot0(dx * dx + dz * dz);
    if (17999 < dist) {
        return -1;
    }
    raw = locate->vy - pp->vy;
    dy = (raw >= 0) ? raw : -raw;
    if (9999 < dy) {
        return -1;
    }
    maxvol = 0x7f;
    if (dist < 2000 && dy < 2000) {
        angle = 0;
        dist = 0x7f;
    } else {
        dist = maxvol - (dist << 7) / 18000;
        dist = (dist * (10000 - dy)) / 10000;
        raw = ratan2(-dx, -dz);
        angle = raw - StagePlayer->rotate->vy;
        if (CamState.Mode == CMODE_DIRECTION) {
            angle -= CamState.DirectionRY;
        }
        if (0x800 < angle) {
            angle = 0x1000 - angle;
        } else {
            if (angle < -0x7ff) {
                angle += 0x1000;
            }
        }
    }
    vol = (angle << 8) | dist;
    return PlaySE(StageSE, seid, vol);
}
