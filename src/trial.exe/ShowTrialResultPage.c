#include "common.h"
#include "results.h"
#include "display.h"
#include "../main.exe/graphics.h"
#include <psxsdk/libgpu.h>

extern void GsInitGraph2(u_short width, u_short height, u_short interlace,
                         u_short dither, u_short vram_mode);

TrialResultPage TrialResultPages[4] = {
    UpdateTrialSuccessPage, UpdateTrialFailurePage,
    UpdateTrialNewRecordPage, UpdateTrialRecordSave
};

short ShowTrialResultPage(short page, short width)
{
    short phase = 1;
    short brightness = 128;
    short result;

    ClearTrialDisplay(width, 480);
    if (page != TRIAL_RESULTS_NEW_RECORD)
        GsInitGraph2(width, 240, 4, 1, 0);
    GsInit3D();
    ClearTrialPages();
    TrialResultPages[page](TRIAL_RESULT_PAGE_INIT);
    SetDispMask(1);
    DrawSync(0);
    VSync(0);
    do
    {
        StartDrawing();
        switch (phase)
        {
        case 1:
            brightness -= 12;
            if (brightness <= 0)
            {
                brightness = 0;
                phase++;
                TrialResultsButtons = GetRealPad(0);
            }
            DrawTrialScreenFade(brightness, width, 240, 0);
            break;
        case 2:
            switch (TrialResultPages[page](TRIAL_RESULT_PAGE_INPUT))
            {
            case 1:
                result = 1;
                phase++;
                break;
            case -1:
                result = 0;
                phase++;
                break;
            }
            break;
        case 3:
            brightness += 12;
            if (brightness >= 128)
            {
                brightness = 128;
                phase++;
            }
            DrawTrialScreenFade(brightness, width, 240, 0);
            break;
        default:
            phase = 0;
            break;
        }
        TrialResultPages[page](TRIAL_RESULT_PAGE_DRAW);
        SkipFrame = SKIPFRAME_AFTER_LOAD;
        EndDrawing(-2);
    } while (phase);
    TrialResultPages[page](TRIAL_RESULT_PAGE_DISPOSE);
    ClearTrialDisplay(width, 480);
    ClearTrialPages();
    SetDispMask(0);
    return result;
}
