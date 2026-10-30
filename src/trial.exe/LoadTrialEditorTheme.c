#include "common.h"
#include "mission_editor.h"
#include "../main.exe/tmdfile.h"
#include "../main.exe/timpack.h"

static const char TileFilename[] = "%stheme%d.lnd";
static const char ThemeDirectory[] = "K:\\WORK\\CDIMAGE\\TRIAL\\THEME\\";
static const char TextureFilename[] = "%stheme%d.tpd";
static const char OrnamentFilename[] = "%s%s.tmd";

/* A negative theme unloads the current resources. Each theme supplies its
 * tile definitions, texture pack, and the ornament models named by the tiles. */
void LoadTrialEditorTheme(short theme)
{
    char filename[64];
    GsIMAGE image;
    int i, count;
    u_long *data;

    DrawTrialEditorTile(NULL, NULL);
    if (TrialTileDefinitions)
    {
        vfree(TrialTileDefinitions);
        TrialTileDefinitions = NULL;
    }
    if (TrialTileTextures)
    {
        vfree(TrialTileTextures);
        TrialTileTextures = NULL;
    }
    if (TrialThemeOrnaments)
    {
        count = vsize(TrialThemeOrnaments) / sizeof(*TrialThemeOrnaments);
        for (i = 0; i < count; i++)
        {
            data = TrialThemeOrnaments[i]->object.tmd;
            vfree((u8 *)data - (sizeof(TMDFile) - sizeof(TmdObjectRecord)));
            DisposeOrnament(TrialThemeOrnaments[i]);
        }
        vfree(TrialThemeOrnaments);
        TrialThemeOrnaments = NULL;
    }
    TrialTerrainOrnamentCount = 0;
    TrialTileDefinitionCount = 0;
    if (theme >= 0)
    {
        sprintf(filename, TileFilename, ThemeDirectory, theme);
        TrialTileDefinitions = FileRead(filename);
        TrialTileDefinitionCount = vsize(TrialTileDefinitions) / sizeof(*TrialTileDefinitions);
        sprintf(filename, TextureFilename, ThemeDirectory, theme);
        data = FileRead(filename);
        count = data[1];
        TrialTileTextures = valloc(count * sizeof(*TrialTileTextures));
        for (i = 0; i < count; i++)
        {
            GetTIMpackInfo(data, &image, i);
            TrialTileTextures[i].page = image.px / 64 + image.py / 256;
            TrialTileTextures[i].clut = image.cx / 16 + image.cy * 64;
            TrialTileTextures[i].u = (short)(image.px % 64) * 4;
            TrialTileTextures[i].v = image.py % 256;
        }
        LoadTIMpackAndFree(data);
        TrialTileTextures = vmemoryGC(TrialTileTextures);
        count = 0;
        for (i = 0; i < TrialTileDefinitionCount; i++)
            if (TrialTileDefinitions[i].flags & TRIAL_TILE_ORNAMENT_MASK)
                count++;
        TrialThemeOrnaments = valloc(count * sizeof(*TrialThemeOrnaments));
        count = 0;
        for (i = 0; i < TrialTileDefinitionCount; i++)
        {
            if (TrialTileDefinitions[i].flags & TRIAL_TILE_ORNAMENT_MASK)
            {
                sprintf(filename, OrnamentFilename, ThemeDirectory,
                    TrialTileDefinitions[i].data.ornament_name);
                TrialThemeOrnaments[count] = LoadOrnament(FileRead(filename));
                TrialTileDefinitions[i].data.ornament_index = count;
                count++;
            }
        }
    }
}
