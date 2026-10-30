#include "common.h"
#include "mission_menu.h"
#include "card.h"
#include "telop.h"
#include "../main.exe/images.h"
#include "../main.exe/font.h"
#include "../main.exe/sound.h"
#include "../main.exe/padcmd.h"
#include <psxsdk/libgpu.h>

enum trial_list_phase
{
    LIST_FADE_IN = 1,
    LIST_RESET,
    LIST_CHECK_CARD,
    LIST_SHOW_DETAILS,
    LIST_BROWSE,
    LIST_HIDE_DETAILS,
    LIST_CHOOSE_ACTION,
    LIST_LOAD_MISSION,
    LIST_FADE_OUT,
    LIST_START_DIRECTORY = 20,
    LIST_READ_DIRECTORY,
    LIST_BUILD_PREVIEWS,
    LIST_DIRECTORY_FAILED,
    LIST_CARD_CHANGED = 25,
    LIST_START_DELETE = 30,
    LIST_DELETE,
    LIST_UNLOCK_REFRESH = 35
};

enum
{
    TRIAL_LIST_CAPACITY = TRIAL_PRESET_COUNT + TRIAL_CARD_DIRECTORY_CAPACITY,
    TRIAL_LIST_VISIBLE_ROWS = 5,
    TRIAL_CHEAT_UNLOCK_PRESETS = 0x4000,
    TRIAL_PRESET_COMPLETION_MASK = (1 << N_CAMPAIGN_MISSIONS) - 1
};

/* Browse the CD presets and card headers in one five-row window. Editing
 * offers only saved missions, including their delete confirmation screen. */
