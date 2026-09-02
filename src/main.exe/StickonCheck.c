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

extern Humanoid *Me_MOTION_C;
extern MapVector map;

MapVector *StickonCheck(void)
{
    facing_angle rv;

    if ((u16)Me_MOTION_C->type >= N_PLAYABLE_CHARACTERS)
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
        /* The half-quadrant bit is set exactly on diagonal wall angles: a
         * stick already in progress may continue around a corner, but a new
         * one cannot start there. */
        if (Me_MOTION_C->status != STAT_STICKON &&
            (rv & ANGLE_HALF_QUADRANT) != 0)
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
