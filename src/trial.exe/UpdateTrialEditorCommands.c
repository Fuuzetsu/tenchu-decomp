#include "common.h"
#include "mission_editor.h"
#include "../main.exe/images.h"
#include "../main.exe/sound.h"

/* The command menu skips the current editing screen, except Playtest, which
 * remains repeatable. Cross and Resume return to that screen without a switch. */
int UpdateTrialEditorCommands(short interactive)
{
    short row, command;
    GsSPRITE *label;

    if (!interactive)
    {
        switch (TrialEditorMode)
        {
        case TRIAL_EDITOR_TERRAIN:
            TrialEditorCommandSelection = TRIAL_EDITOR_CHARACTERS;
            break;
        case TRIAL_EDITOR_CHARACTERS:
            TrialEditorCommandSelection = TRIAL_EDITOR_PATROLS;
            break;
        default:
            TrialEditorCommandSelection = TRIAL_EDITOR_TERRAIN;
            break;
        }
        return 0;
    }
    row = 0;
    for (command = 0; command < TRIAL_EDITOR_COMMAND_COUNT; command++)
    {
        if (command != TrialEditorMode || command == TRIAL_EDITOR_PLAYTEST)
        {
            label = &TrialEditorCommandLabels[command];
            TrialEditorCommandBackground.y = row * 20 - 90;
            label->y = TrialEditorCommandBackground.y;
            GsSortSprite(&TrialEditorCommandBackground, OTablePt, 2);
            if (command == TRIAL_EDITOR_RESUME)
            {
                TrialEditorResumeHint.y = label->y;
                GsSortSprite(&TrialEditorResumeHint, OTablePt, 1);
            }
            if (command == TrialEditorCommandSelection)
            {
                label->r = label->g = label->b = 128;
                label->x--;
                label->y--;
                label->attribute = (label->attribute & ~GS_ATTR_SEMITRANS_MASK) |
                    GS_ATTR_SEMITRANS_ADD;
                GsSortSprite(label, OTablePt, 0);
                label->x++;
                label->y++;
            }
            else
                label->r = label->g = label->b = 48;
            label->attribute = (label->attribute & ~GS_ATTR_SEMITRANS_MASK) |
                GS_ATTR_SEMITRANS_SUBTRACT;
            GsSortSprite(label, OTablePt, 1);
            row++;
        }
    }
    TrialEditorPreviousCommandButtons = TrialEditorCommandButtons;
    TrialEditorCommandButtons = GetRealPad(0);
    if (TrialEditorPreviousCommandButtons)
        return 0;
    if (TrialEditorCommandButtons)
    {
        switch (TrialEditorCommandButtons)
        {
        case PADLup:
        case PADLdown:
            command = TrialEditorCommandSelection;
            do
            {
                int selection = TrialEditorCommandSelection;
                TrialEditorCommandSelection = TrialEditorCommandButtons == PADLdown
                    ? (selection + 1) % TRIAL_EDITOR_COMMAND_COUNT
                    : (selection + 7) % TRIAL_EDITOR_COMMAND_COUNT;
            } while (TrialEditorCommandSelection == TrialEditorMode &&
                     TrialEditorCommandSelection != TRIAL_EDITOR_PLAYTEST);
            if (command != TrialEditorCommandSelection)
                SoundEx(NULL, TRIAL_SOUND_CURSOR);
            break;
        case PADRright:
        case PADRdown:
            SoundEx(NULL, TRIAL_SOUND_ACCEPT);
            while (GetRealPad(0))
                ;
            if (TrialEditorCommandButtons == PADRdown ||
                TrialEditorCommandSelection == TRIAL_EDITOR_RESUME)
            {
                TrialEditorCommandSelection = TRIAL_EDITOR_RESUME;
                return 1;
            }
            TrialEditorMode = TrialEditorCommandSelection;
            return -1;
        }
    }
    return 0;
}