short SelectTrialMission(short saved_only)
{
    TrialMissionHeader headers[TRIAL_LIST_CAPACITY];
    TrialMissionPreview previews[TRIAL_LIST_CAPACITY];
    u_long held;
    u32 reveal_frames = 0;
    u32 idle_ticks = 0;
    short accepted = 0;
    short title_x;
    int selected = 0;
    short phase = LIST_FADE_IN;
    short brightness = 128;
    short row = 0;
    short preset_count = 0;
    int total;
    TrialMissionPreview *window;
    int i;
    u_long pressed;
    short result;
    short file_id;

    held = 0;
    GsInitGraph2(512, 240, 4, 1, 0);
    ClearTrialDisplay(512, 480);
    LoadTrialPreviewImages();
    LoadTrialMissionListGraphics(saved_only);
    for (i = 0; i < TRIAL_LIST_CAPACITY; i++)
    {
        headers[i].checksum = 0;
        previews[i].id = i;
        previews[i].available = 0;
    }
    if (!saved_only)
        preset_count = LoadTrialPresetPreviews(0, headers, previews);
    total = preset_count;
    ClearTrialPages();
    SetDispMask(1);
    DrawSync(0);
    VSync(0);
    while (phase != 0)
    {
        StartDrawing();
        switch (phase)
        {
        case LIST_FADE_IN:
            brightness -= 12;
            if (brightness <= 0)
            {
                brightness = 0;
                phase++;
                held = GetRealPad(0);
            }
            DrawTrialScreenFade(brightness, 512, 240, 0);
            break;
        case LIST_RESET:
            phase++;
            reveal_frames = 0;
            break;
        case LIST_CHECK_CARD:
            TrialCardStatus = CARD_RESULT_SUCCESS;
            TrialCardStatus = CheckTrialCard();
            if (TrialCardStatus == CARD_RESULT_SUCCESS ||
                TrialCardStatus == CARD_RESULT_NEW_CARD)
                phase = LIST_START_DIRECTORY;
            else
            {
                phase = LIST_SHOW_DETAILS;
                window = NULL;
                if (!saved_only)
                    window = previews;
            }
            break;
        case LIST_SHOW_DETAILS:
            if (total == 0)
                TrialThemePreviewSprite->sprite.attribute |= GS_ATTR_HIDDEN;
            else
                TrialThemePreviewSprite->sprite.attribute &= ~GS_ATTR_HIDDEN;
            TrialListHelp->sprite.attribute &= ~GS_ATTR_HIDDEN;
            if ((short)SlideTrialMissionDetails(1))
                phase++;
            break;
        case LIST_BROWSE:
            pressed = ReadTrialMenuPad(0, &held, 0);
            result = check_cheat_command_((short)held, (short)pressed);
            if (result == TRIAL_CHEAT_UNLOCK_PRESETS &&
                (TRIAL_STATE->mission_flags & TRIAL_PRESET_COMPLETION_MASK) !=
                    TRIAL_PRESET_COMPLETION_MASK)
            {
                TRIAL_STATE->mission_flags |= TRIAL_PRESET_COMPLETION_MASK;
                SoundEx(NULL, TRIAL_SOUND_UNLOCK);
                phase = LIST_UNLOCK_REFRESH;
            }
            if (pressed == PADLup)
            {
                if (selected != 0)
                {
                    selected--;
                    if (row != 0)
                        row--;
                    else
                        window--;
                    LoadTrialThemePreview(&window[row]);
                    SoundEx(NULL, TRIAL_SOUND_CURSOR);
                }
            }
            else if (pressed == PADLdown)
            {
                if (selected != total - 1 && total != 0)
                {
                    selected++;
                    if (row != TRIAL_LIST_VISIBLE_ROWS - 1)
                        row++;
                    else
                        window++;
                    LoadTrialThemePreview(&window[row]);
                    SoundEx(NULL, TRIAL_SOUND_CURSOR);
                }
            }
            else if (pressed == PADRdown)
            {
                accepted = 0;
                phase++;
                SoundEx(NULL, TRIAL_SOUND_CANCEL);
            }
            else if (pressed == PADRright)
            {
                if (window)
                {
                    accepted = 1;
                    phase++;
                    SoundEx(NULL, TRIAL_SOUND_ACCEPT);
                }
            }
            else if (pressed == PADRup && saved_only && total)
            {
                phase = LIST_START_DELETE;
                SoundEx(NULL, TRIAL_SOUND_CANCEL);
            }
            if (held == 0)
                idle_ticks++;
            if (idle_ticks % 15 == 0 && pressed == 0 &&
                CheckTrialCard() == CARD_RESULT_NEW_CARD)
            {
                phase = LIST_CARD_CHANGED;
                reveal_frames = 0;
            }
            SlideTrialMissionDetails(2);
            DrawTrialMissionList(window, row, reveal_frames, 5);
            i = selected - row + TRIAL_LIST_VISIBLE_ROWS - 1;
            if (i > TRIAL_LIST_VISIBLE_ROWS - 1)
                DrawTrialListArrow(0, -136, -60, 10);
            if (i < total - 1)
                DrawTrialListArrow(1, -136, 80, 10);
            reveal_frames++;
            break;
        case LIST_HIDE_DETAILS:
            if ((short)SlideTrialMissionDetails(0))
                phase++;
            break;
        case LIST_CHOOSE_ACTION:
            phase = accepted ? LIST_LOAD_MISSION : LIST_FADE_OUT;
            break;
        case LIST_LOAD_MISSION:
            pressed = ReadTrialMenuPad(0, &held, 0);
            TrialListHelp->sprite.attribute |= GS_ATTR_HIDDEN;
            if (window[row].id < TRIAL_PRESET_COUNT)
            {
                phase++;
                LoadTrialPresetMission(&window[row]);
            }
            else
            {
                switch (UpdateTrialCardLoad(pressed, &headers[selected], 1))
                {
                case -1:
                    TrialUserMission = 1;
                    phase++;
                    break;
                case 1:
                    phase = LIST_RESET;
                    selected = 0;
                    row = 0;
                    total = preset_count;
                    break;
                }
            }
            break;
        case LIST_FADE_OUT:
            brightness += 12;
            if (brightness >= 128)
            {
                brightness = 128;
                phase++;
            }
            DrawTrialScreenFade(brightness, 512, 240, 0);
            reveal_frames = 0;
            break;
        case LIST_START_DIRECTORY:
            for (i = preset_count; i < TRIAL_LIST_CAPACITY; i++)
            {
                headers[i].checksum = 0;
                previews[i].available = 0;
            }
            selected = 0;
            row = 0;
            total = 0;
            /* fall through: begin polling the card in the same update. */
        case LIST_READ_DIRECTORY:
            result = UpdateTrialCardLoad(ReadTrialMenuPad(0, &held, 0),
                                        &headers[preset_count], 0);
            phase = result == -1 ? LIST_BUILD_PREVIEWS :
                    result == 0 ? LIST_READ_DIRECTORY : LIST_DIRECTORY_FAILED;
            break;
        case LIST_BUILD_PREVIEWS:
            total = preset_count;
            file_id = TRIAL_PRESET_COUNT;
            for (i = preset_count; i < TRIAL_LIST_CAPACITY; i++)
            {
                if ((short)PrepareTrialMissionPreview(&headers[i], &previews[i]))
                {
                    total++;
                    previews[i].id = file_id;
                    file_id++;
                }
            }
            window = previews;
            LoadTrialThemePreview(window);
            phase = LIST_SHOW_DETAILS;
            break;
        case LIST_DIRECTORY_FAILED:
            window = saved_only ? NULL : previews;
            total = saved_only ? 0 : preset_count;
            phase = LIST_SHOW_DETAILS;
            break;
        case LIST_CARD_CHANGED:
            if ((short)SlideTrialMissionDetails(0))
            {
                phase = LIST_START_DIRECTORY;
                TrialListHelp->sprite.attribute |= GS_ATTR_HIDDEN;
            }
            break;
        case LIST_START_DELETE:
            if ((short)SlideTrialMissionDetails(0))
            {
                phase = LIST_DELETE;
                TrialListHelp->sprite.attribute |= GS_ATTR_HIDDEN;
                title_x = -4 * strlen(headers[selected].title);
            }
            break;
        case LIST_DELETE:
            pressed = ReadTrialMenuPad(0, &held, 0);
            if (UpdateTrialCardDelete(pressed, &headers[selected]))
                phase = LIST_RESET;
            SetupTrialColoredTelop((u8 *)headers[selected].title, 3, TRIAL_TELOP_YELLOW);
            draw_telop_line_(OTablePt->org, title_x, -35, headers[selected].title);
            break;
        case LIST_UNLOCK_REFRESH:
            if ((short)SlideTrialMissionDetails(0))
            {
                phase = LIST_RESET;
                selected = 0;
                row = 0;
                if (!saved_only)
                    preset_count = LoadTrialPresetPreviews(0, headers, previews);
                total = preset_count;
            }
            break;
        default:
            phase = 0;
            break;
        }
        DrawTrialImagePanel(TrialMissionListPanel, 100);
        GsSortSprite(&TrialListHelp->sprite, OTablePt, 10);
        GsSortSprite(&TrialListTitle->sprite, OTablePt, 10);
        GsSortSprite(&TrialListHighlight->sprite, OTablePt, 11);
        SkipFrame = 2;
        EndDrawing(-2);
    }
    GsInitGraph2(320, 240, 4, 1, 0);
    GsInit3D();
    ClearTrialPages();
    SetDispMask(0);
    DisposeTrialMissionListGraphics();
    return accepted;
}
