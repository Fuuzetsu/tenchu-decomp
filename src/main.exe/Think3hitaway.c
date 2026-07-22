#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "game_globals.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3hitaway(void);
 *     THINK_3.C:172, 33 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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
 *     reg   $a1       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern long Distance;
 *     extern short SR;
 *     extern short (*AttackFunc[4])();
 *     extern short Degree;
 *     extern short Attrib;
 * END PSX.SYM */

extern Humanoid *Me_THINK_C;
extern s16 Degree;
extern u16 Attrib;

extern s32 rand(void);
extern short ChasetoTarget(long length);
extern short SuccessionAttack(long dist, short deg);
extern s16 turn_towards_player_(s32 x_diff, s32 z_diff);

/*
 * Think3hitaway (0x8002d984) — think-handler, same "think" TU as
 * Think3chase.c/Think3escape.c (s16 return convention; gp-relative
 * Distance/SR/Degree/Attrib — see gpsyms).
 *
 * If close (Distance < 10000) and not already in the "-2" SR state, clear
 * SR. If the character just got hit (character_status == 7): clear actflg,
 * zero out `some_other_x_position`/`some_other_z_position` (Ghidra's
 * `chase[0]`/`chase[1]`), and SuccessionAttack(3000, 1500) for the result.
 * Else if not already acting (actflg == 0): face the player if aim is
 * close (abs(Degree) < 1000, via turn_towards_player_ OR'd with a walk/run
 * hint) or ChasetoTarget(5000) otherwise; roll a 1-in-30 chance (Distance <
 * 2000) to add a "hit" flag (0x40); arm actflg once far enough away
 * (Distance > 4000) or once Attrib bit 0x400 is set. Else (already
 * acting): dispatch through AttackFunc[wpatk>>4]() — same
 * zero-argument indirect-call idiom as Think3chase.c.
 *
 * Matching notes:
 *  - `character_status` is read SIGNED (`lh`) at this one call site even
 *    though game_types.h proves the field itself `u16` (needed elsewhere
 *    to avoid a bad sign-extend) — reached via an offset-cast read,
 *    `*(s16 *)&Me_THINK_C->status`, matching the cookbook's
 *    "reach a divergent-width access via an offset cast off the same
 *    proven pointer" rule, rather than retyping the shared struct field.
 *  - The AttackFunc dispatch (SHORT body, one call + return) must be the
 *    SECOND arm (`else if (actflg != 0)`) and the long "not yet acting"
 *    body LAST/else — Ghidra's own textual order (`else if (actflg==0)
 *    {long body} else {dispatch}`) is the OPPOSITE polarity and cost 220+
 *    bytes of misalignment; this is the general "put the short body
 *    earlier, the long body last so it can fall into the shared epilogue"
 *    lever, not specific to this function.
 *  - The hit-status arm returns SuccessionAttack directly. This leaves its
 *    result in $v0 and lets the jump delay slot start the s16 conversion;
 *    assigning it to the shared result would add an unnecessary $s0 copy
 *    and move the conversion below the epilogue restores.
 */
s16 Think3hitaway(void)
{
    u16 result;
    s32 degree;

    if (Distance < 10000 && SR != -2)
    {
        SR = 0;
    }
    if (*(s16 *)&Me_THINK_C->status == 7)
    {
        Me_THINK_C->actflg = 0;
        Me_THINK_C->chase[1] = 0;
        Me_THINK_C->chase[0] = 0;
        return SuccessionAttack(3000, 0x5dc);
    }
    else if (Me_THINK_C->actflg != 0)
    {
        result = AttackFunc[Me_THINK_C->wpatk >> 4]();
    }
    else
    {
        degree = Degree;
        if (degree < 0)
        {
            degree = -degree;
        }
        if (degree < 1000)
        {
            result = turn_towards_player_(0, 0);
            result = (result & 0xe000) | 0x4000;
        }
        else
        {
            result = ChasetoTarget(5000);
        }
        if (Distance < 2000)
        {
            s32 r;

            r = rand();
            if (r == (r / 30) * 30)
            {
                result |= 0x40;
            }
        }
        if (4000 < Distance || (Attrib & 0x400))
        {
            Me_THINK_C->actflg = 1;
        }
    }
    return result;
}

