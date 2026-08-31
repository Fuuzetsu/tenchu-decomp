#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupThinkFunction(struct Humanoid *human, short type);
 *     THINK.C:212, 12 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *     param $a1       short type
 *
 * Globals it touches, as the original declared them:
 *     extern short (*Think1Func[10])();
 *     extern short (*Think2Func[5])();
 *     extern short (*Think3Func[10])();
 *     extern short (*Think4Func[6])();
 * END PSX.SYM */

/*
 * SetupThinkFunction (0x8002f73c, 0xb8 bytes) — installs Humanoid's four
 * PSX.SYM-recovered think callbacks from lookup tables keyed off nibbles of
 * `type`, then sets or clears attribute bit 4 depending on whether `type` is
 * one of three "default" sentinels (0, 0x1111, 0x2222).
 *
 * Matching constraints:
 *  - All four callbacks can use ordinary array indexing. For Think2Func and
 *    Think3Func, select the nibble before cc1's implicit pointer scaling:
 *    the staged `packed` value's `>> 20` / `>> 24` nibble reads (naming the
 *    stage is required: reading the nibbles straight off `type` costs 43
 *    lines, and the stage must be assigned AFTER the think[0] store).
 *    These compile identically to the older masked byte-offset casts.
 *  - Keep `table2` and `table3` as pointer locals assigned before their
 *    lookups. They make cc1 materialize each base ahead of the preceding
 *    table's load/store; indexing either extern directly changes scheduling.
 *  - Each shifted selector spells its own full-width `type << 16` compound.
 *    Sharing a narrowed `type` value merges the extensions and reorders the
 *    prologue.
 *  - The final sentinel check independently re-derives signed `type` through
 *    its shift pair rather than reusing another selector.
 */
void SetupThinkFunction(Humanoid *human, TThinkType type)
{
    s32 check;
    s32 packed;
    ThinkFunc *table2;
    ThinkFunc *table3;

    human->think[0] = Think1Func[type & 0xF];
    table2 = Think2Func;
    /* One shared narrowing of the packed nibble field; each level then
     * reads its nibble out of the staged value (bits 4-7, 8-11, 12-15). */
    packed = (s32)type << 16;
    human->think[1] = table2[(packed >> 20) & 0xF];
    table3 = Think3Func;
    human->think[2] = table3[(packed >> 24) & 0xF];
    human->think[3] = Think4Func[(u32)packed >> 28];
    check = packed >> 16;
    if (check == THINK_MIX_NONE || check == THINK_MIX_PLAYER ||
        check == THINK_MIX_PAD2)
    {
        human->attribute &= ~4;
    }
    else
    {
        human->attribute |= 4;
    }
}
