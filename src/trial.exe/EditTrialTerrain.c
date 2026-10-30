#include "common.h"
#include "mission_editor.h"
#include "terrain_support.h"
#include "../main.exe/font.h"
#include "../main.exe/sound.h"

enum trial_tile_edit
{
    TRIAL_REMOVE_ORNAMENT = 1,
    TRIAL_CLEAR_GROUND = 2,
    TRIAL_RESTORE_GROUND = 4,
    TRIAL_TILE_CHANGED = 8
};

/* Terrain and ornaments occupy separate layers. Circle paints the selected
 * tile; Cross removes one layer at a time, or clears both while moving. */
void EditTrialTerrain(void)
{
    TrialMapCell preview;
    char *label = NULL;
    short center_x, center_z;
    short scroll = 0;
    short palette_center;
    short spin = 0;
    short rotation = 0;
    short fade = 128;
    short fade_direction = 1;
    short active = 1;
    int repeat;
    short x, z, selected;
    short i, j, k;
    int turns;
    ModelType *model = TrialEditorTileModel;
    u_long buttons = GetRealPad(0);
    u_long previous_buttons;
    TrialPlacement *placements = TrialMission.placements;

    selected = 0;
    palette_center = 2;
    center_x = TrialEditorCenterX;
    center_z = TrialEditorCenterZ;
    repeat = 0;
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
        model->rotate.vx = 128;
        model->rotate.vy = 0;
        model->rotate.vz = 0;
        for (i = -2; i < 3; i++)
        {
            j = palette_center + i;
            if (j == selected)
            {
                spin += 20;
                TrialTerrainScale = 80;
                TrialTerrainBrightness = 192;
                TrialTerrainHeightSkew = 0;
                model->rotate.vy = spin;
            }
            else
            {
                TrialTerrainScale = 50;
                TrialTerrainBrightness = 160;
                TrialTerrainHeightSkew = 0;
                model->rotate.vy = 0;
            }
            model->locate.coord.t[0] = i * 180 + 550;
            model->locate.coord.t[1] = 650;
            model->locate.coord.t[2] = 1000;
            UpdateCoordinate2(model);
            preview[0] = preview[1] = TRIAL_EMPTY_TILE;
            if (TrialTileDefinitions[j].flags & TRIAL_TILE_ORNAMENT_MASK)
                preview[1] = j;
            else
                preview[0] = j;
            DrawTrialEditorTile(preview, NULL);
        }
        TrialEditorCursors.map.x = (x - center_x) * 25;
        TrialEditorCursors.map.y = (z - center_z) * 25;
        GsSortSprite(&TrialEditorCursors.map, OTablePt, 170);
        TrialEditorCursors.palette.x = (selected - palette_center) * 27 + 82;
        GsSortSprite(&TrialEditorCursors.palette, OTablePt, 170);
        if (label == NULL)
            label = PrepareTrialTileLabel(selected);
        if (fade_direction == 0)
            draw_telop_line_(0, -152, 90, label);
        for (i = 0; i < 4; i++)
            if ((scroll >> i) & 1)
                GsSortSprite(&TrialEditorScrollArrows[i], OTablePt, 3);
        scroll = 0;

        if (fade_direction != 0 || active == 0)
        {
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
                j = x;
                k = z;
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
                if (j != x || k != z)
                    SoundEx(NULL, TRIAL_EDITOR_SOUND_MOVE);
            }
        }
        if (buttons & PADRright)
        {
            for (i = 0; i < TRIAL_PLACEMENT_COUNT; i++)
                if (placements[i].kind != CHARACTER_KIND_END &&
                    placements[i].x == x && placements[i].z == z)
                    break;
            if (i != TRIAL_PLACEMENT_COUNT &&
                ((TrialTileDefinitions[selected].flags & TRIAL_TILE_ORNAMENT_MASK) ||
                 ClassifyTrialTerrainSupport(TrialTileDefinitions[selected].flags &
                                            TRIAL_TILE_KIND_MASK) <= 0))
                continue;
            if (!(TrialTileDefinitions[selected].flags & TRIAL_TILE_ORNAMENT_MASK))
            {
                i = 0;
                if (ClassifyTrialTerrainSupport(TrialTileDefinitions[selected].flags &
                                               TRIAL_TILE_KIND_MASK) <= 0)
                    i = 1;
                if (TrialEditorMap[z][x][0] != TRIAL_EMPTY_TILE &&
                    (TrialTileDefinitions[TrialEditorMap[z][x][0] & TRIAL_TILE_INDEX_MASK].flags &
                     TRIAL_TILE_KIND_MASK) == TRIAL_TERRAIN_HOLE)
                    i = 1;
                if (i)
                    TrialEditorMap[z][x][1] = TRIAL_EMPTY_TILE;
                if (TrialEditorMap[z][x][0] != (selected | (rotation << 6)))
                {
                    TrialEditorMap[z][x][0] = selected | (rotation << 6);
                    SoundEx(NULL, TRIAL_EDITOR_SOUND_PLACE);
                }
            }
            else
            {
                i = TrialEditorMap[z][x][0];
                if (i != TRIAL_EMPTY_TILE)
                {
                    if (TrialTileDefinitions[selected].flags & TRIAL_TILE_DISGUISED_ORNAMENT)
                    {
                        if ((TrialTileDefinitions[i & TRIAL_TILE_INDEX_MASK].flags &
                             TRIAL_TILE_KIND_MASK) == TRIAL_TERRAIN_HOLE)
                        {
                            i = j = k = 0;
                            while (k < TRIAL_MAP_HEIGHT)
                            {
                                if (TrialEditorMap[k][j][1] != TRIAL_EMPTY_TILE)
                                    i++;
                                k += ++j >= TRIAL_MAP_WIDTH ? (j = 0, 1) : 0;
                            }
                            if (i < TRIAL_TERRAIN_ORNAMENT_CAPACITY &&
                                TrialEditorMap[z][x][1] != (selected | (rotation << 6)))
                            {
                                TrialEditorMap[z][x][1] = selected | (rotation << 6);
                                SoundEx(NULL, TRIAL_EDITOR_SOUND_PLACE);
                            }
                        }
                    }
                    else if (ClassifyTrialTerrainSupport(TrialTileDefinitions[
                                 i & TRIAL_TILE_INDEX_MASK].flags & TRIAL_TILE_KIND_MASK) > 0)
                    {
                        i = j = k = 0;
                        while (k < TRIAL_MAP_HEIGHT)
                        {
                            if (TrialEditorMap[k][j][1] != TRIAL_EMPTY_TILE)
                                i++;
                            k += ++j >= TRIAL_MAP_WIDTH ? (j = 0, 1) : 0;
                        }
                        if (i < TRIAL_TERRAIN_ORNAMENT_CAPACITY &&
                            TrialEditorMap[z][x][1] != (selected | (rotation << 6)))
                        {
                            TrialEditorMap[z][x][1] = selected | (rotation << 6);
                            SoundEx(NULL, TRIAL_EDITOR_SOUND_PLACE);
                        }
                    }
                }
            }
        }
        else if (buttons & PADRdown)
        {
            for (i = 0; i < TRIAL_PLACEMENT_COUNT; i++)
                if (placements[i].kind != CHARACTER_KIND_END &&
                    placements[i].x == x && placements[i].z == z)
                    break;
            if (i != TRIAL_PLACEMENT_COUNT)
                continue;
            /* One brush stroke chooses layers, then records any change. */
            i = 0;
            if (buttons & (PADLleft | PADLright | PADLup | PADLdown))
                i = TRIAL_REMOVE_ORNAMENT | TRIAL_CLEAR_GROUND;
            else if (previous_buttons == 0)
            {
                if (TrialEditorMap[z][x][1] != TRIAL_EMPTY_TILE)
                    i = TRIAL_REMOVE_ORNAMENT;
                else if (TrialEditorMap[z][x][0] != TRIAL_EMPTY_TILE)
                {
                    if (TrialEditorMap[z][x][0] == (short)TrialDefaultTiles[TrialMission.header.theme])
                        i = TRIAL_CLEAR_GROUND;
                    else
                        i = TRIAL_RESTORE_GROUND;
                }
            }
            if ((i & TRIAL_REMOVE_ORNAMENT) && TrialEditorMap[z][x][1] != TRIAL_EMPTY_TILE)
            {
                i |= TRIAL_TILE_CHANGED;
                TrialEditorMap[z][x][1] = TRIAL_EMPTY_TILE;
            }
            if (TrialEditorMap[z][x][0] != TRIAL_EMPTY_TILE)
            {
                if (i & TRIAL_CLEAR_GROUND)
                {
                    i |= TRIAL_TILE_CHANGED;
                    TrialEditorMap[z][x][0] = TRIAL_EMPTY_TILE;
                }
                else if (i & TRIAL_RESTORE_GROUND)
                {
                    i |= TRIAL_TILE_CHANGED;
                    TrialEditorMap[z][x][0] = TrialDefaultTiles[TrialMission.header.theme];
                }
            }
            if (i & TRIAL_TILE_CHANGED)
                SoundEx(NULL, TRIAL_EDITOR_SOUND_REMOVE);
        }
        if (previous_buttons == 0)
        {
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
                if (buttons == PADR2 && selected < TrialTileDefinitionCount - 1)
                    selected++;
                if (buttons == PADL2 && selected > 0)
                    selected--;
                if (selected < palette_center - 2)
                    palette_center = selected + 2;
                else if (selected > palette_center + 2)
                    palette_center = selected - 2;
                if (i != selected)
                {
                    SoundEx(NULL, TRIAL_EDITOR_SOUND_SELECT);
                    spin = 0;
                    rotation = 0;
                }
            }
            else if (buttons == PADRup || buttons == PADRleft)
            {
                i = buttons == PADRup ? 64 : -64;
                turns = rotation;
                if (buttons == PADRup)
                    rotation = (turns + 1) % 4;
                else
                    rotation = (turns + 3) % 4;
                if (TrialEditorMap[z][x][1] != TRIAL_EMPTY_TILE)
                    TrialEditorMap[z][x][1] = i + TrialEditorMap[z][x][1];
                else if (TrialEditorMap[z][x][0] != TRIAL_EMPTY_TILE)
                    TrialEditorMap[z][x][0] = i + TrialEditorMap[z][x][0];
                SoundEx(NULL, TRIAL_EDITOR_SOUND_ROTATE);
            }
        }
    }
    TrialEditorCenterX = center_x;
    TrialEditorCenterZ = center_z;
}
