#include "common.h"
#include "mission_menu.h"
#include "../main.exe/images.h"
#include "../main.exe/font.h"
#include "../main.exe/sound.h"
#include "../main.exe/padcmd.h"
#include <psxsdk/libgpu.h>

static Sprite3D **const SettingsSprites[7] = {
    &TrialOptionCursor, &TrialOptionThemeLabels, &TrialOptionObjectiveLabels,
    &TrialOptionStealthLabels, &TrialOptionHeading, &TrialOptionHelp,
    &TrialOptionThemePreview
};

enum { TRIAL_CHEAT_FUTURE_THEME = 0x2000 };

/* Return the field number for text entry, 10 to continue, or 11 to leave.
 * A mission's theme can only be changed while creating a new mission. */
short EditTrialMissionSettings(short editing, TrialMissionHeader *header)
{
    short themes[TRIAL_THEME_COUNT];
    char password[18];
    char title[32];
    char author[18];
    Sprite3D **sprites[7];
    short values[TRIAL_OPTION_COUNT];
    u_long previous_buttons = 0;
    short row = TRIAL_FIELD_TITLE;
    short theme_index = 0;
    short objective;
    short stealth;
    short phase = 1;
    short brightness = 128;
    short cheat_used = 0;
    short last_theme = 1;
    short theme_base_v;
    u_long pressed;
    short changed;
    int i;

    __builtin_memcpy(sprites, SettingsSprites, sizeof(sprites));
    ClearTrialDisplay(320, 480);
    LoadTrialOptionGraphics(editing);
    memset(password, 0, sizeof(password));
    memset(title, 0, sizeof(title));
    for (i = 0; i < (int)strlen(header->password) / 2; i++)
        strcat(password, "\201\226"); /* ＊ */
    strncpy(title, header->title, sizeof(title));
    strncpy(author, header->author, sizeof(author));
    if (!editing)
    {
        last_theme = GetTrialUnlockedThemes(themes);
        for (i = 0; i < TRIAL_THEME_COUNT; i++)
        {
            if (header->theme == themes[i])
            {
                theme_index = i;
                LoadTIM(get_tim_from_archive(TrialSettingsPreviewArchive, themes[theme_index]));
                break;
            }
        }
    }
    else
    {
        memset(themes, 0, sizeof(themes));
        themes[0] = header->theme;
        theme_index = 0;
    }
    theme_base_v = TrialOptionThemeLabels->sprite.v;
    objective = header->flags & TRIAL_OBJECTIVE_MASK;
    stealth = header->flags & TRIAL_REQUIRE_STEALTH;
    stealth = stealth != 0;
    TrialOptionThemeLabels->sprite.v += themes[theme_index] * 16;
    TrialOptionObjectiveLabels->sprite.v += objective * 16;
    TrialOptionStealthLabels->sprite.v += stealth * 16;
    SetDispMask(1);
    DrawSync(0);
    VSync(0);
    while (phase)
    {
        StartDrawing();
        switch (phase)
        {
        case 1:
            brightness -= 16;
            if (brightness <= 0)
            {
                brightness = 0;
                phase++;
                previous_buttons = GetRealPad(0);
            }
            DrawTrialScreenFade(brightness, 512, 240, 0);
            break;
        case 2:
            pressed = ReadTrialMenuPad(0, &previous_buttons, 0);
            if ((short)check_cheat_command_(previous_buttons, pressed) == TRIAL_CHEAT_FUTURE_THEME &&
                !cheat_used && !editing)
            {
                /* TRIAL uses the English-finale completion bit for A.D. 2030. */
                TRIAL_STATE->mission_flags |= MISSION_FLAG_ENGLISH_FINAL_STAGE;
                last_theme = GetTrialUnlockedThemes(themes);
                cheat_used = 1;
                SoundEx(NULL, TRIAL_SOUND_UNLOCK);
            }
            if (!(previous_buttons & PADRleft))
            {
                if (pressed == PADLup && row != TRIAL_FIELD_TITLE)
                {
                    row--;
                    TrialOptionCursor->sprite.y -= 23;
                    SoundEx(NULL, TRIAL_SOUND_CURSOR);
                }
                if (pressed == PADLdown && row != TRIAL_FIELD_STEALTH)
                {
                    row++;
                    TrialOptionCursor->sprite.y += 23;
                    SoundEx(NULL, TRIAL_SOUND_CURSOR);
                }
                if (pressed == PADLleft)
                {
                    changed = 0;
                    if (row == TRIAL_FIELD_THEME && theme_index != 0 && !editing)
                    {
                        theme_index--;
                        changed = 1;
                        TrialOptionThemeLabels->sprite.v = theme_base_v + (themes[theme_index] * 16);
                    }
                    if (row == TRIAL_FIELD_OBJECTIVE && objective != 0)
                    {
                        objective--;
                        changed = 1;
                        TrialOptionObjectiveLabels->sprite.v -= 16;
                    }
                    if (row == TRIAL_FIELD_STEALTH && stealth != 0)
                    {
                        stealth--;
                        changed = 1;
                        TrialOptionStealthLabels->sprite.v -= 16;
                    }
                    if (changed)
                    {
                        SoundEx(NULL, TRIAL_SOUND_OPTION);
                        if (row == TRIAL_FIELD_THEME)
                            LoadTIM(get_tim_from_archive(TrialSettingsPreviewArchive, themes[theme_index]));
                    }
                }
                if (pressed == PADLright)
                {
                    changed = 0;
                    if (row == TRIAL_FIELD_THEME && theme_index < last_theme && !editing)
                    {
                        theme_index++;
                        changed = 1;
                        TrialOptionThemeLabels->sprite.v = theme_base_v + (themes[theme_index] * 16);
                    }
                    if (row == TRIAL_FIELD_OBJECTIVE && objective != 4)
                    {
                        objective++;
                        changed = 1;
                        TrialOptionObjectiveLabels->sprite.v += 16;
                    }
                    if (row == TRIAL_FIELD_STEALTH && stealth != 1)
                    {
                        stealth++;
                        changed = 1;
                        TrialOptionStealthLabels->sprite.v += 16;
                    }
                    if (changed)
                    {
                        SoundEx(NULL, TRIAL_SOUND_OPTION);
                        if (row == TRIAL_FIELD_THEME)
                            LoadTIM(get_tim_from_archive(TrialSettingsPreviewArchive, themes[theme_index]));
                    }
                }
                if (pressed == PADRdown)
                {
                    row = TRIAL_SETTINGS_LEAVE;
                    phase++;
                    if (editing)
                        header->flags = objective | (stealth ? TRIAL_REQUIRE_STEALTH : 0);
                    SoundEx(NULL, TRIAL_SOUND_CANCEL);
                    break;
                }
                if (pressed == PADRright)
                {
                    switch (row)
                    {
                    case TRIAL_FIELD_TITLE:
                        strncpy(TrialTextEntry, header->title, sizeof(header->title));
                        SoundEx(NULL, TRIAL_SOUND_ACCEPT);
                        break;
                    case TRIAL_FIELD_AUTHOR:
                        strncpy(TrialTextEntry, header->author, sizeof(header->author));
                        SoundEx(NULL, TRIAL_SOUND_ACCEPT);
                        break;
                    case TRIAL_FIELD_PASSWORD:
                        strncpy(TrialTextEntry, header->password, sizeof(header->password));
                        SoundEx(NULL, TRIAL_SOUND_ACCEPT);
                        break;
                    }
                    if (row < TRIAL_FIELD_THEME)
                    {
                        phase++;
                        if (!editing)
                            header->theme = themes[theme_index];
                        header->flags = objective | (stealth ? TRIAL_REQUIRE_STEALTH : 0);
                    }
                }
                if (pressed == PADstart && !editing)
                {
                    row = TRIAL_SETTINGS_CONTINUE;
                    phase++;
                    header->theme = themes[theme_index];
                    header->flags = objective | (stealth ? TRIAL_REQUIRE_STEALTH : 0);
                    SoundEx(NULL, TRIAL_SOUND_ACCEPT);
                    break;
                }
            }
            values[0] = theme_index;
            values[1] = objective;
            values[2] = stealth;
            /* The original evaluates this subscript even for text rows;
             * DrawTrialOptionArrows ignores the value outside rows 4..6. */
            if (row != TRIAL_FIELD_THEME || !editing)
                DrawTrialOptionArrows(row, values[row - TRIAL_OPTION_FIRST_ROW], last_theme);
            break;
        case 3:
            brightness += 16;
            if (brightness >= 128)
            {
                brightness = 128;
                phase++;
            }
            DrawTrialScreenFade(brightness, 512, 240, 0);
            break;
        default:
            phase = 0;
            break;
        }
        DrawBG(TrialSettingsBackground);
        for (i = 0; i < 7; i++)
            GsSortSprite(&(*sprites[i])->sprite, OTablePt, 10);
        SetupTelop(title, 0);
        draw_telop_line_(OTablePt->org, -88, -72, title);
        SetupTelop(author, 1);
        draw_telop_line_(OTablePt->org, -60, -48, author);
        SetupTelop(password, 2);
        draw_telop_line_(OTablePt->org, -60, -24, password);
        SkipFrame = 2;
        EndDrawing(-2);
    }
    ClearTrialPages();
    SetDispMask(0);
    DisposeTrialOptionGraphics();
    return row;
}
