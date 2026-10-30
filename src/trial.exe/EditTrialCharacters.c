#include "common.h"
#include "mission_editor.h"
#include "terrain_support.h"
#include "../main.exe/font.h"
#include "../main.exe/sound.h"

/* Place the objective characters first, then the theme's optional roster.
 * Objective slots cannot be deleted, and each named boss has one instance. */
void EditTrialCharacters(void)
{
    short characters[16];
    char *label;
    short center_x, center_z;
    short palette_center;
    short fixed_count;
    short scroll = 0;
    short fade = 128;
    short fade_direction = 1;
    short active = 1;
    int repeat;
    short x, z, selected;
    short i, roster_start, previous_x, previous_z;
    short kind, objective;
    u_long buttons = GetRealPad(0);
    u_long previous_buttons;
    TrialPlacement *placements = TrialMission.placements;

    repeat = 0;
    fixed_count = SetupTrialObjectivePlacements(0);
    label = NULL;
    for (i = 0; i < fixed_count; i++)
        characters[i] = placements[i].kind;
    objective = TrialMission.header.flags & TRIAL_OBJECTIVE_MASK;
    i = 2;
    /* Both playable characters are already represented by the player slot.
     * Assassination/elimination also reserve the theme's boss at index 2. */
    roster_start = objective < TRIAL_PROTECT_PRINCESS && objective != TRIAL_REACH_LORD ? ++i : i;
    do
    {
        kind = characters[i + fixed_count - roster_start] = StageAppearance[TrialMission.header.theme + 1][i];
        i++;
    } while (kind != CHARACTER_KIND_END);

    selected = 0;
    palette_center = 2;
    center_x = TrialEditorCenterX;
    center_z = TrialEditorCenterZ;
    ViewInfo.vpx = 0;
    ViewInfo.vpy = 0;
    ViewInfo.vpz = -1000;
    ViewInfo.vrx = 0;
    ViewInfo.vry = 0;
    ViewInfo.vrz = 0;
    GsSetRefView2(&ViewInfo);
    x = center_x;
    z = center_z;

    while (active >= 0 || fade_direction != 0)
    {
        StartDrawing();
        DrawTrialEditorTerrain(x, z, center_x, center_z);
        DrawTrialEditorCharacters(x, z, center_x, center_z);
        DrawTrialEditorOverviewFrame(x, z, center_x, center_z);
        for (i = -2; i < 3; i++)
            DrawTrialCharacterThumbnail(characters[palette_center + i],
                i * 28 + 83, 106, 10, palette_center + i == selected ? 100 : 50);
        TrialEditorCursors.map.x = (x - center_x) * 25;
        TrialEditorCursors.map.y = (z - center_z) * 25;
        GsSortSprite(&TrialEditorCursors.map, OTablePt, 170);
        TrialEditorCursors.palette.x = (selected - palette_center) * 27 + 82;
        GsSortSprite(&TrialEditorCursors.palette, OTablePt, 170);
        if (label == NULL)
            label = PrepareTrialCharacterLabel(characters[selected]);
        if (fade_direction == 0)
            draw_telop_line_(0, -152, 90, label);
        for (i = 0; i < 4; i++)
            if ((scroll >> i) & 1)
                GsSortSprite(&TrialEditorScrollArrows[i], OTablePt, 3);
        scroll = 0;

        if (fade_direction != 0 || active == 0)
        {
            /* Clamp the rendered endpoint and finish its fade phase together. */
            if (fade_direction == 1)
            {
                fade -= 16;
                DrawTrialScreenFade(fade <= 0 ? (fade_direction = 0, fade = 0) : fade,
                                    320, 240, 0);
            }
            else if (fade_direction == -1)
            {
                fade += 16;
                DrawTrialScreenFade(fade >= 128 ? (fade_direction = 0, fade = 128) : fade,
                                    320, 240, 0);
            }
            if (active == 0)
            {
                active = UpdateTrialEditorCommands(1);
                if (active == -1)
                    fade_direction = -1;
            }
            EndDrawing(-2);
            continue;
        }
        EndDrawing(-2);
        previous_buttons = buttons;
        buttons = GetRealPad(0);
        if (buttons & (PADLleft | PADLright | PADLup | PADLdown))
        {
            if (previous_buttons & (PADLleft | PADLright | PADLup | PADLdown))
                repeat++;
            else
                repeat = 0;
            if (repeat == 0 || (repeat > 10 && repeat % 3 == 0))
            {
                previous_x = x;
                previous_z = z;
                if ((buttons & PADLleft) && x > 0)
                    x--;
                else if ((buttons & PADLright) && x < TRIAL_MAP_WIDTH - 1)
                    x++;
                if ((buttons & PADLup) && z > 0)
                    z--;
                else if ((buttons & PADLdown) && z < TRIAL_MAP_HEIGHT - 1)
                    z++;
                if (x < center_x - 5)
                {
                    scroll |= 4;
                    center_x = x + 5;
                }
                else if (x > center_x + 5)
                {
                    scroll |= 1;
                    center_x = x - 5;
                }
                if (z < center_z - 3)
                {
                    scroll |= 8;
                    center_z = z + 3;
                }
                else if (z > center_z + 3)
                {
                    scroll |= 2;
                    center_z = z - 3;
                }
                if (previous_x != x || previous_z != z)
                    SoundEx(NULL, TRIAL_EDITOR_SOUND_MOVE);
            }
        }
        if (previous_buttons != 0)
            continue;
        if (buttons == PADselect)
        {
            SoundEx(NULL, TRIAL_EDITOR_SOUND_COMMANDS);
            active = 0;
        }
        else if (buttons & PADL1)
        {
            PreviewTrialTerrain(x, z);
            ViewInfo.vpx = 0;
            ViewInfo.vpy = 0;
            ViewInfo.vpz = -1000;
            ViewInfo.vrx = 0;
            ViewInfo.vry = 0;
            ViewInfo.vrz = 0;
            GsSetRefView2(&ViewInfo);
        }
        else if (buttons == PADL2 || buttons == PADR2)
        {
            i = selected;
            label = NULL;
            if (buttons == PADR2 && characters[selected + 1] != CHARACTER_KIND_END)
                selected++;
            if (buttons == PADL2 && selected > 0)
                selected--;
            if (selected < palette_center - 2)
                palette_center = selected + 2;
            else if (selected > palette_center + 2)
                palette_center = selected - 2;
            if (i != selected)
                SoundEx(NULL, TRIAL_EDITOR_SOUND_SELECT);
        }
        else if (buttons == PADRright)
        {
            /* Occupied cells, ornaments, and unsupported terrain reject placement. */
            for (i = 0; i < TRIAL_PLACEMENT_COUNT; i++)
                if (placements[i].kind != CHARACTER_KIND_END &&
                    placements[i].x == x && placements[i].z == z)
                    break;
            if (i != TRIAL_PLACEMENT_COUNT ||
                TrialEditorMap[z][x][0] == TRIAL_EMPTY_TILE ||
                TrialEditorMap[z][x][1] != TRIAL_EMPTY_TILE ||
                ClassifyTrialTerrainSupport(TrialTileDefinitions[
                    TrialEditorMap[z][x][0] & TRIAL_TILE_INDEX_MASK].flags &
                    TRIAL_TILE_KIND_MASK) <= 0)
                continue;
            for (i = 0; i < fixed_count; i++)
                if (characters[selected] == characters[i])
                    break;
            if (i >= fixed_count)
            {
                if ((characters[selected] & PAGE_MASK) == PAGE_BOSS)
                {
                    for (i = 0; i < TRIAL_PLACEMENT_COUNT; i++)
                        if (placements[i].kind == characters[selected])
                            break;
                    if (i != TRIAL_PLACEMENT_COUNT)
                        goto place_character;
                }
                for (i = 0; i < TRIAL_PLACEMENT_COUNT; i++)
                    if (placements[i].kind == CHARACTER_KIND_END)
                        break;
                if (i == TRIAL_PLACEMENT_COUNT)
                    continue;
                placements[i].x = placements[i].z = -1;
                placements[i].kind = characters[selected];
            }
        place_character:
            if (i != TRIAL_PLACEMENT_COUNT)
            {
                if (placements[i].x != x || placements[i].z != z)
                    SoundEx(NULL, TRIAL_EDITOR_SOUND_PLACE);
                placements[i].x = x;
                placements[i].z = z;
                placements[i].route_last = 0;
            }
        }
        else if (buttons == PADRdown)
        {
            for (i = 0; i < TRIAL_PLACEMENT_COUNT; i++)
                if (placements[i].x == x && placements[i].z == z)
                    break;
            if (i != TRIAL_PLACEMENT_COUNT && i >= fixed_count)
            {
                placements[i].kind = CHARACTER_KIND_END;
                SoundEx(NULL, TRIAL_EDITOR_SOUND_REMOVE);
            }
        }
    }
    TrialEditorCenterX = center_x;
    TrialEditorCenterZ = center_z;
}
