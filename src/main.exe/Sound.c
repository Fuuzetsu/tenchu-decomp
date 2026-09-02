#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Sound(struct Humanoid *human, short seid);
 *     SEMNG.C:61, 8 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *     param $a1       short seid
 *     reg   $a0       struct VECTOR * locate
 *     reg   $a1       short seid
 *     reg   $s0       long volume
 *     reg   $s1       long zz
 *     reg   $s2       long xx
 *     reg   $a1       struct VECTOR * player
 *     reg   $a0       struct VECTOR * locate
 *     reg   $a1       short seid
 *     reg   $s0       long volume
 *     reg   $s1       long zz
 *     reg   $s2       long xx
 *     reg   $a1       struct VECTOR * player
 *
 * Globals it touches, as the original declared them:
 *     extern short VoiceMode;
 * END PSX.SYM */

short Sound(Humanoid *human, short seid)
{
    if (SOUND_ID_HAS_PROGRAM(seid))
    {
        return SoundEx(human->locate, seid);
    }
    if (seid > CHAR_SE_SPECIAL)
    {
        if (VoiceMode != 0)
        {
            return -1;
        }
        if ((human->attribute & ATTR_SUSPEND) != 0)
        {
            return -1;
        }
    }
    return SoundEx(human->locate,
                   (short)SOUND_ID_WITH_PROGRAM(seid, human->sound));
}
