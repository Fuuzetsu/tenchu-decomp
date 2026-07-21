#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short PlaySE(struct SoundEffect *se, short pt, long dv);
 *     AUDIO.C:72, 18 src lines, frame 40 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     param $a0       struct SoundEffect * se
 *     param $a1       short pt
 *     param $a2       long dv
 *     reg   $s0       short d
 *     reg   $v1       short v
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
typedef struct
{
    s16 VABid;
    s16 program;
    void *VABhead;
} SoundEffect;

extern u8 gSELevel;
extern s16 voice;
extern u16 SsUtKeyOnV(s16, s16, s32, s32, s32, s32, u32, u32);
extern void SsUtAutoPan(s16, s32, s16, s32);

short PlaySE(SoundEffect *se, short pt, long dv)
{
    s16 d;
    s16 v;
    s16 voll;

    if (se != NULL) {
        d = dv >> 8;
        v = (s16)(dv >> 8);
        voll = (u32)((dv & 0x7f) * gSELevel) >> 7;
        if (v > 0) {
            d = -(s32)((u32)((dv >> 8) & 0x3ff) >> 4);
        } else if (v < 0) {
            v = (v < 0) ? -v : v;
            d = (s32)((u32)(v & 0x3ff) >> 4);
        }
        voice = (voice + 1) % 24;
        if (0 <= (SsUtKeyOnV(voice, se->VABid, pt >> 4, pt & 0xf, 0x24, 0, voll,
                             voll)
                  << 16)) {
            SsUtAutoPan(voice, 0x40, (s16)(0x40 - d), 1);
            return voice;
        }
    }
    return -1;
}

// triage: EASY — 71 insns, mul/div, 2 callees, ~0.04 to handle_char_state_attacking_SEVEN_
// likely-relevant cookbook sections:
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short PlaySE(SoundEffect *se,short pt,long dv)
//
// {
//   ushort uVar1;
//   uint uVar2;
//   uint uVar3;
//   int iVar4;
//   short voll;
//
//   if (se != (SoundEffect *)0x0) {
//     uVar3 = dv >> 8;
//     uVar2 = (uint)(short)((uint)dv >> 8);
//     if ((int)uVar2 < 1) {
//       if ((int)uVar2 < 0) {
//         if ((int)uVar2 < 0) {
//           uVar2 = -uVar2;
//         }
//         uVar3 = (uVar2 & 0x3ff) >> 4;
//       }
//     }
//     else {
//       uVar3 = -((uVar3 & 0x3ff) >> 4);
//     }
//     iVar4 = (voice + 1) % 0x18;
//     voice = (short)iVar4;
//     voll = (short)((dv & 0x7fU) * (uint)(byte)PersistentState._91_1_ >> 7);
//     uVar1 = SsUtKeyOnV((short)((uint)(iVar4 * 0x10000) >> 0x10),se->VABid,pt >> 4,pt & 0xf,0x24,0,
//                        voll,voll);
//     if (-1 < (int)((uint)uVar1 << 0x10)) {
//       SsUtAutoPan(voice,0x40,(short)((0x40 - uVar3) * 0x10000 >> 0x10),1);
//       return voice;
//     }
//   }
//   return -1;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? SsUtAutoPan(s16, ?, s16, ?);                      /* extern */
// s32 SsUtKeyOnV(s16, s16, s32, s32, s32, s32, u32, u32); /* extern */
// extern u8 gSELevel;
// extern s16 voice;
//
// s16 PlaySE(s16 *arg0, s32 arg1, s32 arg2) {
//     s16 temp_t0;
//     s16 var_v0;
//     s32 temp_a2;
//     s32 var_s0;
//     u32 temp_v1;
//
//     if (arg0 != NULL) {
//         temp_a2 = arg2 >> 8;
//         var_s0 = temp_a2;
//         var_v0 = (s16) temp_a2;
//         temp_v1 = (u32) ((arg2 & 0x7F) * gSELevel) >> 7;
//         if (var_v0 > 0) {
//             var_s0 = -(s32) ((u32) (temp_a2 & 0x3FF) >> 4);
//         } else if (var_v0 < 0) {
//             if (var_v0 < 0) {
//                 var_v0 = -var_v0;
//             }
//             var_s0 = (s32) ((u32) (var_v0 & 0x3FF) >> 4);
//         }
//         temp_t0 = (voice + 1) % 24;
//         voice = temp_t0;
//         if (!(SsUtKeyOnV(temp_t0, *arg0, (s32) (arg1 << 0x10) >> 0x14, arg1 & 0xF, 0x24, 0, temp_v1, temp_v1) & 0x8000)) {
//             SsUtAutoPan(voice, 0x40, (s16) (0x40 - var_s0), 1);
//             return voice;
//         }
//         goto block_9;
//     }
// block_9:
//     return -1;
// }