// triage: MEDIUM — 97 insns, mul/div, indirect-call, 4 callees, ~0.07 to AttackAnimal
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference):
//
//
// short Think3hitaway(void)
//
// {
//   Humanoid *pHVar1;
//   ushort uVar2;
//   int iVar3;
//
//   if ((Distance < 10000) && (SR != -2)) {
//     SR = 0;
//   }
//   if (Me_THINK_C->status == 7) {
//     Me_THINK_C->actflg = '\0';
//     pHVar1 = Me_THINK_C;
//     Me_THINK_C->chase[1] = 0;
//     pHVar1->chase[0] = 0;
//     uVar2 = SuccessionAttack(3000,0x5dc);
//   }
//   else if (Me_THINK_C->actflg == '\0') {
//     iVar3 = (int)Degree;
//     if (iVar3 < 0) {
//       iVar3 = -iVar3;
//     }
//     if (iVar3 < 1000) {
//       uVar2 = FUN_8002b990(0,0);
//       uVar2 = uVar2 & 0xe000 | 0x4000;
//     }
//     else {
//       uVar2 = ChasetoTarget(5000);
//     }
//     if ((Distance < 2000) && (iVar3 = rand(), iVar3 == (iVar3 / 0x1e) * 0x1e)) {
//       uVar2 = uVar2 | 0x40;
//     }
//     if ((4000 < Distance) || ((Attrib & 0x400U) != 0)) {
//       Me_THINK_C->actflg = '\x01';
//     }
//   }
//   else {
//     uVar2 = (*(code *)AttackFunc[(int)((uint)(ushort)Me_THINK_C->wpatk << 0x10) >> 0x14])();
//   }
//   return uVar2;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// s32 ChasetoTarget(?, void *);                       /* extern */
// s32 SuccessionAttack(?, ?);                         /* extern */
// s32 rand();                                         /* extern */
// s32 turn_towards_player_(?, ?);                     /* extern */
// extern ? AttackFunc;
// extern u16 Attrib;
// extern s16 Degree;
// extern s32 Distance;
// extern void *Me_THINK_C;
// extern s16 SR;
//
// s32 Think3hitaway(void) {
//     s16 var_v0_2;
//     s32 temp_ret;
//     s32 var_s0;
//     s32 var_v0;
//
//     if ((Distance < 0x2710) && (SR != -2)) {
//         SR = 0;
//     }
//     if (Me_THINK_C->unk2 == 7) {
//         Me_THINK_C->unk89 = 0U;
//         Me_THINK_C->unk84 = 0;
//         Me_THINK_C->unk80 = 0;
//         var_v0 = SuccessionAttack(0xBB8, 0x5DC) << 0x10;
//     } else {
//         if (Me_THINK_C->unk89 != 0) {
//             var_s0 = *((((s32) (Me_THINK_C->unk8E << 0x10) >> 0x14) * 4) + &AttackFunc)(0xBB8, Me_THINK_C);
//             goto block_18;
//         }
//         var_v0_2 = Degree;
//         if (var_v0_2 < 0) {
//             var_v0_2 = -var_v0_2;
//         }
//         if (var_v0_2 < 0x3E8) {
//             var_s0 = (turn_towards_player_(0, 0) & 0xE000) | 0x4000;
//         } else {
//             var_s0 = ChasetoTarget(0x1388, Me_THINK_C);
//         }
//         if (Distance < 0x7D0) {
//             temp_ret = rand();
//             if (temp_ret == ((temp_ret / 30) * 0x1E)) {
//                 var_s0 |= 0x40;
//             }
//         }
//         if ((Distance >= 0xFA1) || (var_v0 = var_s0 << 0x10, ((Attrib & 0x400) != 0))) {
//             Me_THINK_C->unk89 = 1U;
// block_18:
//             var_v0 = var_s0 << 0x10;
//         }
//     }
//     return var_v0 >> 0x10;
// }
