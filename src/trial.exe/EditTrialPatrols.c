#include "common.h"
#include "mission_editor.h"
#include "../main.exe/font.h"
#include "../main.exe/sound.h"
#include <psxsdk/libgte.h>

/* Browse placements, then build a route one facing/step at a time. Circle
 * adds a wait, Cross undoes the last point, and Start finishes the route. */
void EditTrialPatrols(void)
{
    GsLINE line;
    SVECTOR point;
    MATRIX matrix;
    DVECTOR screen;
    TrialPlacement *previous_placement = NULL;
    short center_x, center_z;
    short scroll = 0;
    short preview_index = 0;
    short preview_direction = 1;
    short fade = 128;
    short fade_direction = 1;
    short active = 1;
    short editing = 0;
    short last_facing = -1;
    u_long previous_buttons;
    u_long buttons = GetRealPad(0);
    int repeat = 0;
    short x, z, i, offset_x, offset_z;
    short previous_x, previous_z, value, kind, label_index, direction;
    int arrow;
    TrialPatrolPoint *endpoint;
    char *label;
    TrialPlacement *placement;
    TrialPlacement *placements = TrialMission.placements;

    line.attribute = 0;
    line.r = line.g = 192;
    line.b = 0;
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
        DrawTrialEditorOverviewFrame(x, z, center_x, center_z);
        if (!editing)
            DrawTrialEditorCharacters(x, z, center_x, center_z);
        else
        {
            offset_x = placement->x - center_x;
            offset_z = placement->z - center_z;
            if ((u_short)(offset_x + 6) < 13 && offset_z >= -4 && offset_z < 4)
            {
                value = GetTrialTerrainHeight(placement->x, placement->z, TRIAL_HEIGHT_GROUND);
                point.vx = offset_x * 250 - TrialTerrainHeightSkew * value;
                point.vy = offset_z * 250 - TrialTerrainHeightSkew * value;
                point.vz = value * TrialTerrainScale + 2000;
                GsGetLs(&World.locate, &matrix);
                GsSetLsMatrix(&matrix);
                RotTransPers(&point, (long *)&screen, NULL, NULL);
                screen.vy += 8;
                DrawTrialCharacterThumbnail(placement->kind, screen.vx, screen.vy, 10, 70);
            }
        }
        if (!editing)
        {
            for (i = 0; i < TRIAL_PLACEMENT_COUNT; i++)
                if (placements[i].kind != CHARACTER_KIND_END &&
                    placements[i].x == x && placements[i].z == z)
                    break;
            placement = i == TRIAL_PLACEMENT_COUNT ? NULL : &placements[i];
            if (previous_placement != placement)
            {
                previous_placement = placement;
                preview_index = 0;
            }
        }
        if (placement)
        {
            if (fade_direction == 0)
            {
                kind = placement->kind;
                for (label_index = 0; TrialCharacterLabels[label_index].kind != CHARACTER_KIND_END &&
                            TrialCharacterLabels[label_index].kind != kind; label_index++)
                    ;
                SetupTelop((u8 *)TrialCharacterLabels[label_index].text, 0);
                draw_telop_line_(NULL, -152, 90, (u8 *)TrialCharacterLabels[label_index].text);
                label = PrepareTrialPatrolLabel(placement, editing);
                draw_telop_line_(NULL, 152 - telop_text_width_((u8 *)label), 90, (u8 *)label);
            }
            /* Closed routes wrap; open routes preview in both directions. */
            if (GameClock % 10 == 0 && placement->route_last != 0)
            {
                preview_index += preview_direction;
                if (preview_index < 0)
                {
                    preview_index = 0;
                    preview_direction = 1;
                }
                else if (preview_index > placement->route_last)
                {
                    if (placement->route[0].x == placement->route[placement->route_last].x &&
                        placement->route[0].z == placement->route[placement->route_last].z)
                    {
                        preview_index = 0;
                        preview_direction = 1;
                    }
                    else
                    {
                        preview_index = placement->route_last;
                        preview_direction = -1;
                    }
                }
            }
            for (i = 0; i <= placement->route_last; i++)
            {
                if (!editing && placement->route_last == 0)
                    break;
                offset_x = placement->route[i].x - center_x;
                offset_z = placement->route[i].z - center_z;
                value = GetTrialTerrainHeight(placement->route[i].x, placement->route[i].z,
                                              TRIAL_HEIGHT_GROUND);
                point.vx = offset_x * 250 - TrialTerrainHeightSkew * value;
                point.vy = offset_z * 250 - TrialTerrainHeightSkew * value;
                point.vz = value * TrialTerrainScale + 2000;
                GsGetLs(&World.locate, &matrix);
                GsSetLsMatrix(&matrix);
                RotTransPers(&point, (long *)&screen, NULL, NULL);
                line.x1 = screen.vx;
                line.y1 = screen.vy;
                if ((u_short)(offset_x + 6) < 13 && offset_z >= -4 && offset_z < 4)
                {
                    if (editing || preview_index == i)
                    {
                        direction = placement->route[i].facing;
                        if (placement->route[i].wait)
                            direction += 4;
                        else if (!editing && preview_direction == -1)
                            direction = (direction + 2) % 4;
                        arrow = direction;
                        TrialPatrolArrows[arrow].x = screen.vx;
                        TrialPatrolArrows[arrow].y = screen.vy;
                        GsSortSprite(&TrialPatrolArrows[arrow], OTablePt, 9);
                        if (placement->route[i].wait)
                        {
                            value = placement->route[i].wait;
                            TrialPatrolWaitDigits[value].x = screen.vx;
                            TrialPatrolWaitDigits[value].y = screen.vy;
                            switch (arrow)
                            {
                            case 4: TrialPatrolWaitDigits[value].y -= 21; break;
                            case 5: TrialPatrolWaitDigits[value].x += 21; break;
                            case 6: TrialPatrolWaitDigits[value].y += 21; break;
                            case 7: TrialPatrolWaitDigits[value].x -= 21; break;
                            }
                            GsSortSprite(&TrialPatrolWaitDigits[value], OTablePt, 8);
                        }
                    }
                    if (i != 0 && line.x0 > -160 && line.x0 < 160 &&
                        line.y0 > -120 && line.y0 < 90)
                        GsSortLine(&line, OTablePt, 11);
                }
                line.x0 = line.x1;
                line.y0 = line.y1;
            }
        }
        TrialEditorCursors.map.x = (x - center_x) * 25;
        TrialEditorCursors.map.y = (z - center_z) * 25;
        GsSortSprite(&TrialEditorCursors.map, OTablePt, 170);
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
            if (repeat == 0 || (!editing && repeat > 10 && repeat % 3 == 0))
            {
                previous_x = x;
                previous_z = z;
                if (!editing)
                {
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
                else
                {
                    if (buttons & PADLup)
                    {
                        i = 0;
                        z = z - (z > 0);
                    }
                    else if (buttons & PADLright)
                    {
                        i = 1;
                        x = x + (x < TRIAL_MAP_WIDTH - 1);
                    }
                    else if (buttons & PADLdown)
                    {
                        i = 2;
                        z = z + (z < TRIAL_MAP_HEIGHT - 1);
                    }
                    else if (buttons & PADLleft)
                    {
                        i = 3;
                        x = x - (x > 0);
                    }
                    if (IsTrialPatrolStepBlocked(previous_x, previous_z, x, z, i))
                    {
                        x = previous_x;
                        z = previous_z;
                    }
                    /* A direction press first turns the endpoint; pressing the
                     * same direction again extends the route. */
                    if (last_facing == i && (previous_x != x || previous_z != z))
                    {
                        if (placement->route_last < TRIAL_PATROL_CAPACITY - 1)
                        {
                            placement->route_last++;
                            if (placement->route_last < TRIAL_PATROL_CAPACITY - 1)
                            {
                                placement->route[placement->route_last].x = center_x = x;
                                placement->route[placement->route_last].z = center_z = z;
                                placement->route[placement->route_last].facing = i;
                            }
                            SoundEx(NULL, TRIAL_EDITOR_SOUND_PLACE);
                        }
                        else
                        {
                            SoundEx(NULL, TRIAL_EDITOR_SOUND_REMOVE);
                            x = previous_x;
                            z = previous_z;
                        }
                    }
                    else
                    {
                        x = previous_x;
                        z = previous_z;
                        placement->route[placement->route_last].facing = i;
                        SoundEx(NULL, TRIAL_EDITOR_SOUND_ROTATE);
                    }
                    last_facing = i;
                }
            }
        }
        if (previous_buttons == 0 && editing)
        {
            if (buttons == PADRright)
            {
                i = placement->route_last - 1;
                if (i >= 0 && placement->route[i].facing ==
                              placement->route[placement->route_last].facing &&
                    placement->route[i].wait != 0)
                {
                    if (placement->route[i].wait < 9)
                    {
                        placement->route[i].wait++;
                        SoundEx(NULL, TRIAL_EDITOR_SOUND_PLACE);
                    }
                    else
                        SoundEx(NULL, TRIAL_EDITOR_SOUND_REMOVE);
                }
                else if (placement->route_last < TRIAL_PATROL_CAPACITY - 1)
                {
                    placement->route[placement->route_last++].wait = 1;
                    if (placement->route_last < TRIAL_PATROL_CAPACITY - 1)
                    {
                        center_x = x;
                        placement->route[placement->route_last].x = center_x;
                        center_z = z;
                        placement->route[placement->route_last].z = center_z;
                        placement->route[placement->route_last].facing =
                            placement->route[placement->route_last - 1].facing;
                    }
                    SoundEx(NULL, TRIAL_EDITOR_SOUND_PLACE);
                }
                else
                    SoundEx(NULL, TRIAL_EDITOR_SOUND_REMOVE);
            }
            else if (buttons == PADRdown && placement->route_last != 0)
            {
                placement->route[placement->route_last--].wait = 0;
                center_x = placement->route[placement->route_last].x;
                center_z = placement->route[placement->route_last].z;
                placement->route[placement->route_last].wait = 0;
                x = center_x;
                z = center_z;
                SoundEx(NULL, TRIAL_EDITOR_SOUND_REMOVE);
            }
            else if (buttons == PADstart)
            {
                editing = 0;
                preview_index = 0;
                preview_direction = 1;
                if (placement->route_last != 0 && placement->route[placement->route_last - 1].wait)
                    placement->route_last--;
                x = center_x = placement->x;
                z = center_z = placement->z;
                if (center_x < 5)
                    center_x = 5;
                else if (center_x > 25)
                    center_x = 26;
                if (center_z < 3)
                    center_z = 3;
                else if (center_z > 27)
                    center_z = 28;
                SoundEx(NULL, TRIAL_SOUND_ACCEPT);
                continue;
            }
        }
        if (previous_buttons == 0 && !editing)
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
                if (placement)
                {
                    for (i = 0; i < TRIAL_PLACEMENT_COUNT; i++)
                        if (placement == &placements[i])
                            break;
                    value = buttons == PADR2 ? 1 : TRIAL_PLACEMENT_COUNT - 1;
                    for (i = (i + value) % TRIAL_PLACEMENT_COUNT;
                         placement != &placements[i] &&
                         placements[i].kind == CHARACTER_KIND_END;
                         i = (i + value) % TRIAL_PLACEMENT_COUNT)
                        ;
                }
                else
                    i = 0;
                placement = &placements[i];
                x = center_x = placement->x;
                z = center_z = placement->z;
                if (center_x < 5)
                    center_x = 5;
                else if (center_x > 25)
                    center_x = 26;
                if (center_z < 3)
                    center_z = 3;
                else if (center_z > 27)
                    center_z = 28;
                preview_index = 0;
                preview_direction = 1;
                SoundEx(NULL, TRIAL_EDITOR_SOUND_SELECT);
            }
            else if (buttons == PADRdown && placement)
            {
                placement->route_last = 0;
                SoundEx(NULL, TRIAL_EDITOR_SOUND_REMOVE);
            }
            else if (buttons == PADRright && placement)
            {
                if (placement->kind == 0 || placement->kind == TRIAL_HERB_OBJECT)
                    SoundEx(NULL, TRIAL_EDITOR_SOUND_REMOVE);
                else
                {
                    editing = 1;
                    last_facing = -1;
                    if (placement->route_last == 0)
                    {
                        memset(placement->route, 0, sizeof(placement->route));
                        placement->route[0].x = placement->x;
                        placement->route[0].z = placement->z;
                    }
                    else if (placement->route_last == TRIAL_PATROL_CAPACITY - 1)
                        placement->route_last--;
                    x = center_x = placement->route[placement->route_last].x;
                    z = center_z = placement->route[placement->route_last].z;
                    /* A saved wait needs a fresh, non-waiting endpoint before
                     * the next move can be edited. Copy the preceding record. */
                    if (placement->route_last != 0 && placement->route_last < TRIAL_PATROL_CAPACITY - 1 &&
                        placement->route[placement->route_last].wait)
                    {
                        placement->route_last++;
                        endpoint = (TrialPatrolPoint *)((placement->route_last - 1) *
                            sizeof(*endpoint) + (u_long)placement->route);
                        endpoint[1] = *endpoint;
                        placement->route[placement->route_last].wait = 0;
                    }
                    SoundEx(NULL, TRIAL_EDITOR_SOUND_PLACE);
                }
            }
        }
    }
    TrialEditorCenterX = center_x;
    TrialEditorCenterZ = center_z;
}
