#include "common.h"
#include "mission_editor.h"
#include "../main.exe/font.h"

static char *TrialPatrolDigits[] = {
    "\202\117", "\202\120", "\202\121", "\202\122", "\202\123",
    "\202\124", "\202\125", "\202\126", "\202\127", "\202\130"
};

/* Show either the configured route length or its unused capacity. The
 * player and medicinal herb are fixed placements without patrol routes. */
char *PrepareTrialPatrolLabel(TrialPlacement *placement, short remaining)
{
    short count, digit;

    TrialPatrolCountText[0] = 0;
    if (remaining)
        count = TRIAL_PATROL_CAPACITY - 1 - placement->route_last;
    else
        count = placement->route_last + 1;
    digit = count / 10;
    if (digit != 0)
        strcat(TrialPatrolCountText, TrialPatrolDigits[digit]);
    strcat(TrialPatrolCountText, TrialPatrolDigits[(short)(count % 10)]);

    if (placement->kind == 0 || placement->kind == TRIAL_HERB_OBJECT)
        __builtin_memcpy(TrialPatrolLabelText, "\214\305\222\350", 5); /* 固定 */
    else if (remaining)
        sprintf(TrialPatrolLabelText, "\216\143\202\350%s\203\124\203\103\203\116\203\213", TrialPatrolCountText); /* 残り%sサイクル */
    else
    {
        if (placement->route_last == 0)
            __builtin_memcpy(TrialPatrolLabelText, "\226\242\220\335\222\350", 7); /* 未設定 */
        else
            sprintf(TrialPatrolLabelText, "%s\203\124\203\103\203\116\203\213", TrialPatrolCountText); /* %sサイクル */
    }
    SetupTelop((u8 *)TrialPatrolLabelText, 1);
    return TrialPatrolLabelText;
}
