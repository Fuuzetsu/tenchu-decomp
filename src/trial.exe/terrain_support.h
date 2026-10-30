#ifndef TENCHU_TRIAL_TERRAIN_SUPPORT_H
#define TENCHU_TRIAL_TERRAIN_SUPPORT_H

#include "mission_editor.h"

/* Five regular terrain classes support a stationary placement. A pond
 * has no support; ramps and the two extreme-height classes return -1.
 * The editor inlines the same rule that other callers invoke out of line. */
#ifndef TRIAL_DEFINE_TERRAIN_SUPPORT
extern __inline__
#endif
short ClassifyTrialTerrainSupport(short kind)
{
    switch (kind)
    {
    case TRIAL_TERRAIN_GROUND:
    case TRIAL_TERRAIN_LEDGE_1M:
    case TRIAL_TERRAIN_LEDGE_2M:
    case TRIAL_TERRAIN_LEDGE_3M:
    case TRIAL_TERRAIN_LEDGE_4M:
        return 1;
    case TRIAL_TERRAIN_POND:
        return 0;
    default:
        return -1;
    }
}

#endif
