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
