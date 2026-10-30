#include "common.h"
#include "mission_menu.h"
#include "card.h"
#include "telop.h"
#include "../main.exe/font.h"

/* Reveal five list entries over successive frames. The selected mission's
 * author and three records share the side panel. */
void DrawTrialMissionList(TrialMissionPreview *previews, short selected,
                          u32 reveal_frames, int priority)
{
    TrialTelopStyle styles[5];
    char message[64];
    int x;
    int i;

    memset(styles, 0, sizeof(styles));
    if (!previews)
    {
        switch (TrialCardStatus)
        {
        case CARD_RESULT_NO_CARD:
            /* メモリーカードが差されていません */
            sprintf(message, "\203\201\203\202\203\212\201[\203J\201[\203h\202\252\215\267\202\263\202\352\202\304\202\242\202\334\202\271\202\361");
            x = -228;
            break;
        case CARD_RESULT_UNFORMATTED:
            /* フォーマットされていません */
            sprintf(message, "\203t\203H\201[\203}\203b\203g\202\263\202\352\202\304\202\242\202\334\202\271\202\361");
            x = -204;
            break;
        default:
            /* データがありません */
            sprintf(message, "\203f\201[\203^\202\252\202\240\202\350\202\334\202\271\202\361");
            x = -172;
            break;
        }
        SetupTrialColoredTelop(message, 0, TRIAL_TELOP_WHITE);
        draw_telop_line_(OTablePt->org, x, -5, message);
    }
    else
    {
        styles[selected] = TRIAL_TELOP_YELLOW;
        if (reveal_frames > 0 && previews[0].available)
        {
            SetupTrialColoredTelop(previews[0].title, 0, styles[0]);
            draw_telop_line_(OTablePt->org, -206, -55, previews[0].title);
            DrawTrialMissionOrigin(&previews[0], -246, -58, priority);
            SetupTrialColoredTelop(previews[selected].author, 5, TRIAL_TELOP_DIM);
            draw_telop_line_(OTablePt->org, 104, -2, previews[selected].author);
            for (i = 0; i < TRIAL_PREVIEW_RECORDS; i++)
            {
                SetupTrialColoredTelop(previews[selected].names[i], i + 6, TRIAL_TELOP_DIM);
                draw_telop_line_(OTablePt->org, 74, 32 + i * 20, previews[selected].names[i]);
                SetupTrialColoredTelop(previews[selected].times[i], i + 9, TRIAL_TELOP_DIM);
                draw_telop_line_(OTablePt->org, 154, 32 + i * 20, previews[selected].times[i]);
            }
        }
        if (reveal_frames > 2 && previews[1].available)
        {
            SetupTrialColoredTelop(previews[1].title, 1, styles[1]);
            draw_telop_line_(OTablePt->org, -206, -30, previews[1].title);
            DrawTrialMissionOrigin(&previews[1], -246, -33, priority);
        }
        if (reveal_frames > 4 && previews[2].available)
        {
            SetupTrialColoredTelop(previews[2].title, 2, styles[2]);
            draw_telop_line_(OTablePt->org, -206, -5, previews[2].title);
            DrawTrialMissionOrigin(&previews[2], -246, -8, priority);
        }
        if (reveal_frames > 6 && previews[3].available)
        {
            SetupTrialColoredTelop(previews[3].title, 3, styles[3]);
            draw_telop_line_(OTablePt->org, -206, 20, previews[3].title);
            DrawTrialMissionOrigin(&previews[3], -246, 17, priority);
        }
        if (reveal_frames > 8 && previews[4].available)
        {
            SetupTrialColoredTelop(previews[4].title, 4, styles[4]);
            draw_telop_line_(OTablePt->org, -206, 45, previews[4].title);
            DrawTrialMissionOrigin(&previews[4], -246, 42, priority);
        }
    }
}
