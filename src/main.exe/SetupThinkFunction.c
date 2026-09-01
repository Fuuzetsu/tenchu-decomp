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
 * `type`, then sets or clears ATTR_CUSTOMAI depending on whether `type` is one
 * of three "default" sentinels (0, 0x1111, 0x2222).
 *
 * Matching constraints:
 *  - The four selectors are ordinary `(type >> 4/8/12) & 0xF` reads and the
 *    function needs NO locals at all — PSX.SYM records only the two
 *    parameters. The target's single `sll 16` is not a source construct:
 *    it is cc1's CSE of the signed-short promotion, which combine folds
 *    together with the four-byte pointer scale into `sra 18`, `sra 22` and
 *    `srl 28`, reusing `sra 16` for the sentinel compares.
 *  - Restore the plain expression graph AT ONCE. Changing the nibble reads
 *    while keeping staged or table-alias locals scores 43 diff lines, which
 *    is what made an earlier draft believe the staging was required.
 *  - Refuted 2026-08-31: bitfield unions and pointer overlays (12-48 lines;
 *    an overlay forces an 8-byte frame and an `sh` spill the target does not
 *    have), an `int` parameter (12), `unsigned short` (9), mask-then-shift
 *    (20), destructive `type >>= 4` between stores (41); dropping the
 *    high-nibble mask on think[3] costs 2. The demo (0x800276b4) and trial
 *    (0x80032ebc) builds carry the same shift motif, so it predates retail.
 */
void SetupThinkFunction(Humanoid *human, TThinkType type)
{
    human->think[0] = Think1Func[THINK1_FROM_MIX(type)];
    human->think[1] = Think2Func[THINK2_FROM_MIX(type)];
    human->think[2] = Think3Func[THINK3_FROM_MIX(type)];
    human->think[3] = Think4Func[THINK4_FROM_MIX(type)];
    if (type == THINK_MIX_NONE || type == THINK_MIX_PLAYER ||
        type == THINK_MIX_PAD2)
    {
        human->attribute &= ~ATTR_CUSTOMAI;
    }
    else
    {
        human->attribute |= ATTR_CUSTOMAI;
    }
}
