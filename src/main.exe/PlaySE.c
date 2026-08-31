#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short PlaySE(struct SoundEffect *se, short pt, long dv);
 *     AUDIO.C:72, 18 src lines, frame 40 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct SoundEffect * se
 *     param $a1       short pt
 *     param $a2       long dv
 *     reg   $s0       short d
 *     reg   $v1       short v
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char gSELevel;
 * END PSX.SYM */

/*
 * PlaySE (0x80018b64) — trigger a positional sound effect on a rotating
 * voice slot. `dv` packs the pan direction in its high byte (dv>>8) and a
 * base volume in the low 7 bits; the persisted master-volume byte
 * gSELevel (PersistentState._91_1_, offset 0x5B) scales it. The pan
 * direction is turned into a signed offset `d`: right (v>0) → -mag, left
 * (v<0) → +mag, where mag = (|dir| & 0x3ff) >> 4. `voice` (gp-extern s16)
 * cycles 0..23. SsUtKeyOnV keys the note; if it returns success (bit 15
 * clear, tested as (ret<<16)>=0), SsUtAutoPan applies the pan and the slot
 * index is returned, else -1.
 *
 * STATUS: NON_MATCHING — 69 vs 71 instructions / 8 bytes short. The draft
 * is instruction-for-instruction correct EXCEPT for two register-coalescing
 * copies cc1 emits in the target but elides here:
 *   - `move s0,a2` — the target keeps `dv>>8` in the incoming param reg $a2
 *     (used by the v/mask reads, dies before the call) and copies it to a
 *     separate callee-saved $s0 for `d` (which survives the SsUtKeyOnV call);
 *     our cc1 coalesces both into $s0 and drops the copy.
 *   - `move v0,v1` — the target consolidates `voll` ($v1) into $v0 before its
 *     two stack-arg stores; ours stores $v1 directly.
 * Both are pure register-allocation/coalescing ties below the C level (same
 * value in two regs vs one). autorules found no width win; a bounded
 * decomp-permuter run (4 workers, ~420s, --stop-on-zero) never reached 0 —
 * its AST transforms can't force cc1 to keep the elided copies. The
 * redundant double `bgez` sign test that the target has (an inline
 * `v = (v < 0) ? -v : v;` abs on an already-known-negative value) IS
 * reproduced by the ternary spelling below — a plain `if (v<0) v=-v;` collapses
 * it (cc1 threads the dominated test). New cookbook rule candidate.
 */
extern s16 voice;
extern u16 SsUtKeyOnV(s16, s16, s32, s32, s32, s32, u32, u32);
extern void SsUtAutoPan(s16, s32, s16, s32);

short PlaySE(SoundEffect *se, short pt, long dv)
{
    s16 d;
    s16 v;
    s16 voll;

    if (se != NULL)
    {
        d = dv >> 8;
        v = (s16)(dv >> 8);
        voll = (u32)((dv & 0x7f) * gSELevel) >> 7;
        if (v > 0)
        {
            d = -(s32)((u32)((dv >> 8) & 0x3ff) >> 4);
        }
        else if (v < 0)
        {
            v = (v < 0) ? -v : v;
            d = (v & 0x3ff) >> 4;
        }
        voice = (voice + 1) % 24;
        if ((s16)SsUtKeyOnV(voice, se->VABid, pt >> 4, pt & 0xf, 0x24, 0, voll,
                            voll) >= 0)
        {
            SsUtAutoPan(voice, 0x40, (s16)(0x40 - d), 1);
            return voice;
        }
    }
    return -1;
}
