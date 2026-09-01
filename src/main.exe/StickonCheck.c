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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * the later signed-short extension for the ANGLE_NONE sentinel test.
 */
extern Humanoid *Me_MOTION_C;
extern MapVector map;

MapVector *StickonCheck(void)
{
    facing_angle rv;

    if ((u16)Me_MOTION_C->type >= 2)
    {
        return 0;
    }
    if ((Me_MOTION_C->map.attrib & (MAP_SLOPE_X | MAP_SLOPE_Z)) != 0)
    {
        return 0;
    }
    GetAreaMapVector(GlobalAreaMap, &map, dtL, Me_MOTION_C->width + 100,
                     AREA_LEVEL_STEP_DOWN | AREA_LEVEL_ALLOW_DEEP);
    if ((map.attrib & (MAP_SLOPE_X | MAP_SLOPE_Z)) == 0)
    {
        rv = RefrectVector[map.vector];
        /* Bit 0x200 is set exactly on the DIAGONAL wall angles (odd
         * multiples of 512): a stick already in progress may continue
         * around a corner, but a new one cannot start there. */
        if (Me_MOTION_C->status != STAT_STICKON && (rv & 0x200) != 0)
        {
            return 0;
        }
        if (rv == ANGLE_NONE)
        {
            return 0;
        }
        if (Me_MOTION_C->status != STAT_STICKON)
        {
            SET_MOTION(MOT_STICKON, MOTION_MOVE_APPLY);
        }
        return &map;
    }
    return 0;
}
