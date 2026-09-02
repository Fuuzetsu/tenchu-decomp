#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long * LoadAreaMap(unsigned long *adr);
 *     CONFLICT.C:47, 23 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned long * adr
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct NodeIndexType *FieldIndex;
 *     extern struct AreaNodeType *FieldArea;
 * END PSX.SYM */

extern char msg_no_area_data[]; /* NO AREA DATA */

AreaMapType *LoadAreaMap(AreaMapType *adr)
{
    NodeIndexType *map;
    s16 j;
    long idx0;

    map = (NodeIndexType *)adr;
    if (adr == 0)
        SystemOut(msg_no_area_data);

    j = 0;
    idx0 = ((NodeIndexType *)adr)->index;
    if (idx0 != 0)
    {
        do
        {
            map[j].index += (long)adr;
            map[j].y += 2;
            if (map[j].n < 0)
            {
                ((IndexArrayType *)map[j].index)->index =
                    ((IndexArrayType *)map[j].index)->index + (long)adr;
            }
            j++;
        } while (map[j].index != 0);
        idx0 = ((NodeIndexType *)adr)->index;
    }
    GlobalAreaMap = adr;
    FieldIndex = (NodeIndexType *)adr;
    idx0 = ((NodeIndexType *)adr)->index;
    FieldArea = (AreaNodeType *)idx0;
    return adr;
}
