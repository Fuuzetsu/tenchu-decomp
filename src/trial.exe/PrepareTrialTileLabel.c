#include "common.h"
#include "mission_editor.h"
#include "../main.exe/font.h"

enum
{
    TILE_LABEL_THICK = 10,
    TILE_LABEL_THIN,
    TILE_LABEL_TRIGGER,
    TILE_LABEL_SHOOTER,
    TILE_LABEL_DISGUISE,
    TILE_LABEL_LIGHT,
    TILE_LABEL_DECORATION
};

static char *TrialTileLabels[] = {
    "\222\156\226\312", /* 地面 */
    "\202\120\202\215\202\314\222\151\215\267", /* １ｍの段差 */
    "\202\121\202\215\202\314\222\151\215\267", /* ２ｍの段差 */
    "\202\122\202\215\202\314\222\151\215\267", /* ３ｍの段差 */
    "\202\123\202\215\202\314\222\151\215\267", /* ４ｍの段差 */
    "\202\120\202\215\202\314\215\342", /* １ｍの坂 */
    "\202\121\202\215\202\314\215\342", /* ２ｍの坂 */
    "\225\307", /* 壁 */
    "\214\212", /* 穴 */
    "\222\162", /* 池 */
    "\221\276\202\242\225\250", /* 太い物 */
    "\215\327\202\242\225\250", /* 細い物 */
    "\201\136\215\354\223\256", /* ／作動 */
    "\201\136\224\255\216\313", /* ／発射 */
    "\201\136\213\125\221\225", /* ／偽装 */
    "\201\136\217\306\226\276", /* ／照明 */
    "\221\225\217\374", /* 装飾 */
};

/* Build the editor caption from the terrain kind and optional object,
 * trigger, shooter/disguise, and light flags. Labels are Shift-JIS. */
char *PrepareTrialTileLabel(short tile)
{
    u16 flags = TrialTileDefinitions[tile].flags;
    int kind;

    strcpy(TrialTileLabelText,
        TrialTileLabels[kind = ((flags & TRIAL_TILE_KIND_MASK) >> 4) - 1]);
    if (flags & TRIAL_TILE_ORNAMENT_MASK)
    {
        if (kind == 0)
            strcpy(TrialTileLabelText, TrialTileLabels[TILE_LABEL_DECORATION]);
        else
        {
            if (kind < 5)
                TrialTileLabelText[6] = 0;
            else
                TrialTileLabelText[0] = 0;
            strcat(TrialTileLabelText,
                TrialTileLabels[(flags & TRIAL_TILE_ORNAMENT_THICK) ? TILE_LABEL_THICK : TILE_LABEL_THIN]);
        }
    }
    if (flags & TRIAL_TILE_TRIGGER)
        strcat(TrialTileLabelText, TrialTileLabels[TILE_LABEL_TRIGGER]);
    else if (flags & TRIAL_TILE_SHOOTER)
        strcat(TrialTileLabelText,
            TrialTileLabels[(flags & TRIAL_TILE_ORNAMENT_MASK) ? TILE_LABEL_DISGUISE : TILE_LABEL_SHOOTER]);
    if (flags & TRIAL_TILE_LIGHT)
        strcat(TrialTileLabelText, TrialTileLabels[TILE_LABEL_LIGHT]);
    if (!TrialTileLabelText[2])
        strcat(TrialTileLabelText, "\201\100"); /* full-width padding space */
    SetupTelop((u8 *)TrialTileLabelText, 0);
    return TrialTileLabelText;
}
