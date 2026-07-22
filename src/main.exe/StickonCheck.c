#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct MapVector * StickonCheck(void);
 *     MOTION.C:346, 17 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     reg   $v1       short rv
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct VECTOR *dtL;
 *     extern short RefrectVector[16];
 *     extern short motID;
 *     extern short motMODE;
 * END PSX.SYM */

/*
 * StickonCheck (0x8001d374) tests whether the current character can attach to
 * the surface in front of it. Character and surface attributes can reject the
 * probe; reflect-vector flags reject forbidden edges/slopes. A successful
 * probe selects motion 0xc00 when necessary and returns the shared map result.
 *
 * The map-attribute test is deliberately a positive enclosing `if`, with the
 * null return after the block. This is ordinary human control flow and agrees
 * with the demo decompilation and its source-line sequence. The equivalent
 * early-return spelling gives GCC's reorg pass an owned success label whose
 * leading RefrectVector `lui` can be stolen into the branch delay slot. The
 * positive block instead emits retail's direct failure branch with zero in its
 * delay slot, and the remaining code then matches byte-for-byte.
 *
 * PSX.SYM's signed `short rv` and `short RefrectVector[16]` are retained. GCC
 * still chooses the retail `lhu`, keeps the masked use unsigned, and inserts
 * the later signed-short extension for the -1 sentinel test.
 */
extern Humanoid *Me_MOTION_C;
extern s16 motID;
extern s16 motMODE;
extern MapVector map;
extern long GetAreaMapVector(unsigned long *area, MapVector *mvp,
                             VECTOR *pos, long wide, int mode);

MapVector *StickonCheck(void)
{
    short rv;

    if ((u16)Me_MOTION_C->type >= 2)
    {
        return 0;
    }
    if ((Me_MOTION_C->map.attrib & 0xC000) != 0)
    {
        return 0;
    }
    GetAreaMapVector(GlobalAreaMap, &map, dtL,
                     Me_MOTION_C->width + 0x64, 5);
    if ((map.attrib & 0xC000) == 0)
    {
        rv = RefrectVector[map.vector];
        if (Me_MOTION_C->status != 0xc && (rv & 0x200) != 0)
        {
            return 0;
        }
        if (rv == -1)
        {
            return 0;
        }
        if (Me_MOTION_C->status != 0xc)
        {
            motID = 0xc00;
            motMODE = 1;
        }
        return &map;
    }
    return 0;
}
